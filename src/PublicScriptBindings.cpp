#include "Internal.hpp"

namespace RedFrame
{
void RedFrameStartCapture(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    CaptureStartOptions options{};
    RED4ext::GetParameter(aFrame, &options.includeAudio);
    RED4ext::GetParameter(aFrame, &options.recordingFPS);
    aFrame->code++;

    const auto success = StartCapture(options);

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameVideoStart(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    CaptureStartOptions options{};
    RED4ext::GetParameter(aFrame, &options.includeAudio);
    RED4ext::GetParameter(aFrame, &options.recordingFPS);
    aFrame->code++;

    const auto success = StartCapture(options);

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameDebugStartFrameDump(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    CaptureStartOptions options{};
    RED4ext::GetParameter(aFrame, &options.recordingFPS);
    aFrame->code++;

    const auto success = StartCapture(options);

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameAudioStart(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    RED4ext::CString outputPath;
    RED4ext::GetParameter(aFrame, &outputPath);
    aFrame->code++;

    if (IsAudioSidecarCaptureActive())
    {
        g_audioLastError = kCaptureErrorAlreadyActive;
        LogWarn("Audio capture start rejected: audio capture is already active.");
        if (aOut)
        {
            *aOut = false;
        }
        return;
    }

    const auto resolvedPath = ResolvePublicAudioPath(outputPath);
    if (!resolvedPath)
    {
        g_audioLastError = kCaptureErrorInvalidPath;
        LogWarn("Audio capture rejected unsafe output path: %s", outputPath.c_str());
        if (aOut)
        {
            *aOut = false;
        }
        return;
    }

    const auto success = StartAudioSidecarCapture(*resolvedPath, true);
    g_audioLastError = success ? kCaptureErrorNone : kCaptureErrorToggleFailed;

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameAudioStop(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    const auto success = StopAudioSidecarCapture();
    g_audioLastError = success ? kCaptureErrorNone : kCaptureErrorToggleFailed;

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameCaptureScreenshot(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, std::int32_t* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    RED4ext::CString outputPath;
    auto mode = RED4ext::rend::ScreenshotMode::NORMAL;
    auto saveFormat = RED4ext::ESaveFormat::SF_PNG;
    auto resolution = RED4ext::rend::dim::EPreset::_1280x720;
    auto resolutionMultiplier = RED4ext::rend::ResolutionMultiplier::X1;
    bool forceLOD0 = false;

    RED4ext::GetParameter(aFrame, &outputPath);
    RED4ext::GetParameter(aFrame, &mode);
    RED4ext::GetParameter(aFrame, &saveFormat);
    RED4ext::GetParameter(aFrame, &resolution);
    RED4ext::GetParameter(aFrame, &resolutionMultiplier);
    RED4ext::GetParameter(aFrame, &forceLOD0);
    aFrame->code++;

    const auto resolvedPath = ResolvePublicAdvancedScreenshotPath(outputPath);
    if (!resolvedPath)
    {
        g_screenshotLastError = kCaptureErrorInvalidPath;
        LogWarn("Screenshot rejected unsafe output path: %s", outputPath.c_str());
        if (aOut)
        {
            *aOut = 0;
        }
        return;
    }

    const auto requestId = QueueScreenshot(*resolvedPath,
                                           static_cast<std::int32_t>(mode),
                                           static_cast<std::int32_t>(saveFormat),
                                           static_cast<std::int32_t>(resolution),
                                           static_cast<std::int32_t>(resolutionMultiplier),
                                           forceLOD0);
    if (requestId <= 0)
    {
        g_screenshotLastError = kCaptureErrorScreenshotFailed;
    }
    else
    {
        g_screenshotLastError = kCaptureErrorNone;
    }

    if (aOut)
    {
        *aOut = requestId;
    }
}

void RedFrameStopCapture(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    const auto success = StopActiveCapture();
    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameVideoStop(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RedFrameStopCapture(aContext, aFrame, aOut, a4);
}

void RedFrameIsCapturing(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_captureSession.active;
    }
}

void RedFrameGetCaptureBackend(RED4ext::IScriptable* aContext,
                               RED4ext::CStackFrame* aFrame,
                               std::int32_t* aOut,
                               int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_captureSession.backend;
    }
}

void RedFrameGetCapturedFrameCount(RED4ext::IScriptable* aContext,
                                   RED4ext::CStackFrame* aFrame,
                                   std::int32_t* aOut,
                                   int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = GetCapturedFrameCount();
    }
}

void RedFrameGetLastError(RED4ext::IScriptable* aContext,
                          RED4ext::CStackFrame* aFrame,
                          std::int32_t* aOut,
                          int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_captureSession.lastError;
    }
}

void RedFrameScreenshotGetLastError(RED4ext::IScriptable* aContext,
                                    RED4ext::CStackFrame* aFrame,
                                    std::int32_t* aOut,
                                    int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_screenshotLastError;
    }
}

void RedFrameScreenshotGetRequestStatus(RED4ext::IScriptable* aContext,
                                        RED4ext::CStackFrame* aFrame,
                                        std::int32_t* aOut,
                                        int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    std::int32_t requestId = 0;
    RED4ext::GetParameter(aFrame, &requestId);
    aFrame->code++;

    auto* request = FindScreenshotRequest(requestId);
    if (!request)
    {
        if (aOut)
        {
            *aOut = kScreenshotRequestInvalid;
        }
        return;
    }

    UpdateScreenshotRequestStatus(*request);
    if (aOut)
    {
        *aOut = request->status;
    }
}

void RedFrameScreenshotGetRequestError(RED4ext::IScriptable* aContext,
                                       RED4ext::CStackFrame* aFrame,
                                       std::int32_t* aOut,
                                       int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    std::int32_t requestId = 0;
    RED4ext::GetParameter(aFrame, &requestId);
    aFrame->code++;

    auto* request = FindScreenshotRequest(requestId);
    if (!request)
    {
        if (aOut)
        {
            *aOut = kCaptureErrorInvalidOptions;
        }
        return;
    }

    UpdateScreenshotRequestStatus(*request);
    if (aOut)
    {
        *aOut = request->error;
    }
}

void RedFrameScreenshotGetRequestPath(RED4ext::IScriptable* aContext,
                                      RED4ext::CStackFrame* aFrame,
                                      RED4ext::CString* aOut,
                                      int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    std::int32_t requestId = 0;
    RED4ext::GetParameter(aFrame, &requestId);
    aFrame->code++;

    auto* request = FindScreenshotRequest(requestId);
    if (request)
    {
        UpdateScreenshotRequestStatus(*request);
    }
    if (aOut)
    {
        const auto path = request && !request->outputPaths.empty() ? request->outputPaths.front() : std::filesystem::path{};
        *aOut = RED4ext::CString(path.empty() ? "" : path.string().c_str());
    }
}

void RedFrameScreenshotGetRequestPathCount(RED4ext::IScriptable* aContext,
                                           RED4ext::CStackFrame* aFrame,
                                           std::int32_t* aOut,
                                           int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    std::int32_t requestId = 0;
    RED4ext::GetParameter(aFrame, &requestId);
    aFrame->code++;

    auto* request = FindScreenshotRequest(requestId);
    if (request)
    {
        UpdateScreenshotRequestStatus(*request);
    }
    if (aOut)
    {
        *aOut = request ? static_cast<std::int32_t>(request->outputPaths.size()) : 0;
    }
}

void RedFrameScreenshotGetRequestPathAt(RED4ext::IScriptable* aContext,
                                        RED4ext::CStackFrame* aFrame,
                                        RED4ext::CString* aOut,
                                        int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    std::int32_t requestId = 0;
    std::int32_t index = 0;
    RED4ext::GetParameter(aFrame, &requestId);
    RED4ext::GetParameter(aFrame, &index);
    aFrame->code++;

    auto* request = FindScreenshotRequest(requestId);
    if (request)
    {
        UpdateScreenshotRequestStatus(*request);
    }

    if (aOut)
    {
        if (!request || index < 0 || index >= static_cast<std::int32_t>(request->outputPaths.size()))
        {
            *aOut = RED4ext::CString("");
            return;
        }

        *aOut = RED4ext::CString(request->outputPaths[static_cast<std::size_t>(index)].string().c_str());
    }
}

void RedFrameScreenshotRegisterListener(RED4ext::IScriptable* aContext,
                                        RED4ext::CStackFrame* aFrame,
                                        std::int32_t* aOut,
                                        int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    RED4ext::Handle<RED4ext::IScriptable> target;
    RED4ext::CName functionName;
    RED4ext::GetParameter(aFrame, &target);
    RED4ext::GetParameter(aFrame, &functionName);
    aFrame->code++;

    const auto listenerId = RegisterScreenshotListener(target, functionName);
    if (aOut)
    {
        *aOut = listenerId;
    }
}

void RedFrameScreenshotUnregisterListener(RED4ext::IScriptable* aContext,
                                          RED4ext::CStackFrame* aFrame,
                                          bool* aOut,
                                          int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    std::int32_t listenerId = 0;
    RED4ext::GetParameter(aFrame, &listenerId);
    aFrame->code++;

    const auto removed = UnregisterScreenshotListener(listenerId);
    if (aOut)
    {
        *aOut = removed;
    }
}

void RedFrameScreenshotPump(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    PumpScreenshotRequests();
}

void RedFrameAudioIsActive(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = IsAudioSidecarCaptureActive();
    }
}

void RedFrameAudioGetLastError(RED4ext::IScriptable* aContext,
                               RED4ext::CStackFrame* aFrame,
                               std::int32_t* aOut,
                               int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_audioLastError;
    }
}

void RedFrameVideoIsActive(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_captureSession.active || GetContinuousScreenshotObject() != nullptr;
    }
}

void RedFrameVideoGetTakeIndex(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, std::int32_t* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = GetContinuousScreenshotTakeIndex();
    }
}

void RedFrameVideoGetPocBuild(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, std::int32_t* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = 1;
    }
}

} // namespace RedFrame
