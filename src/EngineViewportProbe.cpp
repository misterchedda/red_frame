#include "Internal.hpp"

namespace RedFrame
{
ToggleContinuousScreenshotHandler_t GetToggleContinuousScreenshotHandler()
{
    static RED4ext::RelocFunc<ToggleContinuousScreenshotHandler_t> handler(kToggleContinuousScreenshotHandlerRva);
    return handler;
}

void* GetContinuousScreenshotObject()
{
    static RED4ext::RelocPtr<void*> object(kContinuousScreenshotObjectRva);
    return object;
}

std::int32_t GetContinuousScreenshotTakeIndex()
{
    static RED4ext::RelocPtr<std::int32_t> takeIndex(kContinuousScreenshotTakeIndexRva);
    return takeIndex;
}

GetConfigManager_t GetConfigManager()
{
    static RED4ext::RelocFunc<GetConfigManager_t> getConfigManager(kGetConfigManagerRva);
    return getConfigManager;
}

HashConfigString_t GetHashConfigString()
{
    static RED4ext::RelocFunc<HashConfigString_t> hashConfigString(kHashConfigStringRva);
    return hashConfigString;
}

SetConfigVar_t GetSetConfigVar()
{
    static RED4ext::RelocFunc<SetConfigVar_t> setConfigVar(kSetConfigVarRva);
    return setConfigVar;
}

PhotoModeSimpleStartCapture_t GetPhotoModeSimpleStartCapture()
{
    static RED4ext::RelocFunc<PhotoModeSimpleStartCapture_t> startCapture(kPhotoModeSimpleStartCaptureRva);
    return startCapture;
}

bool TryReadPointer(const void* aBase, const std::size_t aOffset, std::uintptr_t& aOut);

void* GetPhotoModeSystem()
{
    auto* engine = RED4ext::CGameEngine::Get();
    if (!engine || !engine->framework || !engine->framework->gameInstance)
    {
        return nullptr;
    }

    auto* rtti = RED4ext::CRTTISystem::Get();
    if (!rtti)
    {
        return nullptr;
    }

    auto* systemType = rtti->GetClass(RED4ext::game::PhotoModeSystem::NAME);
    auto* system = systemType ? engine->framework->gameInstance->GetSystem(systemType) : nullptr;
    if (system)
    {
        return system;
    }

    auto* interfaceType = rtti->GetClass(RED4ext::game::IPhotoModeSystem::NAME);
    return interfaceType ? engine->framework->gameInstance->GetSystem(interfaceType) : nullptr;
}

void* GetPhotoModeCaptureObject(void* aPhotoModeSystem)
{
    std::uintptr_t captureObject = 0;
    if (!TryReadPointer(aPhotoModeSystem, kPhotoModeCaptureObjectOffset, captureObject))
    {
        return nullptr;
    }

    return reinterpret_cast<void*>(captureObject);
}

bool IsLikelyProcessPointer(const void* aPointer)
{
    const auto value = reinterpret_cast<std::uintptr_t>(aPointer);
    return value >= 0x100000000 && value < 0x0000800000000000;
}

const char* GetTypeNameOrUnknown(RED4ext::rtti::IType* aType)
{
    if (!aType)
    {
        return "<unknown>";
    }

    return aType->GetName().ToString();
}

RED4ext::rtti::IType* TryGetEngineNativeType(RED4ext::CBaseEngine* aEngine)
{
    if (!aEngine)
    {
        return nullptr;
    }

#if defined(_MSC_VER)
    __try
    {
        return aEngine->GetNativeType();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
#else
    return aEngine->GetNativeType();
#endif
}

bool TryReadPointer(const void* aBase, const std::size_t aOffset, std::uintptr_t& aOut)
{
    aOut = 0;
    if (!IsLikelyProcessPointer(aBase))
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        aOut = *reinterpret_cast<const std::uintptr_t*>(reinterpret_cast<const std::uint8_t*>(aBase) + aOffset);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    aOut = *reinterpret_cast<const std::uintptr_t*>(reinterpret_cast<const std::uint8_t*>(aBase) + aOffset);
    return true;
#endif
}

bool TryReadU32(const void* aBase, const std::size_t aOffset, std::uint32_t& aOut)
{
    aOut = 0;
    if (!IsLikelyProcessPointer(aBase))
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        aOut = *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::uint8_t*>(aBase) + aOffset);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    aOut = *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::uint8_t*>(aBase) + aOffset);
    return true;
#endif
}

bool TryReadBool(const void* aBase, const std::size_t aOffset, bool& aOut)
{
    aOut = false;
    if (!IsLikelyProcessPointer(aBase))
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        aOut = *reinterpret_cast<const bool*>(reinterpret_cast<const std::uint8_t*>(aBase) + aOffset);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    aOut = *reinterpret_cast<const bool*>(reinterpret_cast<const std::uint8_t*>(aBase) + aOffset);
    return true;
#endif
}

bool TryWriteU32(void* aBase, const std::size_t aOffset, const std::uint32_t aValue)
{
    if (!IsLikelyProcessPointer(aBase))
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(aBase) + aOffset) = aValue;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(aBase) + aOffset) = aValue;
    return true;
#endif
}

bool TryWriteBool(void* aBase, const std::size_t aOffset, const bool aValue)
{
    if (!IsLikelyProcessPointer(aBase))
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        *reinterpret_cast<bool*>(reinterpret_cast<std::uint8_t*>(aBase) + aOffset) = aValue;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    *reinterpret_cast<bool*>(reinterpret_cast<std::uint8_t*>(aBase) + aOffset) = aValue;
    return true;
#endif
}

bool TryInvokePhotoModeSimpleStartCapture(void* aPhotoModeSystem)
{
    auto* startCapture = GetPhotoModeSimpleStartCapture();
    if (!startCapture || !aPhotoModeSystem)
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        startCapture(aPhotoModeSystem, nullptr, false);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    startCapture(aPhotoModeSystem, nullptr, false);
#endif

    return true;
}

bool InvokePhotoModeDefaultCapture()
{
    auto* system = GetPhotoModeSystem();
    auto* captureObject = GetPhotoModeCaptureObject(system);
    LogInfo("PhotoMode capture probe: system=%p captureObject(+0x320)=%p", system, captureObject);
    if (!system || !IsLikelyProcessPointer(captureObject))
    {
        LogWarn("PhotoMode capture probe skipped: candidate object pointer is not plausible.");
        return false;
    }

    const auto success = TryInvokePhotoModeSimpleStartCapture(system);
    LogInfo("PhotoMode simple start capture returned success=%s", success ? "true" : "false");
    return success;
}

bool InvokeEngineDefaultFrameCapture()
{
    auto* engine = RED4ext::CGameEngine::Get();
    std::uintptr_t viewportValue = 0;
    std::uintptr_t viewportRefCountValue = 0;
    const auto readViewport = TryReadPointer(engine, 0x320, viewportValue);
    const auto readViewportRefCount = TryReadPointer(engine, 0x328, viewportRefCountValue);
    auto* viewport = reinterpret_cast<EngineViewportProbe*>(viewportValue);
    auto* nativeType = TryGetEngineNativeType(engine);
    LogInfo("Engine frame capture probe: engine=%p nativeType=%s viewport(+0x320)=%p%s viewportRef(+0x328)=%p%s",
            engine,
            GetTypeNameOrUnknown(nativeType),
            viewport,
            readViewport ? "" : "!",
            reinterpret_cast<void*>(viewportRefCountValue),
            readViewportRefCount ? "" : "!");
    if (!engine || !IsLikelyProcessPointer(viewport))
    {
        LogWarn("Engine frame capture probe skipped: EngineViewport candidate is not plausible.");
        return false;
    }

    std::uintptr_t qword0 = 0;
    std::uintptr_t qword8 = 0;
    std::uintptr_t qword10 = 0;
    const auto read0 = TryReadPointer(viewport, 0x0, qword0);
    const auto read8 = TryReadPointer(viewport, 0x8, qword8);
    const auto read10 = TryReadPointer(viewport, 0x10, qword10);
    LogInfo("EngineViewport candidate memory: [0]=%p%s [8/windowInfo]=%p%s [10]=%p%s",
            reinterpret_cast<void*>(qword0),
            read0 ? "" : "!",
            reinterpret_cast<void*>(qword8),
            read8 ? "" : "!",
            reinterpret_cast<void*>(qword10),
            read10 ? "" : "!");

    auto* windowInfo = reinterpret_cast<EngineViewportWindowInfo*>(qword8);
    std::uint32_t windowWidth = 0;
    std::uint32_t windowHeight = 0;
    const auto readWindowWidth = TryReadU32(windowInfo, 0x8, windowWidth);
    const auto readWindowHeight = TryReadU32(windowInfo, 0xC, windowHeight);

    std::uintptr_t singleScreenShotDataVftable = 0;
    const auto readSingleScreenShotDataVftable = TryReadPointer(viewport, 0x190, singleScreenShotDataVftable);

    std::uint32_t captureMode = 0;
    std::uint32_t captureInitialFrame = 0;
    std::uint32_t captureOutputDirectoryIndex = 0;
    std::uint32_t captureFps = 0;
    std::uint32_t captureResolutionMultiplier = 0;
    std::uint32_t captureSaveFormat = 0;
    std::uint32_t captureContextType = 0;
    bool captureEnable = false;
    bool captureVideoRecordingMode = false;
    bool captureAudioRecordingMode = false;
    const auto captureBase = reinterpret_cast<std::uint8_t*>(viewport) + offsetof(EngineViewportProbe, captureParameters);
    const auto readCaptureMode = TryReadU32(captureBase, offsetof(RED4ext::rend::CaptureParameters, mode), captureMode);
    const auto readCaptureInitialFrame = TryReadU32(captureBase,
                                                    offsetof(RED4ext::rend::CaptureParameters, initialFrameNumber),
                                                    captureInitialFrame);
    const auto readCaptureOutputDirectoryIndex = TryReadU32(
        captureBase, offsetof(RED4ext::rend::CaptureParameters, outputDirectoryIndex), captureOutputDirectoryIndex);
    const auto readCaptureFps = TryReadU32(captureBase, offsetof(RED4ext::rend::CaptureParameters, recordingFPS), captureFps);
    const auto readCaptureResolutionMultiplier = TryReadU32(
        captureBase, offsetof(RED4ext::rend::CaptureParameters, resolutionMultiplier), captureResolutionMultiplier);
    const auto readCaptureSaveFormat = TryReadU32(captureBase, offsetof(RED4ext::rend::CaptureParameters, saveFormat), captureSaveFormat);
    const auto readCaptureContextType = TryReadU32(captureBase,
                                                   offsetof(RED4ext::rend::CaptureParameters, captureContextType),
                                                   captureContextType);
    const auto readCaptureEnable = TryReadBool(captureBase, offsetof(RED4ext::rend::CaptureParameters, enable), captureEnable);
    const auto readCaptureVideoRecordingMode = TryReadBool(
        captureBase, offsetof(RED4ext::rend::CaptureParameters, videoRecordingMode), captureVideoRecordingMode);
    const auto readCaptureAudioRecordingMode = TryReadBool(
        captureBase, offsetof(RED4ext::rend::CaptureParameters, audioRecordingMode), captureAudioRecordingMode);

    LogInfo("EngineViewport window probe: windowInfo=%p width=%u%s height=%u%s embeddedSingleScreenShotData(+0x190).vftable=%p%s",
            windowInfo,
            windowWidth,
            readWindowWidth ? "" : "!",
            windowHeight,
            readWindowHeight ? "" : "!",
            reinterpret_cast<void*>(singleScreenShotDataVftable),
            readSingleScreenShotDataVftable ? "" : "!");
    LogInfo("EngineViewport captureParameters(+0xD0): mode=%u%s initialFrame=%u%s outputDirIndex=%u%s fps=%u%s resMultiplier=%u%s saveFormat=%u%s context=%u%s enable=%s%s video=%s%s audio=%s%s",
            captureMode,
            readCaptureMode ? "" : "!",
            captureInitialFrame,
            readCaptureInitialFrame ? "" : "!",
            captureOutputDirectoryIndex,
            readCaptureOutputDirectoryIndex ? "" : "!",
            captureFps,
            readCaptureFps ? "" : "!",
            captureResolutionMultiplier,
            readCaptureResolutionMultiplier ? "" : "!",
            captureSaveFormat,
            readCaptureSaveFormat ? "" : "!",
            captureContextType,
            readCaptureContextType ? "" : "!",
            captureEnable ? "true" : "false",
            readCaptureEnable ? "" : "!",
            captureVideoRecordingMode ? "true" : "false",
            readCaptureVideoRecordingMode ? "" : "!",
            captureAudioRecordingMode ? "true" : "false",
            readCaptureAudioRecordingMode ? "" : "!");

    const auto logSingleScreenShotFields = [](const char* aLabel, const void* aSingleScreenShotData) {
        if (!IsLikelyProcessPointer(aSingleScreenShotData))
        {
            LogInfo("%s: not a plausible object pointer (%p)", aLabel, aSingleScreenShotData);
            return;
        }

        std::uintptr_t vftable = 0;
        std::uint32_t screenshotMode = 0;
        std::uint32_t screenshotResolution = 0;
        std::uint32_t screenshotResolutionMultiplier = 0;
        std::uint32_t screenshotSaveFormat = 0;
        bool screenshotForceLod0 = false;
        const auto readVftable = TryReadPointer(aSingleScreenShotData, 0x0, vftable);
        const auto readScreenshotMode = TryReadU32(aSingleScreenShotData,
                                                   offsetof(RED4ext::rend::SingleScreenShotData, mode),
                                                   screenshotMode);
        const auto readScreenshotResolution = TryReadU32(aSingleScreenShotData,
                                                         offsetof(RED4ext::rend::SingleScreenShotData, resolution),
                                                         screenshotResolution);
        const auto readScreenshotResolutionMultiplier = TryReadU32(
            aSingleScreenShotData,
            offsetof(RED4ext::rend::SingleScreenShotData, resolutionMultiplier),
            screenshotResolutionMultiplier);
        const auto readScreenshotSaveFormat = TryReadU32(aSingleScreenShotData,
                                                         offsetof(RED4ext::rend::SingleScreenShotData, saveFormat),
                                                         screenshotSaveFormat);
        const auto readScreenshotForceLod0 = TryReadBool(aSingleScreenShotData,
                                                         offsetof(RED4ext::rend::SingleScreenShotData, forceLOD0),
                                                         screenshotForceLod0);
        LogInfo("%s: object=%p vftable=%p%s mode=%u%s resolution=%u%s resMultiplier=%u%s forceLOD0=%s%s saveFormat=%u%s",
                aLabel,
                aSingleScreenShotData,
                reinterpret_cast<void*>(vftable),
                readVftable ? "" : "!",
                screenshotMode,
                readScreenshotMode ? "" : "!",
                screenshotResolution,
                readScreenshotResolution ? "" : "!",
                screenshotResolutionMultiplier,
                readScreenshotResolutionMultiplier ? "" : "!",
                screenshotForceLod0 ? "true" : "false",
                readScreenshotForceLod0 ? "" : "!",
                screenshotSaveFormat,
                readScreenshotSaveFormat ? "" : "!");
    };
    logSingleScreenShotFields("EngineViewport embedded singleScreenShotData(+0x190)",
                              reinterpret_cast<const std::uint8_t*>(viewport) + 0x190);

    LogWarn("Engine frame capture probe is inspect-only. Direct wrapper invocation is disabled after crash reporter repro.");
    return false;
}

EngineViewportProbe* GetEngineViewportProbe()
{
    auto* engine = RED4ext::CGameEngine::Get();
    std::uintptr_t viewportValue = 0;
    if (!TryReadPointer(engine, 0x320, viewportValue))
    {
        return nullptr;
    }

    auto* viewport = reinterpret_cast<EngineViewportProbe*>(viewportValue);
    return IsLikelyProcessPointer(viewport) ? viewport : nullptr;
}

bool ReadEngineViewportFrameCounter(std::uint32_t& aOut)
{
    auto* viewport = GetEngineViewportProbe();
    return viewport && TryReadU32(viewport, 0x118, aOut);
}

std::uint32_t RefreshCaptureFrameCounter()
{
    std::uint32_t frameCounter = 0;
    if (ReadEngineViewportFrameCounter(frameCounter))
    {
        g_captureSession.lastFrameCounter = frameCounter;
    }

    return g_captureSession.lastFrameCounter;
}

void LogPersistentCaptureProbeState(const char* aLabel, EngineViewportProbe* aViewport)
{
    if (!IsLikelyProcessPointer(aViewport))
    {
        LogWarn("%s persistent capture probe: invalid viewport=%p", aLabel, aViewport);
        return;
    }

    std::uint32_t frameCounter = 0;
    std::uint32_t persistentState = 0;
    std::uint32_t writerRootMode = 0;
    std::uint32_t writerMinFrameCount = 0;
    bool oneShotFlag = false;
    bool writerSkipPathSetup = false;
    bool enable = false;
    bool videoRecordingMode = false;
    bool audioRecordingMode = false;

    const auto captureBase = reinterpret_cast<std::uint8_t*>(aViewport) + offsetof(EngineViewportProbe, captureParameters);
    const auto readFrameCounter = TryReadU32(aViewport, 0x118, frameCounter);
    const auto readPersistentState = TryReadU32(aViewport, 0x180, persistentState);
    const auto readWriterRootMode = TryReadU32(aViewport, 0x16C, writerRootMode);
    const auto readWriterMinFrameCount = TryReadU32(aViewport, 0x120, writerMinFrameCount);
    const auto readWriterSkipPathSetup = TryReadBool(aViewport, 0x173, writerSkipPathSetup);
    const auto readOneShotFlag = TryReadBool(aViewport, 0x1C0, oneShotFlag);
    const auto readEnable = TryReadBool(captureBase, offsetof(RED4ext::rend::CaptureParameters, enable), enable);
    const auto readVideo = TryReadBool(
        captureBase, offsetof(RED4ext::rend::CaptureParameters, videoRecordingMode), videoRecordingMode);
    const auto readAudio = TryReadBool(
        captureBase, offsetof(RED4ext::rend::CaptureParameters, audioRecordingMode), audioRecordingMode);

    LogWarn("%s persistent capture probe: viewport=%p frameCounter(+0x118)=%u%s persistent(+0x180)=%u%s writerRoot(+0x16C)=%u%s writerMinFrames(+0x120)=%u%s writerSkipPath(+0x173)=%s%s oneShot(+0x1C0)=%s%s enable=%s%s video=%s%s audio=%s%s",
            aLabel,
            aViewport,
            frameCounter,
            readFrameCounter ? "" : "!",
            persistentState,
            readPersistentState ? "" : "!",
            writerRootMode,
            readWriterRootMode ? "" : "!",
            writerMinFrameCount,
            readWriterMinFrameCount ? "" : "!",
            writerSkipPathSetup ? "true" : "false",
            readWriterSkipPathSetup ? "" : "!",
            oneShotFlag ? "true" : "false",
            readOneShotFlag ? "" : "!",
            enable ? "true" : "false",
            readEnable ? "" : "!",
            videoRecordingMode ? "true" : "false",
            readVideo ? "" : "!",
            audioRecordingMode ? "true" : "false",
            readAudio ? "" : "!");
}

bool SavePersistentCaptureProbeState(EngineViewportProbe* aViewport)
{
    if (!IsLikelyProcessPointer(aViewport))
    {
        return false;
    }

    if (g_persistentCaptureProbeState.active)
    {
        return true;
    }

    auto& captureParameters = aViewport->captureParameters;
    const auto captureBase = reinterpret_cast<std::uint8_t*>(&captureParameters);
    TryReadU32(aViewport, 0x180, g_persistentCaptureProbeState.previousPersistentState);
    TryReadU32(aViewport, 0x118, g_persistentCaptureProbeState.previousFrameCounter);
    TryReadU32(aViewport, 0x16C, g_persistentCaptureProbeState.previousWriterRootMode);
    g_persistentCaptureProbeState.previousInitialFrameNumber = captureParameters.initialFrameNumber;
    g_persistentCaptureProbeState.previousOutputDirectoryIndex = captureParameters.outputDirectoryIndex;
    g_persistentCaptureProbeState.previousRecordingFPS = captureParameters.recordingFPS;
    g_persistentCaptureProbeState.previousCustomResolution = captureParameters.customResolution;
    g_persistentCaptureProbeState.previousResolutionMultiplier = captureParameters.resolutionMultiplier;
    g_persistentCaptureProbeState.previousSaveFormat = static_cast<std::uint32_t>(captureParameters.saveFormat);
    g_persistentCaptureProbeState.previousOutputDirectoryName = captureParameters.outputDirectoryName;
    g_persistentCaptureProbeState.previousOutputDirectoryNameSuffix = captureParameters.outputDirectoryNameSuffix;
    TryReadBool(captureBase,
                offsetof(RED4ext::rend::CaptureParameters, enable),
                g_persistentCaptureProbeState.previousEnable);
    TryReadBool(captureBase,
                offsetof(RED4ext::rend::CaptureParameters, videoRecordingMode),
                g_persistentCaptureProbeState.previousVideoRecordingMode);
    TryReadBool(captureBase,
                offsetof(RED4ext::rend::CaptureParameters, audioRecordingMode),
                g_persistentCaptureProbeState.previousAudioRecordingMode);
    g_persistentCaptureProbeState.viewport = aViewport;
    g_persistentCaptureProbeState.active = true;
    g_persistentCaptureProbeState.previousStateSaved = true;
    return true;
}

bool StartPersistentCaptureProbe(const bool aForce, const bool aIncludeAudio)
{
    if (!aForce && !g_autoRunConfig.persistentCaptureProbe)
    {
        return true;
    }

    auto* viewport = GetEngineViewportProbe();
    if (!viewport)
    {
        LogWarn("Persistent capture probe skipped: no plausible EngineViewport.");
        return false;
    }

    if (!SavePersistentCaptureProbeState(viewport))
    {
        return false;
    }

    LogPersistentCaptureProbeState("before-start", viewport);

    const auto captureBase = reinterpret_cast<std::uint8_t*>(viewport) + offsetof(EngineViewportProbe, captureParameters);
    const auto wrotePersistent = TryWriteU32(viewport, 0x180, 1);
    const auto wroteEnable =
        TryWriteBool(captureBase, offsetof(RED4ext::rend::CaptureParameters, enable), true);
    const auto wroteVideo =
        TryWriteBool(captureBase, offsetof(RED4ext::rend::CaptureParameters, videoRecordingMode), true);
    const auto wroteAudio =
        TryWriteBool(captureBase, offsetof(RED4ext::rend::CaptureParameters, audioRecordingMode), aIncludeAudio);
    auto wroteWriterRoot = true;
    if (g_autoRunConfig.probeWriterVideoRoot)
    {
        wroteWriterRoot = TryWriteU32(viewport, 0x16C, 1);
    }

    LogWarn("Persistent capture probe writes: persistent(+0x180)=%s enable=%s video=%s audio=%s writerRoot(+0x16C)=%s%s",
            wrotePersistent ? "ok" : "failed",
            wroteEnable ? "ok" : "failed",
            wroteVideo ? "ok" : "failed",
            wroteAudio ? "ok" : "failed",
            g_autoRunConfig.probeWriterVideoRoot ? (wroteWriterRoot ? "ok" : "failed") : "skipped",
            g_autoRunConfig.probeWriterVideoRoot ? "" : " (probe off)");
    LogPersistentCaptureProbeState("after-start", viewport);
    return wrotePersistent && wroteEnable && wroteVideo && wroteAudio && wroteWriterRoot;
}

void StopPersistentCaptureProbe()
{
    if (!g_persistentCaptureProbeState.active)
    {
        return;
    }

    auto* viewport = reinterpret_cast<EngineViewportProbe*>(g_persistentCaptureProbeState.viewport);
    if (!IsLikelyProcessPointer(viewport))
    {
        g_persistentCaptureProbeState = {};
        return;
    }

    LogPersistentCaptureProbeState("before-stop", viewport);

    const auto captureBase = reinterpret_cast<std::uint8_t*>(viewport) + offsetof(EngineViewportProbe, captureParameters);
    auto& captureParameters = viewport->captureParameters;
    TryWriteU32(viewport, 0x118, g_persistentCaptureProbeState.previousFrameCounter);
    TryWriteU32(viewport, 0x180, g_persistentCaptureProbeState.previousPersistentState);
    TryWriteU32(viewport, 0x16C, g_persistentCaptureProbeState.previousWriterRootMode);
    captureParameters.outputDirectoryName = g_persistentCaptureProbeState.previousOutputDirectoryName;
    captureParameters.outputDirectoryNameSuffix = g_persistentCaptureProbeState.previousOutputDirectoryNameSuffix;
    captureParameters.initialFrameNumber = g_persistentCaptureProbeState.previousInitialFrameNumber;
    captureParameters.outputDirectoryIndex = g_persistentCaptureProbeState.previousOutputDirectoryIndex;
    captureParameters.recordingFPS = g_persistentCaptureProbeState.previousRecordingFPS;
    captureParameters.customResolution = g_persistentCaptureProbeState.previousCustomResolution;
    captureParameters.resolutionMultiplier = g_persistentCaptureProbeState.previousResolutionMultiplier;
    captureParameters.saveFormat = static_cast<RED4ext::ESaveFormat>(g_persistentCaptureProbeState.previousSaveFormat);
    TryWriteBool(captureBase,
                 offsetof(RED4ext::rend::CaptureParameters, enable),
                 g_persistentCaptureProbeState.previousEnable);
    TryWriteBool(captureBase,
                 offsetof(RED4ext::rend::CaptureParameters, videoRecordingMode),
                 g_persistentCaptureProbeState.previousVideoRecordingMode);
    TryWriteBool(captureBase,
                 offsetof(RED4ext::rend::CaptureParameters, audioRecordingMode),
                 g_persistentCaptureProbeState.previousAudioRecordingMode);

    LogPersistentCaptureProbeState("after-stop", viewport);
    g_persistentCaptureProbeState = {};
}

bool TryGetConfigManager(std::uintptr_t* aOut)
{
    if (!aOut)
    {
        return false;
    }

    *aOut = 0;
    auto* getConfigManager = GetConfigManager();
    if (!getConfigManager)
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        *aOut = getConfigManager();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    *aOut = getConfigManager();
#endif

    return *aOut != 0;
}

bool TrySetConfigVar(const std::uintptr_t aManager, void* aGroupKey, void* aNameKey, void* aValue)
{
    auto* setConfigVar = GetSetConfigVar();
    if (!setConfigVar)
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
        setConfigVar(aManager, aGroupKey, aNameKey, aValue);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    setConfigVar(aManager, aGroupKey, aNameKey, aValue);
#endif

    return true;
}

bool SetConfigVarString(const char* aGroup, const char* aName, const char* aValue)
{
    auto* hashConfigString = GetHashConfigString();
    if (!GetConfigManager() || !hashConfigString || !GetSetConfigVar())
    {
        LogError("ConfigVar helpers were null. get=%p hash=%p set=%p",
                 GetConfigManager(),
                 hashConfigString,
                 GetSetConfigVar());
        return false;
    }

    std::uintptr_t manager = 0;
    if (!TryGetConfigManager(&manager))
    {
        LogError("GetConfigManager failed while setting %s/%s.", aGroup, aName);
        return false;
    }

    ConfigVarKey groupKey{};
    ConfigVarKey nameKey{};
    groupKey.text = aGroup;
    nameKey.text = aName;
    groupKey.hash = hashConfigString(aGroup);
    nameKey.hash = hashConfigString(aName);
    RED4ext::CString value(aValue);

    if (!TrySetConfigVar(manager, &groupKey, &nameKey, &value))
    {
        LogError("SetConfigVar(%s/%s=%s) failed.", aGroup, aName, aValue);
        return false;
    }

    LogInfo("SetConfigVar(%s/%s=%s) returned.", aGroup, aName, aValue);
    return true;
}

bool SetCaptureSequentialFrames(const bool aEnabled)
{
    return SetConfigVarString("Render", "CaptureSequentialFrames", aEnabled ? "true" : "false");
}

bool ValidateCaptureStartOptions(const CaptureStartOptions& aOptions)
{
    constexpr auto kMaxCaptureFPS = 240;
    if (aOptions.recordingFPS < 0 || aOptions.recordingFPS > kMaxCaptureFPS)
    {
        LogWarn("Invalid capture fps=%d. Expected 0..%d.", aOptions.recordingFPS, kMaxCaptureFPS);
        return false;
    }

    return true;
}

bool ConfigureEngineViewportCaptureParameters(EngineViewportProbe* aViewport,
                                              const std::int32_t aSaveFormat,
                                              const CaptureOutputDirectory& aOutputDirectory,
                                              const CaptureStartOptions& aOptions)
{
    if (!IsLikelyProcessPointer(aViewport))
    {
        return false;
    }

    auto& captureParameters = aViewport->captureParameters;
    captureParameters.outputDirectoryName = RED4ext::CString(aOutputDirectory.name.c_str());
    captureParameters.outputDirectoryNameSuffix = RED4ext::CString("");
    captureParameters.initialFrameNumber = 0;
    captureParameters.outputDirectoryIndex = aOutputDirectory.index;
    captureParameters.saveFormat = static_cast<RED4ext::ESaveFormat>(aSaveFormat);
    if (aOptions.recordingFPS > 0)
    {
        captureParameters.recordingFPS = static_cast<std::uint32_t>(aOptions.recordingFPS);
    }
    const auto wroteFrameCounter = TryWriteU32(aViewport, 0x118, 0);
    LogInfo("Configured capture parameters: outputDirectoryName=%s outputDirectoryIndex=%u saveFormat=%d fps=%u resetFrameCounter=%s",
            aOutputDirectory.name.c_str(),
            aOutputDirectory.index,
            aSaveFormat,
            captureParameters.recordingFPS,
            wroteFrameCounter ? "ok" : "failed");
    return wroteFrameCounter;
}

bool InvokeToggleContinuousScreenshot(const bool aEnable, const std::int32_t aSaveFormat)
{
    auto* handler = GetToggleContinuousScreenshotHandler();
    if (!handler)
    {
        LogError("ToggleContinuousScreenshot handler was null.");
        return false;
    }

    ToggleContinuousScreenshotPayload payload{};
    payload.enable = aEnable;
    payload.saveFormat = aSaveFormat;

#if defined(_MSC_VER)
    __try
    {
        handler(&payload);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        LogError("ToggleContinuousScreenshot handler raised an SEH exception.");
        return false;
    }
#else
    handler(&payload);
#endif

    LogInfo("ToggleContinuousScreenshot(%s, saveFormat=%d) returned. active=%s takeIndex=%d",
            aEnable ? "true" : "false",
            aSaveFormat,
            GetContinuousScreenshotObject() ? "true" : "false",
            GetContinuousScreenshotTakeIndex());
    return true;
}

} // namespace RedFrame
