#include "Internal.hpp"

namespace RedFrame
{
AutoRunConfig LoadAutoRunConfigFromEnvironment()
{
    AutoRunConfig config{};

    if (const auto enabled = ReadEnvironmentVariable("REDFRAME_AUTO_RUN"))
    {
        config.enabled = ParseEnvironmentBool(*enabled, config.enabled);
    }

    if (const auto delay = ReadEnvironmentVariable("REDFRAME_AUTO_START_DELAY_SECONDS"))
    {
        config.startDelaySeconds = std::max(0.0f, ParseEnvironmentFloat(*delay, config.startDelaySeconds));
    }

    if (const auto duration = ReadEnvironmentVariable("REDFRAME_AUTO_DURATION_SECONDS"))
    {
        config.durationSeconds = std::max(0.0f, ParseEnvironmentFloat(*duration, config.durationSeconds));
    }

    if (const auto includeAudio = ReadEnvironmentVariable("REDFRAME_AUTO_INCLUDE_AUDIO"))
    {
        config.includeAudio = ParseEnvironmentBool(*includeAudio, config.includeAudio);
    }

    if (const auto fps = ReadEnvironmentVariable("REDFRAME_AUTO_FPS"))
    {
        config.recordingFPS = std::max(0, ParseEnvironmentInt32(*fps, config.recordingFPS));
    }

    if (const auto timeDilation = ReadEnvironmentVariable("REDFRAME_AUTO_TIME_DILATION"))
    {
        config.timeDilation = ParseEnvironmentFloat(*timeDilation, config.timeDilation);
    }

    if (const auto closeGame = ReadEnvironmentVariable("REDFRAME_AUTO_CLOSE_GAME"))
    {
        config.closeGame = ParseEnvironmentBool(*closeGame, config.closeGame);
    }

    if (const auto setCaptureSequential = ReadEnvironmentVariable("REDFRAME_AUTO_SET_CAPTURE_SEQUENTIAL"))
    {
        config.setCaptureSequentialFrames = ParseEnvironmentBool(*setCaptureSequential, config.setCaptureSequentialFrames);
    }

    if (const auto captureSequentialValue = ReadEnvironmentVariable("REDFRAME_AUTO_CAPTURE_SEQUENTIAL_VALUE"))
    {
        config.captureSequentialFramesValue =
            ParseEnvironmentBool(*captureSequentialValue, config.captureSequentialFramesValue);
    }

    if (const auto photoModeCapture = ReadEnvironmentVariable("REDFRAME_AUTO_PHOTOMODE_CAPTURE"))
    {
        config.photoModeCapture = ParseEnvironmentBool(*photoModeCapture, config.photoModeCapture);
    }

    if (const auto engineFrameCapture = ReadEnvironmentVariable("REDFRAME_AUTO_ENGINE_FRAME_CAPTURE"))
    {
        config.engineFrameCapture = ParseEnvironmentBool(*engineFrameCapture, config.engineFrameCapture);
    }

    if (const auto screenshotMatrix = ReadEnvironmentVariable("REDFRAME_AUTO_SCREENSHOT_MATRIX"))
    {
        config.screenshotMatrix = ParseEnvironmentBool(*screenshotMatrix, config.screenshotMatrix);
    }

    if (const auto nativeRunningStateTrigger = ReadEnvironmentVariable("REDFRAME_AUTO_NATIVE_RUNNING_STATE"))
    {
        config.nativeRunningStateTrigger =
            ParseEnvironmentBool(*nativeRunningStateTrigger, config.nativeRunningStateTrigger);
    }

    if (const auto persistentCaptureProbe = ReadEnvironmentVariable("REDFRAME_AUTO_PERSISTENT_CAPTURE_PROBE"))
    {
        config.persistentCaptureProbe = ParseEnvironmentBool(*persistentCaptureProbe, config.persistentCaptureProbe);
    }

    if (const auto probeWriterVideoRoot = ReadEnvironmentVariable("REDFRAME_PROBE_WRITER_VIDEO_ROOT"))
    {
        config.probeWriterVideoRoot = ParseEnvironmentBool(*probeWriterVideoRoot, config.probeWriterVideoRoot);
    }

    if (const auto probeVideoRootScreenshot = ReadEnvironmentVariable("REDFRAME_PROBE_VIDEO_ROOT_SCREENSHOT"))
    {
        config.probeVideoRootScreenshot =
            ParseEnvironmentBool(*probeVideoRootScreenshot, config.probeVideoRootScreenshot);
    }

    if (const auto probeAudioCaptureCallback = ReadEnvironmentVariable("REDFRAME_PROBE_AUDIO_CAPTURE_CALLBACK"))
    {
        config.probeAudioCaptureCallback =
            ParseEnvironmentBool(*probeAudioCaptureCallback, config.probeAudioCaptureCallback);
    }

    if (const auto probeAudioSelfRegister = ReadEnvironmentVariable("REDFRAME_PROBE_AUDIO_SELF_REGISTER"))
    {
        config.probeAudioSelfRegister =
            ParseEnvironmentBool(*probeAudioSelfRegister, config.probeAudioSelfRegister);
    }

    if (const auto probeScreenshotOutputSubmit = ReadEnvironmentVariable("REDFRAME_PROBE_SCREENSHOT_OUTPUT_SUBMIT"))
    {
        config.probeScreenshotOutputSubmit =
            ParseEnvironmentBool(*probeScreenshotOutputSubmit, config.probeScreenshotOutputSubmit);
    }

    if (const auto screenshotMatrixCase = ReadEnvironmentVariable("REDFRAME_SCREENSHOT_MATRIX_CASE"))
    {
        config.screenshotMatrixCase = ParseEnvironmentInt32(*screenshotMatrixCase, config.screenshotMatrixCase);
    }

    if (const auto probeScreenshotTailMode = ReadEnvironmentVariable("REDFRAME_PROBE_SCREENSHOT_TAIL_MODE"))
    {
        config.probeScreenshotTailMode =
            ParseEnvironmentInt32(*probeScreenshotTailMode, config.probeScreenshotTailMode);
    }

    if (const auto probeScreenshotTailF3 = ReadEnvironmentVariable("REDFRAME_PROBE_SCREENSHOT_TAIL_F3"))
    {
        config.probeScreenshotTailF3 =
            ParseEnvironmentBool(*probeScreenshotTailF3, config.probeScreenshotTailF3);
    }

    if (const auto probeScreenshotTailF4 = ReadEnvironmentVariable("REDFRAME_PROBE_SCREENSHOT_TAIL_F4"))
    {
        config.probeScreenshotTailF4 =
            ParseEnvironmentInt32(*probeScreenshotTailF4, config.probeScreenshotTailF4);
    }

    return config;
}

} // namespace RedFrame
