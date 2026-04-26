#include "Internal.hpp"

namespace RedFrame
{
bool StartEngineViewportCapture(const CaptureStartOptions& aOptions)
{
    auto* viewport = GetEngineViewportProbe();
    if (!viewport)
    {
        LogWarn("EngineViewport capture start failed: no plausible EngineViewport.");
        g_captureSession.lastError = kCaptureErrorViewportUnavailable;
        return false;
    }

    if (!ValidateCaptureStartOptions(aOptions))
    {
        g_captureSession.lastError = kCaptureErrorInvalidOptions;
        return false;
    }

    if (!SavePersistentCaptureProbeState(viewport))
    {
        StopPersistentCaptureProbe();
        g_captureSession.lastError = kCaptureErrorViewportUnavailable;
        return false;
    }

    const auto outputDirectory = MakeCaptureOutputDirectory();
    const auto sourceFrameCutoff = std::filesystem::file_time_type::clock::now() - std::chrono::seconds(2);
    if (!ConfigureEngineViewportCaptureParameters(viewport, kFrameCaptureSaveFormat, outputDirectory, aOptions))
    {
        StopPersistentCaptureProbe();
        g_captureSession.lastError = kCaptureErrorViewportUnavailable;
        return false;
    }

    if (g_autoRunConfig.probeWriterVideoRoot)
    {
        const auto wroteWriterRoot = TryWriteU32(viewport, 0x16C, 1);
        LogWarn("Pre-toggle writer video root probe: writerRoot(+0x16C)=%s",
                wroteWriterRoot ? "ok" : "failed");
        LogPersistentCaptureProbeState("pre-toggle", viewport);
    }

    if (!InvokeToggleContinuousScreenshot(true, kFrameCaptureSaveFormat))
    {
        StopPersistentCaptureProbe();
        g_captureSession.lastError = kCaptureErrorToggleFailed;
        return false;
    }

    if (!ConfigureEngineViewportCaptureParameters(viewport, kFrameCaptureSaveFormat, outputDirectory, aOptions))
    {
        StopPersistentCaptureProbe();
        InvokeToggleContinuousScreenshot(false, 0);
        g_captureSession.lastError = kCaptureErrorViewportUnavailable;
        return false;
    }

    if (!StartPersistentCaptureProbe(true, aOptions.includeAudio))
    {
        StopPersistentCaptureProbe();
        InvokeToggleContinuousScreenshot(false, 0);
        g_captureSession.lastError = kCaptureErrorViewportUnavailable;
        return false;
    }

    if (aOptions.includeAudio || g_autoRunConfig.probeAudioSelfRegister)
    {
        const auto audioOutputPath = GetGameScreenshotsDirectory() / outputDirectory.name / "audio.wav";
        const auto audioStarted = StartAudioSidecarCapture(audioOutputPath, aOptions.includeAudio);
        if (aOptions.includeAudio && !audioStarted)
        {
            StopPersistentCaptureProbe();
            InvokeToggleContinuousScreenshot(false, 0);
            g_captureSession.lastError = kCaptureErrorToggleFailed;
            return false;
        }
    }

    g_captureSession.active = true;
    g_captureSession.backend = kCaptureBackendEngineViewport;
    g_captureSession.includeAudio = aOptions.includeAudio;
    g_captureSession.recordingFPS = aOptions.recordingFPS;
    g_captureSession.saveFormat = kFrameCaptureSaveFormat;
    g_captureSession.lastError = kCaptureErrorNone;
    g_captureSession.initialFrameCounter = 0;
    g_captureSession.lastFrameCounter = 0;
    g_captureSession.outputDirectoryIndex = outputDirectory.index;
    g_captureSession.outputDirectoryName = outputDirectory.name;
    g_captureSession.sourceFrameCutoff = sourceFrameCutoff;

    LogInfo("RedFrame capture started. backend=%d includeAudio=%s fps=%d saveFormat=%d outputDirectoryName=%s outputDirectoryIndex=%u initialFrameCounter=%u",
            g_captureSession.backend,
            g_captureSession.includeAudio ? "true" : "false",
            g_captureSession.recordingFPS,
            g_captureSession.saveFormat,
            g_captureSession.outputDirectoryName.c_str(),
            g_captureSession.outputDirectoryIndex,
            g_captureSession.initialFrameCounter);
    return true;
}

bool StopActiveCapture()
{
    if (!g_captureSession.active)
    {
        return true;
    }

    const auto backend = g_captureSession.backend;
    RefreshCaptureFrameCounter();

    bool success = true;
    if (backend == kCaptureBackendEngineViewport)
    {
        if (IsAudioSidecarCaptureActive())
        {
            success = StopAudioSidecarCapture() && success;
        }
        StopPersistentCaptureProbe();
        success = InvokeToggleContinuousScreenshot(false, 0);
        if (!success)
        {
            g_captureSession.lastError = kCaptureErrorToggleFailed;
        }
    }
    LogInfo("RedFrame capture stopped. backend=%d includeAudio=%s saveFormat=%d outputDirectoryName=%s outputDirectoryIndex=%u frames=%d success=%s",
            backend,
            g_captureSession.includeAudio ? "true" : "false",
            g_captureSession.saveFormat,
            g_captureSession.outputDirectoryName.c_str(),
            g_captureSession.outputDirectoryIndex,
            static_cast<std::int32_t>(g_captureSession.lastFrameCounter - g_captureSession.initialFrameCounter),
            success ? "true" : "false");

    if (success && !g_captureSession.outputDirectoryName.empty())
    {
        FinalizeCaptureOutputFolder(g_captureSession.outputDirectoryName, g_captureSession.sourceFrameCutoff);
    }

    g_captureSession.active = false;
    g_captureSession.backend = 0;
    g_captureSession.includeAudio = false;
    g_captureSession.recordingFPS = 0;
    g_captureSession.outputDirectoryIndex = 0;
    g_captureSession.outputDirectoryName.clear();
    return success;
}

std::int32_t GetCapturedFrameCount()
{
    if (g_captureSession.active)
    {
        RefreshCaptureFrameCounter();
    }

    if (g_captureSession.lastFrameCounter < g_captureSession.initialFrameCounter)
    {
        return 0;
    }

    return static_cast<std::int32_t>(g_captureSession.lastFrameCounter - g_captureSession.initialFrameCounter);
}

bool StartCapture(const CaptureStartOptions& aOptions)
{
    if (g_captureSession.active || g_persistentCaptureProbeState.active || GetContinuousScreenshotObject())
    {
        g_captureSession.lastError = kCaptureErrorAlreadyActive;
        LogWarn("RedFrame capture start rejected: capture is already active.");
        return false;
    }

    return StartEngineViewportCapture(aOptions);
}

} // namespace RedFrame
