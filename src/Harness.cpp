#include "Internal.hpp"

namespace RedFrame
{
void RedFrameAutoRunCaptureDiagnosticScreenshot(RED4ext::IScriptable* aContext,
                                                RED4ext::CStackFrame* aFrame,
                                                bool* aOut,
                                                int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    const auto success = QueueProbeScreenshot();
    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameCaptureTestScreenshot(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RedFrameAutoRunCaptureDiagnosticScreenshot(aContext, aFrame, aOut, a4);
}

void RedFrameSetCaptureSequentialFrames(RED4ext::IScriptable* aContext,
                                        RED4ext::CStackFrame* aFrame,
                                        bool* aOut,
                                        int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    bool enabled = true;
    RED4ext::GetParameter(aFrame, &enabled);
    aFrame->code++;

    const auto success = SetCaptureSequentialFrames(enabled);
    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameAutoApplyCaptureSequentialFrames(RED4ext::IScriptable* aContext,
                                              RED4ext::CStackFrame* aFrame,
                                              bool* aOut,
                                              int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    bool success = true;
    if (g_autoRunConfig.setCaptureSequentialFrames)
    {
        success = SetCaptureSequentialFrames(g_autoRunConfig.captureSequentialFramesValue);
    }

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameAutoRunPhotoModeCapture(RED4ext::IScriptable* aContext,
                                     RED4ext::CStackFrame* aFrame,
                                     bool* aOut,
                                     int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    bool success = true;
    if (g_autoRunConfig.photoModeCapture)
    {
        success = InvokePhotoModeDefaultCapture();
    }

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameAutoRunEngineFrameCapture(RED4ext::IScriptable* aContext,
                                       RED4ext::CStackFrame* aFrame,
                                       bool* aOut,
                                       int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    bool success = true;
    if (g_autoRunConfig.engineFrameCapture)
    {
        success = InvokeEngineDefaultFrameCapture();
    }

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameAutoRunScreenshotMatrix(RED4ext::IScriptable* aContext,
                                     RED4ext::CStackFrame* aFrame,
                                     bool* aOut,
                                     int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    bool success = false;
    if (g_autoRunConfig.screenshotMatrix)
    {
        if (g_autoRunConfig.probeScreenshotOutputSubmit)
        {
            AttachScreenshotOutputSubmitProbe();
        }

        success = QueueScreenshotMatrix();
    }

    if (aOut)
    {
        *aOut = success;
    }
}

void RedFrameHarnessReportScreenshotListener(RED4ext::IScriptable* aContext,
                                             RED4ext::CStackFrame* aFrame,
                                             void* aOut,
                                             int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);

    std::int32_t listenerId = 0;
    std::int32_t requestId = 0;
    std::int32_t status = 0;
    std::int32_t error = 0;
    RED4ext::CString primaryPath;
    std::int32_t pathCount = 0;

    RED4ext::GetParameter(aFrame, &listenerId);
    RED4ext::GetParameter(aFrame, &requestId);
    RED4ext::GetParameter(aFrame, &status);
    RED4ext::GetParameter(aFrame, &error);
    RED4ext::GetParameter(aFrame, &primaryPath);
    RED4ext::GetParameter(aFrame, &pathCount);
    aFrame->code++;

    LogWarn("Redscript screenshot listener fired: listener=%d request=%d status=%d error=%d pathCount=%d primary=%s",
            listenerId,
            requestId,
            status,
            error,
            pathCount,
            primaryPath.c_str());
}

void RedFrameIsAutoRunEnabled(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_autoRunConfig.enabled;
    }
}

void RedFrameGetAutoRunStartDelaySeconds(RED4ext::IScriptable* aContext,
                                         RED4ext::CStackFrame* aFrame,
                                         float* aOut,
                                         int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_autoRunConfig.startDelaySeconds;
    }
}

void RedFrameGetAutoRunDurationSeconds(RED4ext::IScriptable* aContext,
                                       RED4ext::CStackFrame* aFrame,
                                       float* aOut,
                                       int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_autoRunConfig.durationSeconds;
    }
}

void RedFrameGetAutoRunIncludeAudio(RED4ext::IScriptable* aContext,
                                    RED4ext::CStackFrame* aFrame,
                                    bool* aOut,
                                    int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_autoRunConfig.includeAudio;
    }
}

void RedFrameGetAutoRunFps(RED4ext::IScriptable* aContext,
                           RED4ext::CStackFrame* aFrame,
                           std::int32_t* aOut,
                           int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_autoRunConfig.recordingFPS;
    }
}

void RedFrameGetAutoRunTimeDilation(RED4ext::IScriptable* aContext,
                                    RED4ext::CStackFrame* aFrame,
                                    float* aOut,
                                    int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_autoRunConfig.timeDilation;
    }
}

void RedFrameShouldAutoCloseGame(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    if (aOut)
    {
        *aOut = g_autoRunConfig.closeGame;
    }
}

void RedFrameRequestGameClose(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);
    aFrame->code++;

    LogWarn("Auto-run requested process exit.");
    ExitProcess(0);
}

void StopNativeAutoRun();

void StartNativeAutoRun()
{
    if (g_nativeAutoRunStarted)
    {
        return;
    }

    g_nativeAutoRunStarted = true;
    LogWarn("Native running-state auto-run starting.");

    if (g_autoRunConfig.setCaptureSequentialFrames)
    {
        SetCaptureSequentialFrames(g_autoRunConfig.captureSequentialFramesValue);
    }

    if (g_autoRunConfig.photoModeCapture)
    {
        InvokePhotoModeDefaultCapture();
    }

    if (g_autoRunConfig.engineFrameCapture)
    {
        InvokeEngineDefaultFrameCapture();
    }

    if (g_autoRunConfig.probeScreenshotOutputSubmit)
    {
        AttachScreenshotOutputSubmitProbe();
    }

    if (g_autoRunConfig.screenshotMatrix)
    {
        QueueScreenshotMatrix();
    }
    else
    {
        QueueProbeScreenshot();
    }

    CaptureStartOptions options{};
    options.includeAudio = g_autoRunConfig.includeAudio;
    options.recordingFPS = g_autoRunConfig.recordingFPS;
    const auto shouldStartFrameDump = !g_autoRunConfig.screenshotMatrix || options.includeAudio || options.recordingFPS > 0;
    if (shouldStartFrameDump)
    {
        StartCapture(options);
    }

    if (g_autoRunConfig.durationSeconds <= 0.0f)
    {
        StopNativeAutoRun();
        return;
    }

    g_nativeAutoRunStopDue = Clock::now() + std::chrono::milliseconds(
                                                 static_cast<int>(g_autoRunConfig.durationSeconds * 1000.0f));
}

void StopNativeAutoRun()
{
    if (g_nativeAutoRunStopped)
    {
        return;
    }

    g_nativeAutoRunStopped = true;
    LogWarn("Native running-state auto-run stopping.");

    StopActiveCapture();

    if (!g_autoRunConfig.screenshotMatrix)
    {
        QueueProbeScreenshot();
    }

    if (g_autoRunConfig.closeGame)
    {
        g_nativeAutoRunCloseScheduled = true;
        g_nativeAutoRunCloseDue = Clock::now() + std::chrono::seconds(10);
        LogWarn("Native running-state auto-run scheduled process exit.");
    }
}

bool RunningStateOnEnter(RED4ext::CGameApplication* aApp)
{
    RED4EXT_UNUSED_PARAMETER(aApp);
    LogInfo("Native Running state entered.");

    if (g_autoRunConfig.enabled && g_autoRunConfig.nativeRunningStateTrigger)
    {
        g_nativeAutoRunActive = true;
        g_nativeAutoRunStarted = false;
        g_nativeAutoRunStopped = false;
        g_nativeAutoRunCloseScheduled = false;
        g_nativeAutoRunStartDue = Clock::now() + std::chrono::milliseconds(
                                                   static_cast<int>(g_autoRunConfig.startDelaySeconds * 1000.0f));
        LogWarn("Native running-state auto-run armed: delay=%.2fs duration=%.2fs",
                g_autoRunConfig.startDelaySeconds,
                g_autoRunConfig.durationSeconds);
        if (g_autoRunConfig.startDelaySeconds <= 0.0f)
        {
            StartNativeAutoRun();
        }
    }

    return true;
}

bool RunningStateOnUpdate(RED4ext::CGameApplication* aApp)
{
    RED4EXT_UNUSED_PARAMETER(aApp);

    PumpScreenshotRequests();

    if (!g_nativeAutoRunActive)
    {
        return true;
    }

    const auto now = Clock::now();
    if (!g_nativeAutoRunStarted && now >= g_nativeAutoRunStartDue)
    {
        StartNativeAutoRun();
    }

    if (g_nativeAutoRunStarted && !g_nativeAutoRunStopped && now >= g_nativeAutoRunStopDue)
    {
        StopNativeAutoRun();
    }

    if (g_nativeAutoRunCloseScheduled && now >= g_nativeAutoRunCloseDue)
    {
        LogWarn("Native running-state auto-run requested process exit.");
        ExitProcess(0);
    }

    return true;
}

bool RunningStateOnExit(RED4ext::CGameApplication* aApp)
{
    RED4EXT_UNUSED_PARAMETER(aApp);
    g_nativeAutoRunActive = false;
    g_nativeAutoRunStarted = false;
    g_nativeAutoRunStopped = false;
    g_nativeAutoRunCloseScheduled = false;
    LogInfo("Native Running state exited.");
    return true;
}

RED4ext::v1::GameState g_runningState = {
    .OnEnter = &RunningStateOnEnter,
    .OnUpdate = &RunningStateOnUpdate,
    .OnExit = &RunningStateOnExit,
};
} // namespace RedFrame
