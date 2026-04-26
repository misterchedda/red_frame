#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Memory/Allocators.hpp>
#include <RED4ext/Relocation.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/EEnvManagerModifier.hpp>
#include <RED4ext/Scripting/Natives/Generated/ESaveFormat.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/PhotoModeSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/rend/CaptureParameters.hpp>
#include <RED4ext/Scripting/Natives/Generated/rend/ResolutionMultiplier.hpp>
#include <RED4ext/Scripting/Natives/Generated/rend/ScreenshotMode.hpp>
#include <RED4ext/Scripting/Natives/Generated/rend/SingleScreenShotData.hpp>
#include <RED4ext/Scripting/Natives/Generated/rend/dim/EPreset.hpp>
#include <RED4ext/Scripting/Utils.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

namespace RedFrame
{
inline constexpr auto kProjectName = L"RedFrame";

inline constexpr std::uintptr_t kToggleContinuousScreenshotHandlerRva = 0x291987C;
inline constexpr std::uintptr_t kContinuousScreenshotObjectRva = 0x48FF9C0;
inline constexpr std::uintptr_t kContinuousScreenshotTakeIndexRva = 0x49227AC;
inline constexpr std::uintptr_t kGetConfigManagerRva = 0x20458C8;
inline constexpr std::uintptr_t kHashConfigStringRva = 0x60A86C;
inline constexpr std::uintptr_t kSetConfigVarRva = 0x29F9E74;
inline constexpr std::uintptr_t kPhotoModeSimpleStartCaptureRva = 0x29A5220;
inline constexpr std::uintptr_t kPhotoModeCaptureObjectOffset = 0x320;
inline constexpr std::uintptr_t kAkRegisterCaptureCallbackRva = 0x1ACB970;
inline constexpr std::uintptr_t kAkUnregisterCaptureCallbackRva = 0x1AD1830;
inline constexpr std::uintptr_t kAkGetSampleRateRva = 0x1AC6D20;
inline constexpr std::uintptr_t kRendererManagerPtrRva = 0x3427C00;
inline constexpr std::uintptr_t kScreenshotCommandQueueSubmitRva = 0x1D51300;
inline constexpr std::uintptr_t kScreenshotCommandExecuteRva = 0x1D53070;
inline constexpr std::uintptr_t kImageSavingJobExecute80Rva = 0x290F214;
inline constexpr std::uintptr_t kImageSavingJobExecute68Rva = 0x290F2FC;
inline constexpr std::uintptr_t kTaskQueueSubmitRva = 0x14299C;
inline constexpr std::uintptr_t kScreenshotWriterDispatchRva = 0x290F4EC;
inline constexpr std::uintptr_t kImageSavingCompletionNotifyRva = 0x290ECEC;

inline constexpr std::int32_t kCaptureBackendEngineViewport = 1;
inline constexpr std::int32_t kCaptureErrorNone = 0;
inline constexpr std::int32_t kCaptureErrorAlreadyActive = 1;
inline constexpr std::int32_t kCaptureErrorToggleFailed = 3;
inline constexpr std::int32_t kCaptureErrorViewportUnavailable = 4;
inline constexpr std::int32_t kCaptureErrorScreenshotFailed = 5;
inline constexpr std::int32_t kCaptureErrorInvalidPath = 6;
inline constexpr std::int32_t kCaptureErrorInvalidOptions = 7;
inline constexpr std::int32_t kFrameCaptureSaveFormat = static_cast<std::int32_t>(RED4ext::ESaveFormat::SF_PNG);
inline constexpr std::uint32_t kAudioCaptureSampleRate = 48000;
inline constexpr std::uint32_t kAudioCaptureMaxChannels = 8;
inline constexpr std::uint64_t kAudioCaptureDefaultOutputDeviceId = 0;
inline constexpr std::int32_t kScreenshotRequestInvalid = 0;
inline constexpr std::int32_t kScreenshotRequestQueued = 1;
inline constexpr std::int32_t kScreenshotRequestWriting = 2;
inline constexpr std::int32_t kScreenshotRequestComplete = 3;
inline constexpr std::int32_t kScreenshotRequestFailed = 4;
inline constexpr std::int32_t kScreenshotRequestTimeout = 5;
inline constexpr auto kScreenshotRequestTimeoutSeconds = 30;
inline constexpr auto kScreenshotRequestStableMilliseconds = 250;
inline constexpr auto kScreenshotRequestOutputDiscoverySeconds = 30;

using Clock = std::chrono::steady_clock;
using ToggleContinuousScreenshotHandler_t = void (*)(void*);
using GetConfigManager_t = std::uintptr_t (*)();
using HashConfigString_t = std::uint32_t (*)(const char*);
using SetConfigVar_t = void (*)(std::uintptr_t, void*, void*, void*);
using PhotoModeSimpleStartCapture_t = void (*)(void*, void*, bool);
using AkCaptureCallback_t = void (*)(void* aAudioBuffer, std::uint64_t aOutputDeviceId, void* aCookie);
using AkRegisterCaptureCallback_t = std::int32_t (*)(AkCaptureCallback_t aCallback, std::uint64_t aOutputDeviceId, void* aCookie);
using AkUnregisterCaptureCallback_t = std::int32_t (*)(AkCaptureCallback_t aCallback, std::uint64_t aOutputDeviceId, void* aCookie);
using AkGetSampleRate_t = std::uint32_t (*)();

struct AutoRunConfig
{
    bool enabled = false;
    float startDelaySeconds = 8.0f;
    float durationSeconds = 10.0f;
    bool includeAudio = false;
    std::int32_t recordingFPS = 0;
    float timeDilation = -1.0f;
    bool closeGame = false;
    bool setCaptureSequentialFrames = false;
    bool captureSequentialFramesValue = true;
    bool photoModeCapture = false;
    bool engineFrameCapture = false;
    bool screenshotMatrix = false;
    bool nativeRunningStateTrigger = false;
    bool persistentCaptureProbe = false;
    bool probeWriterVideoRoot = false;
    bool probeVideoRootScreenshot = false;
    bool probeAudioCaptureCallback = false;
    bool probeAudioSelfRegister = false;
    bool probeScreenshotOutputSubmit = false;
    std::int32_t screenshotMatrixCase = -1;
    std::int32_t probeScreenshotTailMode = -1;
    bool probeScreenshotTailF3 = false;
    std::int32_t probeScreenshotTailF4 = 0;
};

struct PersistentCaptureProbeState
{
    bool active = false;
    void* viewport = nullptr;
    bool previousStateSaved = false;
    std::uint32_t previousPersistentState = 0;
    std::uint32_t previousFrameCounter = 0;
    std::uint32_t previousInitialFrameNumber = 0;
    std::uint32_t previousOutputDirectoryIndex = 0;
    std::uint32_t previousRecordingFPS = 0;
    RED4ext::Point previousCustomResolution{};
    RED4ext::rend::ResolutionMultiplier previousResolutionMultiplier = RED4ext::rend::ResolutionMultiplier::X1;
    std::uint32_t previousSaveFormat = 2;
    RED4ext::CString previousOutputDirectoryName;
    RED4ext::CString previousOutputDirectoryNameSuffix;
    bool previousEnable = false;
    bool previousVideoRecordingMode = false;
    bool previousAudioRecordingMode = false;
    std::uint32_t previousWriterRootMode = 0;
};

struct CaptureStartOptions
{
    bool includeAudio = false;
    std::int32_t recordingFPS = 0;
};

struct CaptureSessionState
{
    bool active = false;
    std::int32_t backend = 0;
    bool includeAudio = false;
    std::int32_t recordingFPS = 0;
    std::int32_t saveFormat = kFrameCaptureSaveFormat;
    std::int32_t lastError = kCaptureErrorNone;
    std::uint32_t initialFrameCounter = 0;
    std::uint32_t lastFrameCounter = 0;
    std::uint32_t outputDirectoryIndex = 0;
    std::string outputDirectoryName;
    std::filesystem::file_time_type sourceFrameCutoff{};
};

struct ConfigVarKey
{
    const char* text = nullptr;
    std::uint32_t hash = 0;
    std::uint32_t pad0C = 0;
};
static_assert(sizeof(ConfigVarKey) == 0x10);

struct ToggleContinuousScreenshotPayload
{
    std::array<std::uint8_t, 0x20> pad00{};
    bool enable = false;
    std::array<std::uint8_t, 3> pad21{};
    std::int32_t saveFormat = 2;
};
static_assert(sizeof(ToggleContinuousScreenshotPayload) == 0x28);
static_assert(offsetof(ToggleContinuousScreenshotPayload, enable) == 0x20);
static_assert(offsetof(ToggleContinuousScreenshotPayload, saveFormat) == 0x24);

struct EngineViewportWindowInfo
{
    std::uint64_t unk00 = 0;
    std::uint32_t windowWidth = 0;
    std::uint32_t windowHeight = 0;
};
static_assert(sizeof(EngineViewportWindowInfo) == 0x10);
static_assert(offsetof(EngineViewportWindowInfo, windowWidth) == 0x08);
static_assert(offsetof(EngineViewportWindowInfo, windowHeight) == 0x0C);

struct EngineViewportProbe
{
    std::uint64_t unk00 = 0;
    EngineViewportWindowInfo* windowInfo = nullptr;
    std::uint64_t unk10 = 0;
    std::uint32_t unk18 = 0;
    bool unk1C = false;
    std::uint8_t unk1D[0x20 - 0x1D]{};
    std::uint64_t unk20 = 0;
    std::uint8_t unk28[0x80 - 0x28]{};
    std::uint64_t unk80 = 0;
    std::uint8_t unk88[0xD0 - 0x88]{};
    RED4ext::rend::CaptureParameters captureParameters;
    std::uint8_t unk180[0x190 - 0x180]{};
    std::uint8_t singleScreenShotData[0xF8]{};
    std::uint8_t unk288[0x2C0 - 0x288]{};
};
static_assert(sizeof(EngineViewportProbe) == 0x2C0);
static_assert(offsetof(EngineViewportProbe, windowInfo) == 0x08);
static_assert(offsetof(EngineViewportProbe, captureParameters) == 0xD0);
static_assert(offsetof(EngineViewportProbe, singleScreenShotData) == 0x190);

struct NativeSingleScreenShotData : RED4ext::rend::SingleScreenShotData
{
    RED4ext::CClass* GetNativeType() override;
};

struct ScreenshotRequest
{
    std::int32_t id = 0;
    std::int32_t status = kScreenshotRequestQueued;
    std::int32_t error = kCaptureErrorNone;
    std::int32_t expectedOutputCount = 1;
    bool listenerDispatched = false;
    std::filesystem::path outputPath;
    std::vector<std::filesystem::path> outputPaths;
    Clock::time_point queuedAt{};
    std::filesystem::file_time_type queuedFileTime{};
    Clock::time_point lastObservedAt{};
    Clock::time_point completedAt{};
    std::uintmax_t observedTotalSize = 0;
    std::filesystem::file_time_type observedLatestWriteTime{};
    std::unique_ptr<NativeSingleScreenShotData> data;
};

struct ScreenshotListener
{
    std::int32_t id = 0;
    RED4ext::WeakHandle<RED4ext::IScriptable> target;
    RED4ext::CName functionName;
};

struct AkChannelConfigProbe
{
    std::uint32_t fullConfig = 0;
    std::uint32_t GetNumChannels() const;
    std::uint32_t GetConfigType() const;
    std::uint32_t GetChannelMask() const;
};
static_assert(sizeof(AkChannelConfigProbe) == 0x4);

struct AkAudioBufferProbe
{
    void* data = nullptr;
    AkChannelConfigProbe channelConfig;
    std::int32_t state = 0;
    std::uint16_t frameWord10 = 0;
    std::uint16_t frameWord12 = 0;
    std::uint32_t tailWord14 = 0;
};
static_assert(sizeof(AkAudioBufferProbe) == 0x18);
static_assert(offsetof(AkAudioBufferProbe, data) == 0x0);
static_assert(offsetof(AkAudioBufferProbe, channelConfig) == 0x8);
static_assert(offsetof(AkAudioBufferProbe, state) == 0xC);
static_assert(offsetof(AkAudioBufferProbe, frameWord10) == 0x10);
static_assert(offsetof(AkAudioBufferProbe, frameWord12) == 0x12);
static_assert(offsetof(AkAudioBufferProbe, tailWord14) == 0x14);

struct AudioSidecarCaptureState
{
    std::mutex mutex;
    bool registered = false;
    bool writeSidecar = false;
    std::filesystem::path outputPath;
    std::uint32_t channels = 0;
    std::uint32_t sampleRate = kAudioCaptureSampleRate;
    std::uint32_t callbacks = 0;
    std::uint64_t frames = 0;
    std::uint32_t skippedBuffers = 0;
    std::uint64_t lastOutputDeviceId = 0;
    std::vector<float> samples;
};

struct RedFrameScreenshotAPI : RED4ext::IScriptable { RED4ext::CClass* GetNativeType() override; };
struct RedFrameAudioAPI : RED4ext::IScriptable { RED4ext::CClass* GetNativeType() override; };
struct RedFrameDebugAPI : RED4ext::IScriptable { RED4ext::CClass* GetNativeType() override; };

extern RED4ext::v1::PluginHandle g_pluginHandle;
extern const RED4ext::v1::Sdk* g_sdk;
extern std::int32_t g_screenshotIndex;
extern std::int32_t g_captureSessionIndex;
extern std::int32_t g_screenshotRequestIndex;
extern AutoRunConfig g_autoRunConfig;
extern bool g_nativeAutoRunActive;
extern bool g_nativeAutoRunStarted;
extern bool g_nativeAutoRunStopped;
extern bool g_nativeAutoRunCloseScheduled;
extern Clock::time_point g_nativeAutoRunStartDue;
extern Clock::time_point g_nativeAutoRunStopDue;
extern Clock::time_point g_nativeAutoRunCloseDue;
extern std::thread g_captureOutputFinalizeThread;
extern std::atomic_bool g_captureOutputFinalizeCancel;
extern PersistentCaptureProbeState g_persistentCaptureProbeState;
extern CaptureSessionState g_captureSession;
extern std::int32_t g_screenshotLastError;
extern std::int32_t g_audioLastError;
extern std::vector<std::unique_ptr<NativeSingleScreenShotData>> g_probeScreenshotRequests;
extern std::vector<ScreenshotRequest> g_screenshotRequests;
extern std::int32_t g_screenshotListenerIndex;
extern std::vector<ScreenshotListener> g_screenshotListeners;
extern std::mutex g_screenshotClosedOutputsMutex;
extern std::vector<std::filesystem::path> g_screenshotClosedOutputs;
extern AkRegisterCaptureCallback_t g_akRegisterCaptureCallbackOriginal;
extern AkUnregisterCaptureCallback_t g_akUnregisterCaptureCallbackOriginal;
extern bool g_audioCaptureCallbackProbeAttached;
extern bool g_screenshotOutputSubmitProbeAttached;
extern void* g_screenshotOutputSubmitB8Target;
extern void* g_screenshotOutputSubmitC0Target;
extern std::atomic_uint32_t g_screenshotOutputSubmitCalls;
extern bool g_imageSavingJobProbeAttached;
extern void* g_imageSavingJobExecute80Target;
extern void* g_imageSavingJobExecute68Target;
extern void* g_taskQueueSubmitTarget;
extern void* g_screenshotWriterDispatchTarget;
extern void* g_imageSavingCompletionNotifyTarget;
extern void* g_screenshotCommandQueueSubmitTarget;
extern void* g_screenshotCommandExecuteTarget;
extern std::atomic_uint32_t g_imageSavingJobExecuteCalls;
extern std::atomic_uint32_t g_imageSavingJobSubmitCalls;
extern std::atomic_uint32_t g_screenshotWriterDispatchCalls;
extern std::atomic_uint32_t g_imageSavingCompletionNotifyCalls;
extern std::atomic_uint32_t g_screenshotCommandQueueSubmitCalls;
extern std::atomic_uint32_t g_screenshotCommandExecuteCalls;
extern std::atomic_uint32_t g_akRegisterCaptureCallbackCalls;
extern std::atomic_uint32_t g_akUnregisterCaptureCallbackCalls;
extern std::atomic_bool g_audioSelfRegisterProbeActive;
extern std::atomic_uint32_t g_audioSelfRegisterProbeCallbacks;
extern std::atomic<std::uint64_t> g_audioSelfRegisterProbeLastDeviceId;
extern void* g_audioSelfRegisterProbeCookie;
extern AudioSidecarCaptureState g_audioSidecarCaptureState;
extern RED4ext::TTypedClass<RedFrameScreenshotAPI> g_screenshotApiClass;
extern RED4ext::TTypedClass<RedFrameAudioAPI> g_audioApiClass;
extern RED4ext::TTypedClass<RedFrameDebugAPI> g_debugApiClass;
extern RED4ext::v1::GameState g_runningState;

template<typename... Args>
void LogInfo(const char* aFormat, Args... aArgs)
{
    if (g_sdk && g_sdk->logger && g_sdk->logger->InfoF)
    {
        g_sdk->logger->InfoF(g_pluginHandle, aFormat, aArgs...);
    }
}

template<typename... Args>
void LogWarn(const char* aFormat, Args... aArgs)
{
    if (g_sdk && g_sdk->logger && g_sdk->logger->WarnF)
    {
        g_sdk->logger->WarnF(g_pluginHandle, aFormat, aArgs...);
    }
}

template<typename... Args>
void LogError(const char* aFormat, Args... aArgs)
{
    if (g_sdk && g_sdk->logger && g_sdk->logger->ErrorF)
    {
        g_sdk->logger->ErrorF(g_pluginHandle, aFormat, aArgs...);
    }
}

bool IsAudioSidecarCaptureActive();
AkRegisterCaptureCallback_t GetAkRegisterCaptureCallback();
AkUnregisterCaptureCallback_t GetAkUnregisterCaptureCallback();
AkGetSampleRate_t GetAkGetSampleRate();
std::int32_t AkRegisterCaptureCallbackHook(AkCaptureCallback_t aCallback, std::uint64_t aOutputDeviceId, void* aCookie);
std::int32_t AkUnregisterCaptureCallbackHook(AkCaptureCallback_t aCallback, std::uint64_t aOutputDeviceId, void* aCookie);
void AttachAudioCaptureCallbackProbe();
void DetachAudioCaptureCallbackProbe();
void AttachScreenshotOutputSubmitProbe();
void DetachScreenshotOutputSubmitProbe();
void AttachScreenshotFileProbe();
void DetachScreenshotFileProbe();
void AttachImageSavingJobProbe();
void DetachImageSavingJobProbe();
void RecordClosedScreenshotOutput(const std::filesystem::path& aOutputPath);
void RedFrameAudioCaptureCallback(void* aAudioBuffer, std::uint64_t aOutputDeviceId, void* aCookie);
bool WriteFloatWavFile(const std::filesystem::path& aPath, const std::vector<float>& aSamples, std::uint32_t aChannels, std::uint32_t aSampleRate);
bool StartAudioSidecarCapture(const std::filesystem::path& aOutputPath, bool aWriteSidecar);
bool StopAudioSidecarCapture();

std::string TrimCopy(std::string_view aValue);
std::optional<std::string> ReadEnvironmentVariable(const char* aName);
bool ParseEnvironmentBool(std::string_view aValue, bool aFallback);
float ParseEnvironmentFloat(std::string_view aValue, float aFallback);
std::int32_t ParseEnvironmentInt32(std::string_view aValue, std::int32_t aFallback);
std::filesystem::path GetGameScreenshotsDirectory();
std::filesystem::path GetRedFrameOutputDirectory();
std::string ToLowerCopy(std::string aValue);
bool IsReservedWindowsDeviceName(const std::string& aName);
bool IsSafeRelativePathComponent(const std::filesystem::path& aComponent);
std::optional<std::filesystem::path> ResolvePublicScreenshotPath(const RED4ext::CString& aRequestedPath);
std::optional<std::filesystem::path> ResolvePublicScreenshotPath(const RED4ext::CString& aRequestedPath, RED4ext::ESaveFormat aSaveFormat);
std::optional<std::filesystem::path> ResolvePublicAdvancedScreenshotPath(const RED4ext::CString& aRequestedPath);
std::optional<std::filesystem::path> ResolvePublicAudioPath(const RED4ext::CString& aRequestedPath);

void FinalizeCaptureOutputFolder(std::string aOutputDirectoryName, std::filesystem::file_time_type aSourceFrameCutoff);
void StopCaptureOutputFinalizer();
void WaitCaptureOutputFinalizer();
AutoRunConfig LoadAutoRunConfigFromEnvironment();

ToggleContinuousScreenshotHandler_t GetToggleContinuousScreenshotHandler();
void* GetContinuousScreenshotObject();
std::int32_t GetContinuousScreenshotTakeIndex();
GetConfigManager_t GetConfigManager();
HashConfigString_t GetHashConfigString();
SetConfigVar_t GetSetConfigVar();
PhotoModeSimpleStartCapture_t GetPhotoModeSimpleStartCapture();
void* GetRendererManager();
void* GetRendererOutputObject();
void* GetPhotoModeSystem();
void* GetPhotoModeCaptureObject(void* aPhotoModeSystem);
bool IsLikelyProcessPointer(const void* aPointer);
const char* GetTypeNameOrUnknown(RED4ext::rtti::IType* aType);
RED4ext::rtti::IType* TryGetEngineNativeType(RED4ext::CBaseEngine* aEngine);
bool TryReadPointer(const void* aBase, std::size_t aOffset, std::uintptr_t& aOut);
bool TryReadU32(const void* aBase, std::size_t aOffset, std::uint32_t& aOut);
bool TryReadBool(const void* aBase, std::size_t aOffset, bool& aOut);
bool TryWriteU32(void* aBase, std::size_t aOffset, std::uint32_t aValue);
bool TryWriteBool(void* aBase, std::size_t aOffset, bool aValue);
bool TryInvokePhotoModeSimpleStartCapture(void* aPhotoModeSystem);
bool InvokePhotoModeDefaultCapture();
bool InvokeEngineDefaultFrameCapture();
EngineViewportProbe* GetEngineViewportProbe();
bool ReadEngineViewportFrameCounter(std::uint32_t& aOut);
std::uint32_t RefreshCaptureFrameCounter();
void LogPersistentCaptureProbeState(const char* aLabel, EngineViewportProbe* aViewport);
bool SavePersistentCaptureProbeState(EngineViewportProbe* aViewport);
bool StartPersistentCaptureProbe(bool aForce, bool aIncludeAudio);
void StopPersistentCaptureProbe();
bool TryGetConfigManager(std::uintptr_t* aOut);
bool TrySetConfigVar(std::uintptr_t aManager, void* aGroupKey, void* aNameKey, void* aValue);
bool SetConfigVarString(const char* aGroup, const char* aName, const char* aValue);
bool SetCaptureSequentialFrames(bool aEnabled);
bool ValidateCaptureStartOptions(const CaptureStartOptions& aOptions);
bool ConfigureEngineViewportCaptureParameters(EngineViewportProbe* aViewport, std::int32_t aSaveFormat, const struct CaptureOutputDirectory& aOutputDirectory, const CaptureStartOptions& aOptions);
bool InvokeToggleContinuousScreenshot(bool aEnable, std::int32_t aSaveFormat);

struct CaptureOutputDirectory
{
    std::string name;
    std::uint32_t index = 0;
};
CaptureOutputDirectory MakeCaptureOutputDirectory();

std::filesystem::path MakeProbeScreenshotPath();
ScreenshotRequest* FindScreenshotRequest(std::int32_t aRequestId);
void UpdateScreenshotRequestStatus(ScreenshotRequest& aRequest);
void PumpScreenshotRequests();
std::int32_t RegisterScreenshotListener(const RED4ext::Handle<RED4ext::IScriptable>& aTarget, RED4ext::CName aFunctionName);
bool UnregisterScreenshotListener(std::int32_t aListenerId);
void ClearScreenshotListeners();
std::int32_t QueueScreenshot(const std::filesystem::path& aOutputPath, std::int32_t aMode, std::int32_t aSaveFormat, std::int32_t aResolution, std::int32_t aResolutionMultiplier, bool aForceLOD0);
bool QueueDefaultRootScreenshot(bool aVideoRoot);
bool QueueProbeScreenshot();
bool QueueScreenshotMatrix();

bool StartEngineViewportCapture(const CaptureStartOptions& aOptions);
bool StopActiveCapture();
std::int32_t GetCapturedFrameCount();
bool StartCapture(const CaptureStartOptions& aOptions);

void RedFrameStartCapture(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameVideoStart(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameDebugStartFrameDump(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameAudioStart(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameAudioStop(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameCaptureScreenshot(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameStopCapture(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameVideoStop(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameIsCapturing(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameGetCapturedFrameCount(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameGetLastError(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameScreenshotGetLastError(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameScreenshotGetRequestStatus(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameScreenshotGetRequestError(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameScreenshotGetRequestPath(RED4ext::IScriptable*, RED4ext::CStackFrame*, RED4ext::CString*, int64_t);
void RedFrameScreenshotGetRequestPathCount(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameScreenshotGetRequestPathAt(RED4ext::IScriptable*, RED4ext::CStackFrame*, RED4ext::CString*, int64_t);
void RedFrameScreenshotRegisterListener(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameScreenshotUnregisterListener(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameScreenshotPump(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
void RedFrameAudioIsActive(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameAudioGetLastError(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameAutoRunCaptureDiagnosticScreenshot(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameAutoApplyCaptureSequentialFrames(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameAutoRunPhotoModeCapture(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameAutoRunEngineFrameCapture(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameAutoRunScreenshotMatrix(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameHarnessReportScreenshotListener(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
void RedFrameIsAutoRunEnabled(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameGetAutoRunStartDelaySeconds(RED4ext::IScriptable*, RED4ext::CStackFrame*, float*, int64_t);
void RedFrameGetAutoRunDurationSeconds(RED4ext::IScriptable*, RED4ext::CStackFrame*, float*, int64_t);
void RedFrameGetAutoRunIncludeAudio(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameGetAutoRunFps(RED4ext::IScriptable*, RED4ext::CStackFrame*, std::int32_t*, int64_t);
void RedFrameGetAutoRunTimeDilation(RED4ext::IScriptable*, RED4ext::CStackFrame*, float*, int64_t);
void RedFrameShouldAutoCloseGame(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool*, int64_t);
void RedFrameRequestGameClose(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
void StartNativeAutoRun();
void StopNativeAutoRun();
bool RunningStateOnEnter(RED4ext::CGameApplication* aApp);
bool RunningStateOnUpdate(RED4ext::CGameApplication* aApp);
bool RunningStateOnExit(RED4ext::CGameApplication* aApp);
void RegisterTypes();
void PostRegisterTypes();
void RegisterHarnessScriptApi(RED4ext::CRTTISystem* aRtti, RED4ext::CBaseFunction::Flags aFlags);

} // namespace RedFrame
