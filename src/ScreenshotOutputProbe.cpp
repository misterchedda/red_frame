#include "Internal.hpp"

#include <Windows.h>

#include <cwctype>

namespace RedFrame
{
namespace
{
using ScreenshotOutputSubmit_t = void (*)(void*,
                                          void*,
                                          void*,
                                          std::uint32_t,
                                          float,
                                          void*,
                                          std::uint32_t);
using ImageSavingJobExecute_t = void (*)(void*, void*);
using TaskQueueSubmit_t = std::uint32_t (*)(void*, void*, std::uint8_t, void*, void*);
using ScreenshotWriterDispatch_t = void (*)(void*, void*, void*, std::int32_t);
using ImageSavingCompletionNotify_t = void (*)(void*, void*, std::uint8_t);
using ScreenshotCommandQueueSubmit_t = void (*)(void*, void*);
using ScreenshotCommandExecute_t = void (*)(void*);
using CreateFileW_t = HANDLE(WINAPI*)(LPCWSTR,
                                      DWORD,
                                      DWORD,
                                      LPSECURITY_ATTRIBUTES,
                                      DWORD,
                                      DWORD,
                                      HANDLE);
using CloseHandle_t = BOOL(WINAPI*)(HANDLE);

ScreenshotOutputSubmit_t s_screenshotOutputSubmitB8Original = nullptr;
ScreenshotOutputSubmit_t s_screenshotOutputSubmitC0Original = nullptr;
ImageSavingJobExecute_t s_imageSavingJobExecute80Original = nullptr;
ImageSavingJobExecute_t s_imageSavingJobExecute68Original = nullptr;
TaskQueueSubmit_t s_taskQueueSubmitOriginal = nullptr;
ScreenshotWriterDispatch_t s_screenshotWriterDispatchOriginal = nullptr;
ImageSavingCompletionNotify_t s_imageSavingCompletionNotifyOriginal = nullptr;
ScreenshotCommandQueueSubmit_t s_screenshotCommandQueueSubmitOriginal = nullptr;
ScreenshotCommandExecute_t s_screenshotCommandExecuteOriginal = nullptr;
CreateFileW_t s_createFileWOriginal = nullptr;
CloseHandle_t s_closeHandleOriginal = nullptr;
void* s_createFileWTarget = nullptr;
void* s_closeHandleTarget = nullptr;
bool s_screenshotFileProbeAttached = false;
std::atomic_uint32_t s_screenshotFileCreateCalls{0};
std::atomic_uint32_t s_screenshotFileCloseCalls{0};
std::mutex s_screenshotFileHandlesMutex;
std::vector<std::pair<HANDLE, std::filesystem::path>> s_screenshotFileHandles;
thread_local bool s_insideScreenshotFileHook = false;

struct ScreenshotFileHookGuard
{
    bool active = false;

    ScreenshotFileHookGuard()
    {
        if (!s_insideScreenshotFileHook)
        {
            s_insideScreenshotFileHook = true;
            active = true;
        }
    }

    ~ScreenshotFileHookGuard()
    {
        if (active)
        {
            s_insideScreenshotFileHook = false;
        }
    }
};

bool TryReadCStringLike(void* aStringLike, char* aBuffer, const std::size_t aBufferSize)
{
    if (!aBuffer || aBufferSize == 0)
    {
        return false;
    }

    aBuffer[0] = '\0';
    if (!IsLikelyProcessPointer(aStringLike))
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
#endif
        auto* stringValue = reinterpret_cast<const RED4ext::CString*>(aStringLike);
        const auto length = stringValue->Length();
        const auto* text = stringValue->c_str();
        if (!text || length == 0 || length >= aBufferSize)
        {
            return false;
        }

        for (std::uint32_t i = 0; i < length; ++i)
        {
            const auto ch = text[i];
            const auto byte = static_cast<unsigned char>(ch);
            if ((byte < 0x20 || byte > 0x7E) && ch != '\t')
            {
                aBuffer[0] = '\0';
                return false;
            }

            aBuffer[i] = ch;
        }

        aBuffer[length] = '\0';
        return true;
#if defined(_MSC_VER)
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        aBuffer[0] = '\0';
        return false;
    }
#endif
}

template<typename T>
bool TryReadProbeValue(void* aBase, const std::ptrdiff_t aOffset, T& aOut)
{
    if (!IsLikelyProcessPointer(aBase))
    {
        return false;
    }

#if defined(_MSC_VER)
    __try
    {
#endif
        aOut = *reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(aBase) + aOffset);
        return true;
#if defined(_MSC_VER)
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#endif
}

struct OutputFileSnapshot
{
    bool hasPath = false;
    bool exists = false;
    std::uintmax_t size = 0;
};

OutputFileSnapshot GetOutputFileSnapshot(const char* aPath)
{
    OutputFileSnapshot snapshot{};
    snapshot.hasPath = aPath && aPath[0] != '\0';
    if (!snapshot.hasPath)
    {
        return snapshot;
    }

    std::error_code ec;
    const std::filesystem::path path(aPath);
    snapshot.exists = std::filesystem::exists(path, ec);
    if (ec || !snapshot.exists)
    {
        return snapshot;
    }

    snapshot.size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        snapshot.size = 0;
    }

    return snapshot;
}

OutputFileSnapshot GetOutputFileSnapshot(const std::filesystem::path& aPath)
{
    OutputFileSnapshot snapshot{};
    snapshot.hasPath = !aPath.empty();
    if (!snapshot.hasPath)
    {
        return snapshot;
    }

    std::error_code ec;
    snapshot.exists = std::filesystem::exists(aPath, ec);
    if (ec || !snapshot.exists)
    {
        return snapshot;
    }

    snapshot.size = std::filesystem::file_size(aPath, ec);
    if (ec)
    {
        snapshot.size = 0;
    }

    return snapshot;
}

std::wstring ToLowerCopy(std::wstring aValue)
{
    std::transform(aValue.begin(), aValue.end(), aValue.begin(), [](const wchar_t aCh) {
        return static_cast<wchar_t>(std::towlower(aCh));
    });

    return aValue;
}

bool IsScreenshotFileProbeCandidate(const wchar_t* aPath)
{
    if (!aPath || aPath[0] == L'\0')
    {
        return false;
    }

    const std::filesystem::path path(aPath);
    const auto lowerPath = ToLowerCopy(path.native());
    const auto lowerExtension = ToLowerCopy(path.extension().native());

    if (lowerExtension != L".png" && lowerExtension != L".exr")
    {
        return false;
    }

    return lowerPath.find(L"\\cyberpunk 2077\\screenshots\\") != std::wstring::npos ||
           lowerPath.find(L"/cyberpunk 2077/screenshots/") != std::wstring::npos;
}

void TrackScreenshotFileHandle(HANDLE aHandle, const std::filesystem::path& aPath)
{
    std::lock_guard lock(s_screenshotFileHandlesMutex);
    s_screenshotFileHandles.emplace_back(aHandle, aPath);
}

std::optional<std::filesystem::path> TakeTrackedScreenshotFileHandle(HANDLE aHandle)
{
    std::lock_guard lock(s_screenshotFileHandlesMutex);
    for (auto it = s_screenshotFileHandles.begin(); it != s_screenshotFileHandles.end(); ++it)
    {
        if (it->first == aHandle)
        {
            auto path = it->second;
            s_screenshotFileHandles.erase(it);
            return path;
        }
    }

    return std::nullopt;
}

HANDLE WINAPI CreateFileWProbeHook(LPCWSTR aFileName,
                                   DWORD aDesiredAccess,
                                   DWORD aShareMode,
                                   LPSECURITY_ATTRIBUTES aSecurityAttributes,
                                   DWORD aCreationDisposition,
                                   DWORD aFlagsAndAttributes,
                                   HANDLE aTemplateFile)
{
    if (!s_createFileWOriginal)
    {
        return INVALID_HANDLE_VALUE;
    }

    if (s_insideScreenshotFileHook)
    {
        return s_createFileWOriginal(aFileName,
                                     aDesiredAccess,
                                     aShareMode,
                                     aSecurityAttributes,
                                     aCreationDisposition,
                                     aFlagsAndAttributes,
                                     aTemplateFile);
    }

    ScreenshotFileHookGuard guard;
    const auto isCandidate = IsScreenshotFileProbeCandidate(aFileName);
    const auto handle = s_createFileWOriginal(aFileName,
                                              aDesiredAccess,
                                              aShareMode,
                                              aSecurityAttributes,
                                              aCreationDisposition,
                                              aFlagsAndAttributes,
                                              aTemplateFile);

    if (isCandidate && handle != INVALID_HANDLE_VALUE && handle != nullptr)
    {
        const std::filesystem::path path(aFileName);
        TrackScreenshotFileHandle(handle, path);

        const auto callCount = ++s_screenshotFileCreateCalls;
        if (g_autoRunConfig.probeScreenshotOutputSubmit)
        {
            const auto snapshot = GetOutputFileSnapshot(path);
            LogWarn("Screenshot file CreateFileW probe: count=%u handle=%p access=0x%08X share=0x%08X disposition=%u flags=0x%08X path='%s' exists=%s size=%llu activeRequests=%zu",
                    callCount,
                    handle,
                    static_cast<unsigned int>(aDesiredAccess),
                    static_cast<unsigned int>(aShareMode),
                    static_cast<unsigned int>(aCreationDisposition),
                    static_cast<unsigned int>(aFlagsAndAttributes),
                    path.string().c_str(),
                    snapshot.exists ? "true" : "false",
                    static_cast<unsigned long long>(snapshot.size),
                    g_screenshotRequests.size());
        }
    }

    return handle;
}

BOOL WINAPI CloseHandleProbeHook(HANDLE aHandle)
{
    if (!s_closeHandleOriginal)
    {
        return FALSE;
    }

    if (s_insideScreenshotFileHook)
    {
        return s_closeHandleOriginal(aHandle);
    }

    ScreenshotFileHookGuard guard;
    const auto trackedPath = TakeTrackedScreenshotFileHandle(aHandle);
    const auto result = s_closeHandleOriginal(aHandle);

    if (trackedPath)
    {
        RecordClosedScreenshotOutput(*trackedPath);
        const auto callCount = ++s_screenshotFileCloseCalls;
        if (g_autoRunConfig.probeScreenshotOutputSubmit)
        {
            const auto snapshot = GetOutputFileSnapshot(*trackedPath);
            LogWarn("Screenshot file CloseHandle probe: count=%u handle=%p ok=%s path='%s' exists=%s size=%llu activeRequests=%zu",
                    callCount,
                    aHandle,
                    result ? "true" : "false",
                    trackedPath->string().c_str(),
                    snapshot.exists ? "true" : "false",
                    static_cast<unsigned long long>(snapshot.size),
                    g_screenshotRequests.size());
        }
    }

    return result;
}

void LogScreenshotOutputSubmitBefore(const char* aWhich,
                                     void* aThis,
                                     void* aRenderState,
                                     void* aOutputPathLike,
                                     const std::uint32_t aArg4,
                                     const float aArg5,
                                     void* aOutputDescriptor,
                                     const std::uint32_t aArg7,
                                     char* aPath,
                                     const std::size_t aPathSize,
                                     OutputFileSnapshot& aBeforeSnapshot)
{
    const auto callCount = ++g_screenshotOutputSubmitCalls;
    TryReadCStringLike(aOutputPathLike, aPath, aPathSize);
    aBeforeSnapshot = GetOutputFileSnapshot(aPath);

    LogWarn("Screenshot output submit probe %s before: count=%u this=%p renderState=%p pathLike=%p path='%s' exists=%s size=%llu arg4=%u arg5=%.6f descriptor=%p arg7=%u activeRequests=%zu",
            aWhich,
            callCount,
            aThis,
            aRenderState,
            aOutputPathLike,
            aPath,
            aBeforeSnapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(aBeforeSnapshot.size),
            aArg4,
            aArg5,
            aOutputDescriptor,
            aArg7,
            g_screenshotRequests.size());
}

void LogScreenshotOutputSubmitAfter(const char* aWhich,
                                    const char* aPath,
                                    const OutputFileSnapshot& aBeforeSnapshot)
{
    const auto afterSnapshot = GetOutputFileSnapshot(aPath);
    if (!afterSnapshot.hasPath)
    {
        return;
    }

    LogWarn("Screenshot output submit probe %s after: path='%s' beforeExists=%s beforeSize=%llu afterExists=%s afterSize=%llu",
            aWhich,
            aPath,
            aBeforeSnapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(aBeforeSnapshot.size),
            afterSnapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(afterSnapshot.size));
}

void LogImageSavingJobExecuteBefore(const char* aWhich,
                                    void* aJobPayload,
                                    void* aContext,
                                    char* aPath,
                                    const std::size_t aPathSize,
                                    OutputFileSnapshot& aBeforeSnapshot)
{
    const auto callCount = ++g_imageSavingJobExecuteCalls;
    auto* pathLike = reinterpret_cast<std::uint8_t*>(aJobPayload) + 0x10;
    TryReadCStringLike(pathLike, aPath, aPathSize);
    aBeforeSnapshot = GetOutputFileSnapshot(aPath);

    LogWarn("ImageSaving job probe %s before: count=%u payload=%p context=%p pathLike=%p path='%s' exists=%s size=%llu activeRequests=%zu",
            aWhich,
            callCount,
            aJobPayload,
            aContext,
            pathLike,
            aPath,
            aBeforeSnapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(aBeforeSnapshot.size),
            g_screenshotRequests.size());
}

void LogImageSavingJobExecuteAfter(const char* aWhich,
                                   void* aJobPayload,
                                   const char* aPath,
                                   const OutputFileSnapshot& aBeforeSnapshot)
{
    const auto afterSnapshot = GetOutputFileSnapshot(aPath);
    if (!afterSnapshot.hasPath)
    {
        LogWarn("ImageSaving job probe %s after: payload=%p no decoded path",
                aWhich,
                aJobPayload);
        return;
    }

    LogWarn("ImageSaving job probe %s after: payload=%p path='%s' beforeExists=%s beforeSize=%llu afterExists=%s afterSize=%llu",
            aWhich,
            aJobPayload,
            aPath,
            aBeforeSnapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(aBeforeSnapshot.size),
            afterSnapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(afterSnapshot.size));
}

void ImageSavingJobExecute80Hook(void* aJobPayload, void* aContext)
{
    char path[4096]{};
    OutputFileSnapshot beforeSnapshot{};
    LogImageSavingJobExecuteBefore("0x80", aJobPayload, aContext, path, std::size(path), beforeSnapshot);

    if (s_imageSavingJobExecute80Original)
    {
        s_imageSavingJobExecute80Original(aJobPayload, aContext);
    }

    LogImageSavingJobExecuteAfter("0x80", aJobPayload, path, beforeSnapshot);
}

void ImageSavingJobExecute68Hook(void* aJobPayload, void* aContext)
{
    char path[4096]{};
    OutputFileSnapshot beforeSnapshot{};
    LogImageSavingJobExecuteBefore("0x68", aJobPayload, aContext, path, std::size(path), beforeSnapshot);

    if (s_imageSavingJobExecute68Original)
    {
        s_imageSavingJobExecute68Original(aJobPayload, aContext);
    }

    LogImageSavingJobExecuteAfter("0x68", aJobPayload, path, beforeSnapshot);
}

void LogImageSavingJobSubmit(void* aQueue,
                             void* aJobDescriptor,
                             const std::uint8_t aPriority,
                             void* aParam4,
                             void* aParam5)
{
    if (!IsLikelyProcessPointer(aJobDescriptor))
    {
        return;
    }

    std::uintptr_t entryPoint = 0;
    std::uintptr_t payload = 0;
    if (!TryReadPointer(aJobDescriptor, 0x0, entryPoint) ||
        !TryReadPointer(aJobDescriptor, 0x8, payload))
    {
        return;
    }

    if (entryPoint != kImageSavingJobExecute80Rva + RED4ext::RelocBase::GetImageBase() &&
        entryPoint != kImageSavingJobExecute68Rva + RED4ext::RelocBase::GetImageBase())
    {
        return;
    }

    char path[4096]{};
    auto* pathLike = reinterpret_cast<std::uint8_t*>(payload) + 0x10;
    TryReadCStringLike(pathLike, path, std::size(path));
    const auto snapshot = GetOutputFileSnapshot(path);
    const auto callCount = ++g_imageSavingJobSubmitCalls;
    LogWarn("ImageSaving job submit probe: count=%u queue=%p descriptor=%p entry=%p payload=%p priority=%u param4=%p param5=%p pathLike=%p path='%s' exists=%s size=%llu activeRequests=%zu",
            callCount,
            aQueue,
            aJobDescriptor,
            reinterpret_cast<void*>(entryPoint),
            reinterpret_cast<void*>(payload),
            static_cast<unsigned int>(aPriority),
            aParam4,
            aParam5,
            pathLike,
            path,
            snapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(snapshot.size),
            g_screenshotRequests.size());
}

std::uint32_t TaskQueueSubmitHook(void* aQueue, void* aJobDescriptor, std::uint8_t aPriority, void* aParam4, void* aParam5)
{
    LogImageSavingJobSubmit(aQueue, aJobDescriptor, aPriority, aParam4, aParam5);

    if (s_taskQueueSubmitOriginal)
    {
        return s_taskQueueSubmitOriginal(aQueue, aJobDescriptor, aPriority, aParam4, aParam5);
    }

    return 0;
}

void LogScreenshotWriterDispatch(void* aRendererManager, void* aRenderState, void* aOutputDescriptor, const std::int32_t aParam4)
{
    const auto callCount = ++g_screenshotWriterDispatchCalls;
    char basePath[4096]{};
    char suffixPath[4096]{};
    auto* basePathLike = reinterpret_cast<std::uint8_t*>(aOutputDescriptor) + 0x10;
    auto* suffixPathLike = reinterpret_cast<std::uint8_t*>(aOutputDescriptor) + 0x68;
    TryReadCStringLike(basePathLike, basePath, std::size(basePath));
    TryReadCStringLike(suffixPathLike, suffixPath, std::size(suffixPath));

    std::uintptr_t writer70 = 0;
    std::uintptr_t writer80 = 0;
    std::uintptr_t writer90 = 0;
    std::uintptr_t writerA0 = 0;
    std::uintptr_t writerB0 = 0;
    std::uintptr_t writerC0 = 0;
    std::uintptr_t writerD0 = 0;
    std::uintptr_t writer100 = 0;
    TryReadPointer(aRenderState, 0x70, writer70);
    TryReadPointer(aRenderState, 0x80, writer80);
    TryReadPointer(aRenderState, 0x90, writer90);
    TryReadPointer(aRenderState, 0xA0, writerA0);
    TryReadPointer(aRenderState, 0xB0, writerB0);
    TryReadPointer(aRenderState, 0xC0, writerC0);
    TryReadPointer(aRenderState, 0xD0, writerD0);
    TryReadPointer(aRenderState, 0x100, writer100);

    std::uintptr_t writer80Vftable = 0;
    std::uintptr_t writer80Submit = 0;
    if (writer80 != 0 &&
        TryReadPointer(reinterpret_cast<void*>(writer80), 0x0, writer80Vftable))
    {
        TryReadPointer(reinterpret_cast<void*>(writer80Vftable), 0x10, writer80Submit);
    }

    std::uintptr_t writer90Payload = 0;
    if (writer90 != 0)
    {
        TryReadPointer(reinterpret_cast<void*>(writer90), 0x8, writer90Payload);
    }

    LogWarn("Screenshot writer dispatch probe: count=%u manager=%p renderState=%p descriptor=%p param4=%d base='%s' suffix='%s' w70=%p w80=%p w80vft=%p w80submit=%p w90=%p w90payload=%p wA0=%p wB0=%p wC0=%p wD0=%p w100=%p",
            callCount,
            aRendererManager,
            aRenderState,
            aOutputDescriptor,
            aParam4,
            basePath,
            suffixPath,
            reinterpret_cast<void*>(writer70),
            reinterpret_cast<void*>(writer80),
            reinterpret_cast<void*>(writer80Vftable),
            reinterpret_cast<void*>(writer80Submit),
            reinterpret_cast<void*>(writer90),
            reinterpret_cast<void*>(writer90Payload),
            reinterpret_cast<void*>(writerA0),
            reinterpret_cast<void*>(writerB0),
            reinterpret_cast<void*>(writerC0),
            reinterpret_cast<void*>(writerD0),
            reinterpret_cast<void*>(writer100));
}

void ScreenshotWriterDispatchHook(void* aRendererManager, void* aRenderState, void* aOutputDescriptor, const std::int32_t aParam4)
{
    LogScreenshotWriterDispatch(aRendererManager, aRenderState, aOutputDescriptor, aParam4);

    if (s_screenshotWriterDispatchOriginal)
    {
        s_screenshotWriterDispatchOriginal(aRendererManager, aRenderState, aOutputDescriptor, aParam4);
    }
}

void ImageSavingCompletionNotifyHook(void* aNotifier, void* aPathLike, const std::uint8_t aSuccess)
{
    char path[4096]{};
    TryReadCStringLike(aPathLike, path, std::size(path));
    const auto snapshot = GetOutputFileSnapshot(path);
    const auto callCount = ++g_imageSavingCompletionNotifyCalls;
    LogWarn("ImageSaving completion notify probe: count=%u notifier=%p pathLike=%p path='%s' success=%u exists=%s size=%llu activeRequests=%zu",
            callCount,
            aNotifier,
            aPathLike,
            path,
            static_cast<unsigned int>(aSuccess),
            snapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(snapshot.size),
            g_screenshotRequests.size());

    if (s_imageSavingCompletionNotifyOriginal)
    {
        s_imageSavingCompletionNotifyOriginal(aNotifier, aPathLike, aSuccess);
    }
}

void ScreenshotCommandQueueSubmitHook(void* aQueue, void* aCommand)
{
    const auto callCount = ++g_screenshotCommandQueueSubmitCalls;

    std::uintptr_t vftable = 0;
    std::uintptr_t namePtr = 0;
    std::uintptr_t executeEntry = 0;
    TryReadPointer(aCommand, 0x0, vftable);
    TryReadPointer(aCommand, 0x8, namePtr);
    if (vftable != 0)
    {
        TryReadPointer(reinterpret_cast<void*>(vftable), 0x0, executeEntry);
    }

    const char* name = "<unknown>";
    if (IsLikelyProcessPointer(reinterpret_cast<void*>(namePtr)))
    {
#if defined(_MSC_VER)
        __try
        {
#endif
            name = reinterpret_cast<const char*>(namePtr);
#if defined(_MSC_VER)
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            name = "<fault>";
        }
#endif
    }

    char path[4096]{};
    TryReadCStringLike(reinterpret_cast<std::uint8_t*>(aCommand) + 0x170, path, std::size(path));
    LogWarn("Screenshot command queue submit probe: count=%u queue=%p command=%p vftable=%p execute=%p namePtr=%p name='%s' commandPath='%s'",
            callCount,
            aQueue,
            aCommand,
            reinterpret_cast<void*>(vftable),
            reinterpret_cast<void*>(executeEntry),
            reinterpret_cast<void*>(namePtr),
            name,
            path);

    if (s_screenshotCommandQueueSubmitOriginal)
    {
        s_screenshotCommandQueueSubmitOriginal(aQueue, aCommand);
    }
}

void ScreenshotCommandExecuteHook(void* aCommand)
{
    const auto callCount = ++g_screenshotCommandExecuteCalls;
    char path[4096]{};
    TryReadCStringLike(reinterpret_cast<std::uint8_t*>(aCommand) + 0x170, path, std::size(path));

    std::uint32_t mode = 0;
    std::uint32_t saveFormat = 0;
    std::uint32_t arg2b8 = 0;
    TryReadProbeValue(reinterpret_cast<std::uint8_t*>(aCommand) + 0x190, 0x0, mode);
    TryReadProbeValue(reinterpret_cast<std::uint8_t*>(aCommand) + 0x190, 0x98, saveFormat);
    TryReadProbeValue(aCommand, 0x2B8, arg2b8);

    LogWarn("Screenshot command execute probe before: count=%u command=%p path='%s' mode=%u saveFormat=%u arg2b8=%u",
            callCount,
            aCommand,
            path,
            mode,
            saveFormat,
            arg2b8);

    if (s_screenshotCommandExecuteOriginal)
    {
        s_screenshotCommandExecuteOriginal(aCommand);
    }

    const auto snapshot = GetOutputFileSnapshot(path);
    LogWarn("Screenshot command execute probe after: command=%p path='%s' exists=%s size=%llu",
            aCommand,
            path,
            snapshot.exists ? "true" : "false",
            static_cast<unsigned long long>(snapshot.size));
}

void ScreenshotOutputSubmitB8Hook(void* aThis,
                                  void* aRenderState,
                                  void* aOutputPathLike,
                                  const std::uint32_t aArg4,
                                  const float aArg5,
                                  void* aOutputDescriptor,
                                  const std::uint32_t aArg7)
{
    char path[4096]{};
    OutputFileSnapshot beforeSnapshot{};
    LogScreenshotOutputSubmitBefore("+0xB8",
                                    aThis,
                                    aRenderState,
                                    aOutputPathLike,
                                    aArg4,
                                    aArg5,
                                    aOutputDescriptor,
                                    aArg7,
                                    path,
                                    std::size(path),
                                    beforeSnapshot);

    if (s_screenshotOutputSubmitB8Original)
    {
        s_screenshotOutputSubmitB8Original(aThis, aRenderState, aOutputPathLike, aArg4, aArg5, aOutputDescriptor, aArg7);
    }

    LogScreenshotOutputSubmitAfter("+0xB8", path, beforeSnapshot);
}

void ScreenshotOutputSubmitC0Hook(void* aThis,
                                  void* aRenderState,
                                  void* aOutputPathLike,
                                  const std::uint32_t aArg4,
                                  const float aArg5,
                                  void* aOutputDescriptor,
                                  const std::uint32_t aArg7)
{
    char path[4096]{};
    OutputFileSnapshot beforeSnapshot{};
    LogScreenshotOutputSubmitBefore("+0xC0",
                                    aThis,
                                    aRenderState,
                                    aOutputPathLike,
                                    aArg4,
                                    aArg5,
                                    aOutputDescriptor,
                                    aArg7,
                                    path,
                                    std::size(path),
                                    beforeSnapshot);

    if (s_screenshotOutputSubmitC0Original)
    {
        s_screenshotOutputSubmitC0Original(aThis, aRenderState, aOutputPathLike, aArg4, aArg5, aOutputDescriptor, aArg7);
    }

    LogScreenshotOutputSubmitAfter("+0xC0", path, beforeSnapshot);
}
} // namespace

void AttachScreenshotFileProbe()
{
    if (s_screenshotFileProbeAttached)
    {
        return;
    }

    if (!g_sdk || !g_sdk->hooking || !g_sdk->hooking->Attach)
    {
        LogWarn("Screenshot file probe skipped: RED4ext hooking API unavailable.");
        return;
    }

    const auto kernel32 = GetModuleHandleW(L"kernel32.dll");
    s_createFileWTarget = kernel32 ? reinterpret_cast<void*>(GetProcAddress(kernel32, "CreateFileW")) : nullptr;
    s_closeHandleTarget = kernel32 ? reinterpret_cast<void*>(GetProcAddress(kernel32, "CloseHandle")) : nullptr;

    const auto attachedCreateFileW = s_createFileWTarget
                                         ? g_sdk->hooking->Attach(g_pluginHandle,
                                                                 s_createFileWTarget,
                                                                 reinterpret_cast<void*>(&CreateFileWProbeHook),
                                                                 reinterpret_cast<void**>(&s_createFileWOriginal))
                                         : false;
    const auto attachedCloseHandle = s_closeHandleTarget
                                         ? g_sdk->hooking->Attach(g_pluginHandle,
                                                                 s_closeHandleTarget,
                                                                 reinterpret_cast<void*>(&CloseHandleProbeHook),
                                                                 reinterpret_cast<void**>(&s_closeHandleOriginal))
                                         : false;

    s_screenshotFileProbeAttached = attachedCreateFileW || attachedCloseHandle;
    LogInfo("Screenshot file probe attach: createFileW=%p attached=%s original=%p closeHandle=%p attached=%s original=%p",
            s_createFileWTarget,
            attachedCreateFileW ? "true" : "false",
            reinterpret_cast<void*>(s_createFileWOriginal),
            s_closeHandleTarget,
            attachedCloseHandle ? "true" : "false",
            reinterpret_cast<void*>(s_closeHandleOriginal));
}

void DetachScreenshotFileProbe()
{
    if (!s_screenshotFileProbeAttached || !g_sdk || !g_sdk->hooking || !g_sdk->hooking->Detach)
    {
        return;
    }

    const auto detachedCreateFileW = s_createFileWTarget
                                         ? g_sdk->hooking->Detach(g_pluginHandle, s_createFileWTarget)
                                         : false;
    const auto detachedCloseHandle = s_closeHandleTarget
                                         ? g_sdk->hooking->Detach(g_pluginHandle, s_closeHandleTarget)
                                         : false;

    LogInfo("Screenshot file probe detach: createFileWDetached=%s closeHandleDetached=%s createCalls=%u closeCalls=%u",
            detachedCreateFileW ? "true" : "false",
            detachedCloseHandle ? "true" : "false",
            s_screenshotFileCreateCalls.load(),
            s_screenshotFileCloseCalls.load());

    s_screenshotFileProbeAttached = false;
    s_createFileWTarget = nullptr;
    s_closeHandleTarget = nullptr;
    s_createFileWOriginal = nullptr;
    s_closeHandleOriginal = nullptr;
    {
        std::lock_guard lock(s_screenshotFileHandlesMutex);
        s_screenshotFileHandles.clear();
    }
}

void AttachScreenshotOutputSubmitProbe()
{
    LogWarn("Screenshot output submit probe attach requested: alreadyAttached=%s",
            g_screenshotOutputSubmitProbeAttached ? "true" : "false");

    if (g_screenshotOutputSubmitProbeAttached)
    {
        return;
    }

    if (!g_sdk || !g_sdk->hooking || !g_sdk->hooking->Attach)
    {
        LogWarn("Screenshot output submit probe skipped: RED4ext hooking API unavailable.");
        return;
    }

    AttachScreenshotFileProbe();

    auto* outputObject = GetRendererOutputObject();
    std::uintptr_t outputVftable = 0;
    std::uintptr_t b8Target = 0;
    std::uintptr_t c0Target = 0;
    if (!IsLikelyProcessPointer(outputObject) ||
        !TryReadPointer(outputObject, 0x0, outputVftable) ||
        !TryReadPointer(reinterpret_cast<void*>(outputVftable), 0xB8, b8Target) ||
        !TryReadPointer(reinterpret_cast<void*>(outputVftable), 0xC0, c0Target))
    {
        LogWarn("Screenshot output submit probe skipped: outputObject=%p vftable=%p b8=%p c0=%p",
                outputObject,
                reinterpret_cast<void*>(outputVftable),
                reinterpret_cast<void*>(b8Target),
                reinterpret_cast<void*>(c0Target));
        return;
    }

    g_screenshotOutputSubmitB8Target = reinterpret_cast<void*>(b8Target);
    g_screenshotOutputSubmitC0Target = reinterpret_cast<void*>(c0Target);

    const auto attachedB8 = g_sdk->hooking->Attach(g_pluginHandle,
                                                  g_screenshotOutputSubmitB8Target,
                                                  reinterpret_cast<void*>(&ScreenshotOutputSubmitB8Hook),
                                                  reinterpret_cast<void**>(&s_screenshotOutputSubmitB8Original));
    const auto attachedC0 = g_sdk->hooking->Attach(g_pluginHandle,
                                                  g_screenshotOutputSubmitC0Target,
                                                  reinterpret_cast<void*>(&ScreenshotOutputSubmitC0Hook),
                                                  reinterpret_cast<void**>(&s_screenshotOutputSubmitC0Original));

    g_screenshotOutputSubmitProbeAttached = attachedB8 || attachedC0;
    LogWarn("Screenshot output submit probe attach: outputObject=%p vftable=%p b8=%p attached=%s original=%p c0=%p attached=%s original=%p",
            outputObject,
            reinterpret_cast<void*>(outputVftable),
            g_screenshotOutputSubmitB8Target,
            attachedB8 ? "true" : "false",
            reinterpret_cast<void*>(s_screenshotOutputSubmitB8Original),
            g_screenshotOutputSubmitC0Target,
            attachedC0 ? "true" : "false",
            reinterpret_cast<void*>(s_screenshotOutputSubmitC0Original));

    AttachImageSavingJobProbe();
}

void DetachScreenshotOutputSubmitProbe()
{
    if (!g_screenshotOutputSubmitProbeAttached || !g_sdk || !g_sdk->hooking || !g_sdk->hooking->Detach)
    {
        return;
    }

    const auto detachedB8 = g_screenshotOutputSubmitB8Target
                                ? g_sdk->hooking->Detach(g_pluginHandle, g_screenshotOutputSubmitB8Target)
                                : false;
    const auto detachedC0 = g_screenshotOutputSubmitC0Target
                                ? g_sdk->hooking->Detach(g_pluginHandle, g_screenshotOutputSubmitC0Target)
                                : false;

    LogWarn("Screenshot output submit probe detach: b8Detached=%s c0Detached=%s calls=%u",
            detachedB8 ? "true" : "false",
            detachedC0 ? "true" : "false",
            g_screenshotOutputSubmitCalls.load());

    g_screenshotOutputSubmitProbeAttached = false;
    g_screenshotOutputSubmitB8Target = nullptr;
    g_screenshotOutputSubmitC0Target = nullptr;
    s_screenshotOutputSubmitB8Original = nullptr;
    s_screenshotOutputSubmitC0Original = nullptr;
}

void AttachImageSavingJobProbe()
{
    LogWarn("ImageSaving job probe attach requested: alreadyAttached=%s",
            g_imageSavingJobProbeAttached ? "true" : "false");

    if (g_imageSavingJobProbeAttached)
    {
        return;
    }

    if (!g_sdk || !g_sdk->hooking || !g_sdk->hooking->Attach)
    {
        LogWarn("ImageSaving job probe skipped: RED4ext hooking API unavailable.");
        return;
    }

    static RED4ext::RelocFunc<ImageSavingJobExecute_t> execute80(kImageSavingJobExecute80Rva);
    static RED4ext::RelocFunc<ImageSavingJobExecute_t> execute68(kImageSavingJobExecute68Rva);
    static RED4ext::RelocFunc<TaskQueueSubmit_t> submit(kTaskQueueSubmitRva);
    static RED4ext::RelocFunc<ScreenshotWriterDispatch_t> writerDispatch(kScreenshotWriterDispatchRva);
    static RED4ext::RelocFunc<ImageSavingCompletionNotify_t> completionNotify(kImageSavingCompletionNotifyRva);
    static RED4ext::RelocFunc<ScreenshotCommandQueueSubmit_t> commandQueueSubmit(kScreenshotCommandQueueSubmitRva);
    static RED4ext::RelocFunc<ScreenshotCommandExecute_t> commandExecute(kScreenshotCommandExecuteRva);
    g_imageSavingJobExecute80Target = reinterpret_cast<void*>(static_cast<ImageSavingJobExecute_t>(execute80));
    g_imageSavingJobExecute68Target = reinterpret_cast<void*>(static_cast<ImageSavingJobExecute_t>(execute68));
    g_taskQueueSubmitTarget = reinterpret_cast<void*>(static_cast<TaskQueueSubmit_t>(submit));
    g_screenshotWriterDispatchTarget = reinterpret_cast<void*>(static_cast<ScreenshotWriterDispatch_t>(writerDispatch));
    g_imageSavingCompletionNotifyTarget = reinterpret_cast<void*>(static_cast<ImageSavingCompletionNotify_t>(completionNotify));
    g_screenshotCommandQueueSubmitTarget = reinterpret_cast<void*>(static_cast<ScreenshotCommandQueueSubmit_t>(commandQueueSubmit));
    g_screenshotCommandExecuteTarget = reinterpret_cast<void*>(static_cast<ScreenshotCommandExecute_t>(commandExecute));

    const auto attached80 = g_sdk->hooking->Attach(g_pluginHandle,
                                                  g_imageSavingJobExecute80Target,
                                                  reinterpret_cast<void*>(&ImageSavingJobExecute80Hook),
                                                  reinterpret_cast<void**>(&s_imageSavingJobExecute80Original));
    const auto attached68 = g_sdk->hooking->Attach(g_pluginHandle,
                                                  g_imageSavingJobExecute68Target,
                                                  reinterpret_cast<void*>(&ImageSavingJobExecute68Hook),
                                                  reinterpret_cast<void**>(&s_imageSavingJobExecute68Original));
    const auto attachedSubmit = g_sdk->hooking->Attach(g_pluginHandle,
                                                       g_taskQueueSubmitTarget,
                                                       reinterpret_cast<void*>(&TaskQueueSubmitHook),
                                                       reinterpret_cast<void**>(&s_taskQueueSubmitOriginal));
    const auto attachedWriterDispatch = g_sdk->hooking->Attach(g_pluginHandle,
                                                               g_screenshotWriterDispatchTarget,
                                                               reinterpret_cast<void*>(&ScreenshotWriterDispatchHook),
                                                               reinterpret_cast<void**>(&s_screenshotWriterDispatchOriginal));
    const auto attachedCompletionNotify = g_sdk->hooking->Attach(g_pluginHandle,
                                                                 g_imageSavingCompletionNotifyTarget,
                                                                 reinterpret_cast<void*>(&ImageSavingCompletionNotifyHook),
                                                                 reinterpret_cast<void**>(&s_imageSavingCompletionNotifyOriginal));
    const auto attachedCommandQueueSubmit = g_sdk->hooking->Attach(g_pluginHandle,
                                                                   g_screenshotCommandQueueSubmitTarget,
                                                                   reinterpret_cast<void*>(&ScreenshotCommandQueueSubmitHook),
                                                                   reinterpret_cast<void**>(&s_screenshotCommandQueueSubmitOriginal));
    const auto attachedCommandExecute = g_sdk->hooking->Attach(g_pluginHandle,
                                                              g_screenshotCommandExecuteTarget,
                                                              reinterpret_cast<void*>(&ScreenshotCommandExecuteHook),
                                                              reinterpret_cast<void**>(&s_screenshotCommandExecuteOriginal));

    g_imageSavingJobProbeAttached = attached80 || attached68 || attachedSubmit || attachedWriterDispatch ||
                                    attachedCompletionNotify || attachedCommandQueueSubmit || attachedCommandExecute;
    LogWarn("ImageSaving job probe attach: execute80=%p attached=%s original=%p execute68=%p attached=%s original=%p submit=%p attached=%s original=%p writerDispatch=%p attached=%s original=%p completionNotify=%p attached=%s original=%p commandQueueSubmit=%p attached=%s original=%p commandExecute=%p attached=%s original=%p",
            g_imageSavingJobExecute80Target,
            attached80 ? "true" : "false",
            reinterpret_cast<void*>(s_imageSavingJobExecute80Original),
            g_imageSavingJobExecute68Target,
            attached68 ? "true" : "false",
            reinterpret_cast<void*>(s_imageSavingJobExecute68Original),
            g_taskQueueSubmitTarget,
            attachedSubmit ? "true" : "false",
            reinterpret_cast<void*>(s_taskQueueSubmitOriginal),
            g_screenshotWriterDispatchTarget,
            attachedWriterDispatch ? "true" : "false",
            reinterpret_cast<void*>(s_screenshotWriterDispatchOriginal),
            g_imageSavingCompletionNotifyTarget,
            attachedCompletionNotify ? "true" : "false",
            reinterpret_cast<void*>(s_imageSavingCompletionNotifyOriginal),
            g_screenshotCommandQueueSubmitTarget,
            attachedCommandQueueSubmit ? "true" : "false",
            reinterpret_cast<void*>(s_screenshotCommandQueueSubmitOriginal),
            g_screenshotCommandExecuteTarget,
            attachedCommandExecute ? "true" : "false",
            reinterpret_cast<void*>(s_screenshotCommandExecuteOriginal));
}

void DetachImageSavingJobProbe()
{
    if (!g_imageSavingJobProbeAttached || !g_sdk || !g_sdk->hooking || !g_sdk->hooking->Detach)
    {
        return;
    }

    const auto detached80 = g_imageSavingJobExecute80Target
                                ? g_sdk->hooking->Detach(g_pluginHandle, g_imageSavingJobExecute80Target)
                                : false;
    const auto detached68 = g_imageSavingJobExecute68Target
                                ? g_sdk->hooking->Detach(g_pluginHandle, g_imageSavingJobExecute68Target)
                                : false;
    const auto detachedSubmit = g_taskQueueSubmitTarget
                                    ? g_sdk->hooking->Detach(g_pluginHandle, g_taskQueueSubmitTarget)
                                    : false;
    const auto detachedWriterDispatch = g_screenshotWriterDispatchTarget
                                            ? g_sdk->hooking->Detach(g_pluginHandle, g_screenshotWriterDispatchTarget)
                                            : false;
    const auto detachedCompletionNotify = g_imageSavingCompletionNotifyTarget
                                              ? g_sdk->hooking->Detach(g_pluginHandle, g_imageSavingCompletionNotifyTarget)
                                              : false;
    const auto detachedCommandQueueSubmit = g_screenshotCommandQueueSubmitTarget
                                                ? g_sdk->hooking->Detach(g_pluginHandle, g_screenshotCommandQueueSubmitTarget)
                                                : false;
    const auto detachedCommandExecute = g_screenshotCommandExecuteTarget
                                            ? g_sdk->hooking->Detach(g_pluginHandle, g_screenshotCommandExecuteTarget)
                                            : false;

    LogWarn("ImageSaving job probe detach: execute80Detached=%s execute68Detached=%s submitDetached=%s writerDispatchDetached=%s completionNotifyDetached=%s commandQueueSubmitDetached=%s commandExecuteDetached=%s executeCalls=%u submitCalls=%u writerDispatchCalls=%u completionNotifyCalls=%u commandQueueSubmitCalls=%u commandExecuteCalls=%u",
            detached80 ? "true" : "false",
            detached68 ? "true" : "false",
            detachedSubmit ? "true" : "false",
            detachedWriterDispatch ? "true" : "false",
            detachedCompletionNotify ? "true" : "false",
            detachedCommandQueueSubmit ? "true" : "false",
            detachedCommandExecute ? "true" : "false",
            g_imageSavingJobExecuteCalls.load(),
            g_imageSavingJobSubmitCalls.load(),
            g_screenshotWriterDispatchCalls.load(),
            g_imageSavingCompletionNotifyCalls.load(),
            g_screenshotCommandQueueSubmitCalls.load(),
            g_screenshotCommandExecuteCalls.load());

    g_imageSavingJobProbeAttached = false;
    g_imageSavingJobExecute80Target = nullptr;
    g_imageSavingJobExecute68Target = nullptr;
    g_taskQueueSubmitTarget = nullptr;
    g_screenshotWriterDispatchTarget = nullptr;
    g_imageSavingCompletionNotifyTarget = nullptr;
    g_screenshotCommandQueueSubmitTarget = nullptr;
    g_screenshotCommandExecuteTarget = nullptr;
    s_imageSavingJobExecute80Original = nullptr;
    s_imageSavingJobExecute68Original = nullptr;
    s_taskQueueSubmitOriginal = nullptr;
    s_screenshotWriterDispatchOriginal = nullptr;
    s_imageSavingCompletionNotifyOriginal = nullptr;
    s_screenshotCommandQueueSubmitOriginal = nullptr;
    s_screenshotCommandExecuteOriginal = nullptr;
}
} // namespace RedFrame
