#include "Internal.hpp"

namespace RedFrame
{
AkRegisterCaptureCallback_t GetAkRegisterCaptureCallback()
{
    static RED4ext::RelocFunc<AkRegisterCaptureCallback_t> callback(kAkRegisterCaptureCallbackRva);
    return callback;
}

AkUnregisterCaptureCallback_t GetAkUnregisterCaptureCallback()
{
    static RED4ext::RelocFunc<AkUnregisterCaptureCallback_t> callback(kAkUnregisterCaptureCallbackRva);
    return callback;
}

std::int32_t AkRegisterCaptureCallbackHook(AkCaptureCallback_t aCallback,
                                           const std::uint64_t aOutputDeviceId,
                                           void* aCookie)
{
    const auto callCount = ++g_akRegisterCaptureCallbackCalls;
    LogWarn("AK::SoundEngine::RegisterCaptureCallback called: count=%u callback=%p outputDeviceId=%llu cookie=%p captureActive=%s includeAudio=%s",
            callCount,
            reinterpret_cast<void*>(aCallback),
            static_cast<unsigned long long>(aOutputDeviceId),
            aCookie,
            g_captureSession.active ? "true" : "false",
            g_captureSession.includeAudio ? "true" : "false");

    const auto result = g_akRegisterCaptureCallbackOriginal
                            ? g_akRegisterCaptureCallbackOriginal(aCallback, aOutputDeviceId, aCookie)
                            : -1;
    LogWarn("AK::SoundEngine::RegisterCaptureCallback returned: result=%d count=%u", result, callCount);
    return result;
}

std::int32_t AkUnregisterCaptureCallbackHook(AkCaptureCallback_t aCallback,
                                             const std::uint64_t aOutputDeviceId,
                                             void* aCookie)
{
    const auto callCount = ++g_akUnregisterCaptureCallbackCalls;
    LogWarn("AK::SoundEngine::UnregisterCaptureCallback called: count=%u callback=%p outputDeviceId=%llu cookie=%p captureActive=%s includeAudio=%s",
            callCount,
            reinterpret_cast<void*>(aCallback),
            static_cast<unsigned long long>(aOutputDeviceId),
            aCookie,
            g_captureSession.active ? "true" : "false",
            g_captureSession.includeAudio ? "true" : "false");

    const auto result = g_akUnregisterCaptureCallbackOriginal
                            ? g_akUnregisterCaptureCallbackOriginal(aCallback, aOutputDeviceId, aCookie)
                            : -1;
    LogWarn("AK::SoundEngine::UnregisterCaptureCallback returned: result=%d count=%u", result, callCount);
    return result;
}

void AttachAudioCaptureCallbackProbe()
{
    if (g_audioCaptureCallbackProbeAttached)
    {
        return;
    }

    if (!g_sdk || !g_sdk->hooking || !g_sdk->hooking->Attach)
    {
        LogWarn("Audio capture callback probe skipped: RED4ext hooking API unavailable.");
        return;
    }

    auto* registerTarget = reinterpret_cast<void*>(GetAkRegisterCaptureCallback());
    auto* unregisterTarget = reinterpret_cast<void*>(GetAkUnregisterCaptureCallback());
    const auto registerAttached = g_sdk->hooking->Attach(
        g_pluginHandle,
        registerTarget,
        reinterpret_cast<void*>(&AkRegisterCaptureCallbackHook),
        reinterpret_cast<void**>(&g_akRegisterCaptureCallbackOriginal));
    const auto unregisterAttached = g_sdk->hooking->Attach(
        g_pluginHandle,
        unregisterTarget,
        reinterpret_cast<void*>(&AkUnregisterCaptureCallbackHook),
        reinterpret_cast<void**>(&g_akUnregisterCaptureCallbackOriginal));

    g_audioCaptureCallbackProbeAttached = registerAttached || unregisterAttached;
    LogWarn("Audio capture callback probe attach: registerTarget=%p attached=%s original=%p unregisterTarget=%p attached=%s original=%p",
            registerTarget,
            registerAttached ? "true" : "false",
            reinterpret_cast<void*>(g_akRegisterCaptureCallbackOriginal),
            unregisterTarget,
            unregisterAttached ? "true" : "false",
            reinterpret_cast<void*>(g_akUnregisterCaptureCallbackOriginal));
}

void DetachAudioCaptureCallbackProbe()
{
    if (!g_audioCaptureCallbackProbeAttached || !g_sdk || !g_sdk->hooking || !g_sdk->hooking->Detach)
    {
        return;
    }

    auto* registerTarget = reinterpret_cast<void*>(GetAkRegisterCaptureCallback());
    auto* unregisterTarget = reinterpret_cast<void*>(GetAkUnregisterCaptureCallback());
    const auto registerDetached = g_sdk->hooking->Detach(g_pluginHandle, registerTarget);
    const auto unregisterDetached = g_sdk->hooking->Detach(g_pluginHandle, unregisterTarget);
    LogWarn("Audio capture callback probe detach: registerDetached=%s unregisterDetached=%s registerCalls=%u unregisterCalls=%u",
            registerDetached ? "true" : "false",
            unregisterDetached ? "true" : "false",
            g_akRegisterCaptureCallbackCalls.load(),
            g_akUnregisterCaptureCallbackCalls.load());
    g_audioCaptureCallbackProbeAttached = false;
    g_akRegisterCaptureCallbackOriginal = nullptr;
    g_akUnregisterCaptureCallbackOriginal = nullptr;
}

void RedFrameAudioCaptureCallback(void* aAudioBuffer, const std::uint64_t aOutputDeviceId, void* aCookie)
{
    const auto callbackCount = ++g_audioSelfRegisterProbeCallbacks;
    g_audioSelfRegisterProbeLastDeviceId.store(aOutputDeviceId);

    const auto* buffer = static_cast<const AkAudioBufferProbe*>(aAudioBuffer);
    const auto data = buffer ? buffer->data : nullptr;
    const auto fullConfig = buffer ? buffer->channelConfig.fullConfig : 0;
    const auto numChannels = buffer ? buffer->channelConfig.GetNumChannels() : 0;
    const auto configType = buffer ? buffer->channelConfig.GetConfigType() : 0;
    const auto channelMask = buffer ? buffer->channelConfig.GetChannelMask() : 0;
    const auto state = buffer ? buffer->state : 0;
    const auto maxFrames = buffer ? buffer->frameWord10 : 0;
    const auto validFrames = buffer ? buffer->frameWord12 : 0;
    const auto tailWord14 = buffer ? buffer->tailWord14 : 0;

    if (data && numChannels > 0 && numChannels <= kAudioCaptureMaxChannels && maxFrames >= validFrames &&
        validFrames > 0)
    {
        auto* sourceSamples = static_cast<const float*>(data);
        std::lock_guard lock(g_audioSidecarCaptureState.mutex);
        if (g_audioSidecarCaptureState.registered)
        {
            ++g_audioSidecarCaptureState.callbacks;
            g_audioSidecarCaptureState.lastOutputDeviceId = aOutputDeviceId;
            if (g_audioSidecarCaptureState.channels == 0)
            {
                g_audioSidecarCaptureState.channels = numChannels;
            }

            if (g_audioSidecarCaptureState.writeSidecar && g_audioSidecarCaptureState.channels == numChannels)
            {
                const auto sampleCount = static_cast<std::size_t>(validFrames) * numChannels;

                // The observed Wwise capture buffer for this callback is already in WAV-ready order.
                // Treating it as planar produces channel/time scrambling in captured game audio.
                const auto oldSize = g_audioSidecarCaptureState.samples.size();
                g_audioSidecarCaptureState.samples.resize(oldSize + sampleCount);
                std::copy(sourceSamples,
                          sourceSamples + sampleCount,
                          g_audioSidecarCaptureState.samples.data() + oldSize);
                g_audioSidecarCaptureState.frames += validFrames;
            }
            else if (g_audioSidecarCaptureState.writeSidecar)
            {
                ++g_audioSidecarCaptureState.skippedBuffers;
            }
        }
    }
    else
    {
        std::lock_guard lock(g_audioSidecarCaptureState.mutex);
        if (g_audioSidecarCaptureState.registered)
        {
            ++g_audioSidecarCaptureState.callbacks;
            ++g_audioSidecarCaptureState.skippedBuffers;
            g_audioSidecarCaptureState.lastOutputDeviceId = aOutputDeviceId;
        }
    }

    if (g_autoRunConfig.probeAudioSelfRegister && (callbackCount <= 5 || callbackCount % 100 == 0))
    {
        LogWarn("RedFrame audio self-register callback fired: count=%u buffer=%p data=%p outputDeviceId=%llu cookie=%p channels=%u configType=%u fullConfig=0x%08X channelMask=0x%08X state=%d frameWord10=%u frameWord12=%u tailWord14=0x%08X",
                callbackCount,
                aAudioBuffer,
                data,
                static_cast<unsigned long long>(aOutputDeviceId),
                aCookie,
                numChannels,
                configType,
                fullConfig,
                channelMask,
                state,
                maxFrames,
                validFrames,
                tailWord14);
    }
}

void WriteU16LE(FILE* aFile, const std::uint16_t aValue)
{
    const std::uint8_t bytes[] = {
        static_cast<std::uint8_t>(aValue & 0xFFu),
        static_cast<std::uint8_t>((aValue >> 8u) & 0xFFu),
    };
    std::fwrite(bytes, sizeof(bytes), 1, aFile);
}

void WriteU32LE(FILE* aFile, const std::uint32_t aValue)
{
    const std::uint8_t bytes[] = {
        static_cast<std::uint8_t>(aValue & 0xFFu),
        static_cast<std::uint8_t>((aValue >> 8u) & 0xFFu),
        static_cast<std::uint8_t>((aValue >> 16u) & 0xFFu),
        static_cast<std::uint8_t>((aValue >> 24u) & 0xFFu),
    };
    std::fwrite(bytes, sizeof(bytes), 1, aFile);
}

bool WriteFloatWavFile(const std::filesystem::path& aPath,
                       const std::vector<float>& aSamples,
                       const std::uint32_t aChannels,
                       const std::uint32_t aSampleRate)
{
    if (aSamples.empty() || aChannels == 0 || aSampleRate == 0)
    {
        LogWarn("Audio sidecar skipped: no samples captured.");
        return false;
    }

    const auto dataSize64 = static_cast<std::uint64_t>(aSamples.size()) * sizeof(float);
    if (dataSize64 > std::numeric_limits<std::uint32_t>::max() - 36u)
    {
        LogWarn("Audio sidecar skipped: WAV data is too large for RIFF32 (%llu bytes).",
                static_cast<unsigned long long>(dataSize64));
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(aPath.parent_path(), ec);
    if (ec)
    {
        LogWarn("Audio sidecar failed: could not create %s (%d)", aPath.parent_path().string().c_str(), ec.value());
        return false;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, aPath.string().c_str(), "wb") != 0 || !file)
    {
        LogWarn("Audio sidecar failed: could not open %s", aPath.string().c_str());
        return false;
    }

    const auto dataSize = static_cast<std::uint32_t>(dataSize64);
    const auto fmtSize = 16u;
    const auto blockAlign = static_cast<std::uint16_t>(aChannels * sizeof(float));
    const auto byteRate = aSampleRate * blockAlign;

    std::fwrite("RIFF", 4, 1, file);
    WriteU32LE(file, 36u + dataSize);
    std::fwrite("WAVE", 4, 1, file);
    std::fwrite("fmt ", 4, 1, file);
    WriteU32LE(file, fmtSize);
    WriteU16LE(file, 3); // WAVE_FORMAT_IEEE_FLOAT
    WriteU16LE(file, static_cast<std::uint16_t>(aChannels));
    WriteU32LE(file, aSampleRate);
    WriteU32LE(file, byteRate);
    WriteU16LE(file, blockAlign);
    WriteU16LE(file, 32);
    std::fwrite("data", 4, 1, file);
    WriteU32LE(file, dataSize);
    std::fwrite(aSamples.data(), sizeof(float), aSamples.size(), file);
    const auto closeResult = std::fclose(file);
    if (closeResult != 0)
    {
        LogWarn("Audio sidecar failed while closing %s", aPath.string().c_str());
        return false;
    }

    LogInfo("Audio sidecar written: path=%s channels=%u sampleRate=%u samples=%llu bytes=%u",
            aPath.string().c_str(),
            aChannels,
            aSampleRate,
            static_cast<unsigned long long>(aSamples.size()),
            dataSize);
    return true;
}

bool StartAudioSidecarCapture(const std::filesystem::path& aOutputPath, const bool aWriteSidecar)
{
    auto registerCallback = g_akRegisterCaptureCallbackOriginal ? g_akRegisterCaptureCallbackOriginal
                                                                : GetAkRegisterCaptureCallback();
    if (!registerCallback)
    {
        LogWarn("Audio self-register probe skipped: AK::SoundEngine::RegisterCaptureCallback unavailable.");
        return false;
    }

    {
        std::lock_guard lock(g_audioSidecarCaptureState.mutex);
        if (g_audioSidecarCaptureState.registered)
        {
            return true;
        }

        g_audioSidecarCaptureState.registered = true;
        g_audioSidecarCaptureState.writeSidecar = aWriteSidecar;
        g_audioSidecarCaptureState.outputPath = aOutputPath;
        g_audioSidecarCaptureState.channels = 0;
        g_audioSidecarCaptureState.sampleRate = kAudioCaptureSampleRate;
        g_audioSidecarCaptureState.callbacks = 0;
        g_audioSidecarCaptureState.frames = 0;
        g_audioSidecarCaptureState.skippedBuffers = 0;
        g_audioSidecarCaptureState.lastOutputDeviceId = 0;
        g_audioSidecarCaptureState.samples.clear();
        if (aWriteSidecar)
        {
            g_audioSidecarCaptureState.samples.reserve(kAudioCaptureSampleRate * 2 * 10);
        }
    }

    g_audioSelfRegisterProbeCallbacks.store(0);
    g_audioSelfRegisterProbeLastDeviceId.store(0);
    g_audioSelfRegisterProbeActive.store(true);
    g_audioSelfRegisterProbeCookie = &g_audioSelfRegisterProbeCallbacks;

    const auto result =
        registerCallback(RedFrameAudioCaptureCallback, kAudioCaptureDefaultOutputDeviceId, g_audioSelfRegisterProbeCookie);
    const auto success = result == 1;
    if (!success)
    {
        std::lock_guard lock(g_audioSidecarCaptureState.mutex);
        g_audioSidecarCaptureState.registered = false;
        g_audioSidecarCaptureState.writeSidecar = false;
        g_audioSidecarCaptureState.samples.clear();
        g_audioSelfRegisterProbeActive.store(false);
    }

    g_audioSelfRegisterProbeActive.store(success);
    LogWarn("Audio capture callback start: result=%d active=%s writeSidecar=%s callback=%p outputDeviceId=%llu cookie=%p outputPath=%s",
            result,
            success ? "true" : "false",
            aWriteSidecar ? "true" : "false",
            reinterpret_cast<void*>(&RedFrameAudioCaptureCallback),
            static_cast<unsigned long long>(kAudioCaptureDefaultOutputDeviceId),
            g_audioSelfRegisterProbeCookie,
            aOutputPath.string().c_str());
    return success;
}

bool StopAudioSidecarCapture()
{
    {
        std::lock_guard lock(g_audioSidecarCaptureState.mutex);
        if (!g_audioSidecarCaptureState.registered)
        {
            return true;
        }
    }

    const auto callbackCount = g_audioSelfRegisterProbeCallbacks.load();
    const auto lastDeviceId = g_audioSelfRegisterProbeLastDeviceId.load();
    std::vector<float> samples;
    std::filesystem::path outputPath;
    std::uint32_t channels = 0;
    std::uint32_t sampleRate = kAudioCaptureSampleRate;
    std::uint32_t sidecarCallbacks = 0;
    std::uint64_t sidecarFrames = 0;
    std::uint32_t skippedBuffers = 0;
    std::uint64_t sidecarDeviceId = 0;
    bool writeSidecar = false;
    auto result = -1;

    auto unregisterCallback = g_akUnregisterCaptureCallbackOriginal ? g_akUnregisterCaptureCallbackOriginal
                                                                    : GetAkUnregisterCaptureCallback();
    if (unregisterCallback)
    {
        result = unregisterCallback(RedFrameAudioCaptureCallback,
                                    kAudioCaptureDefaultOutputDeviceId,
                                    g_audioSelfRegisterProbeCookie);
    }
    else
    {
        LogWarn("Audio capture callback stop could not unregister: AK::SoundEngine::UnregisterCaptureCallback unavailable.");
    }

    {
        std::lock_guard lock(g_audioSidecarCaptureState.mutex);
        writeSidecar = g_audioSidecarCaptureState.writeSidecar;
        outputPath = g_audioSidecarCaptureState.outputPath;
        channels = g_audioSidecarCaptureState.channels;
        sampleRate = g_audioSidecarCaptureState.sampleRate;
        sidecarCallbacks = g_audioSidecarCaptureState.callbacks;
        sidecarFrames = g_audioSidecarCaptureState.frames;
        skippedBuffers = g_audioSidecarCaptureState.skippedBuffers;
        sidecarDeviceId = g_audioSidecarCaptureState.lastOutputDeviceId;
        samples.swap(g_audioSidecarCaptureState.samples);
        g_audioSidecarCaptureState.registered = false;
        g_audioSidecarCaptureState.writeSidecar = false;
        g_audioSidecarCaptureState.outputPath.clear();
        g_audioSidecarCaptureState.channels = 0;
        g_audioSidecarCaptureState.callbacks = 0;
        g_audioSidecarCaptureState.frames = 0;
        g_audioSidecarCaptureState.skippedBuffers = 0;
        g_audioSidecarCaptureState.lastOutputDeviceId = 0;
        g_audioSidecarCaptureState.samples.clear();
    }

    g_audioSelfRegisterProbeActive.store(false);
    LogWarn("Audio capture callback stop: result=%d callbacks=%u sidecarCallbacks=%u frames=%llu skipped=%u lastOutputDeviceId=%llu sidecarDeviceId=%llu cookie=%p writeSidecar=%s",
            result,
            callbackCount,
            sidecarCallbacks,
            static_cast<unsigned long long>(sidecarFrames),
            skippedBuffers,
            static_cast<unsigned long long>(lastDeviceId),
            static_cast<unsigned long long>(sidecarDeviceId),
            g_audioSelfRegisterProbeCookie,
            writeSidecar ? "true" : "false");
    g_audioSelfRegisterProbeCookie = nullptr;

    if (writeSidecar)
    {
        return WriteFloatWavFile(outputPath, samples, channels, sampleRate) && result == 1;
    }

    return result == 1;
}

} // namespace RedFrame
