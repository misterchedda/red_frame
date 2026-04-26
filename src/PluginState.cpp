#include "Internal.hpp"

namespace RedFrame
{
RED4ext::v1::PluginHandle g_pluginHandle{};
const RED4ext::v1::Sdk* g_sdk{};
std::int32_t g_screenshotIndex = 0;
std::int32_t g_captureSessionIndex = 0;
std::int32_t g_screenshotRequestIndex = 0;
AutoRunConfig g_autoRunConfig{};
bool g_nativeAutoRunActive = false;
bool g_nativeAutoRunStarted = false;
bool g_nativeAutoRunStopped = false;
bool g_nativeAutoRunCloseScheduled = false;
Clock::time_point g_nativeAutoRunStartDue{};
Clock::time_point g_nativeAutoRunStopDue{};
Clock::time_point g_nativeAutoRunCloseDue{};
std::thread g_captureOutputFinalizeThread;
std::atomic_bool g_captureOutputFinalizeCancel{false};
PersistentCaptureProbeState g_persistentCaptureProbeState{};
CaptureSessionState g_captureSession{};
std::int32_t g_screenshotLastError = kCaptureErrorNone;
std::int32_t g_audioLastError = kCaptureErrorNone;
std::vector<std::unique_ptr<NativeSingleScreenShotData>> g_probeScreenshotRequests;
std::vector<ScreenshotRequest> g_screenshotRequests;
AkRegisterCaptureCallback_t g_akRegisterCaptureCallbackOriginal = nullptr;
AkUnregisterCaptureCallback_t g_akUnregisterCaptureCallbackOriginal = nullptr;
bool g_audioCaptureCallbackProbeAttached = false;
std::atomic_uint32_t g_akRegisterCaptureCallbackCalls{0};
std::atomic_uint32_t g_akUnregisterCaptureCallbackCalls{0};
std::atomic_bool g_audioSelfRegisterProbeActive{false};
std::atomic_uint32_t g_audioSelfRegisterProbeCallbacks{0};
std::atomic<std::uint64_t> g_audioSelfRegisterProbeLastDeviceId{0};
void* g_audioSelfRegisterProbeCookie = nullptr;
AudioSidecarCaptureState g_audioSidecarCaptureState{};
RED4ext::TTypedClass<RedFrameScreenshotAPI> g_screenshotApiClass("RedFrameScreenshot");
RED4ext::TTypedClass<RedFrameAudioAPI> g_audioApiClass("RedFrameAudio");
RED4ext::TTypedClass<RedFrameDebugAPI> g_debugApiClass("RedFrameDebug");

RED4ext::CClass* NativeSingleScreenShotData::GetNativeType()
{
    return RED4ext::CRTTISystem::Get()->GetClass(RED4ext::rend::SingleScreenShotData::NAME);
}

std::uint32_t AkChannelConfigProbe::GetNumChannels() const
{
    return fullConfig & 0xFFu;
}

std::uint32_t AkChannelConfigProbe::GetConfigType() const
{
    return (fullConfig >> 8u) & 0xFu;
}

std::uint32_t AkChannelConfigProbe::GetChannelMask() const
{
    return (fullConfig >> 12u) & 0xFFFFFu;
}

RED4ext::CClass* RedFrameScreenshotAPI::GetNativeType()
{
    return &g_screenshotApiClass;
}

RED4ext::CClass* RedFrameAudioAPI::GetNativeType()
{
    return &g_audioApiClass;
}

RED4ext::CClass* RedFrameDebugAPI::GetNativeType()
{
    return &g_debugApiClass;
}

bool IsAudioSidecarCaptureActive()
{
    std::lock_guard lock(g_audioSidecarCaptureState.mutex);
    return g_audioSidecarCaptureState.registered;
}
} // namespace RedFrame
