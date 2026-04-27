#include "Internal.hpp"

using namespace RedFrame;

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    g_pluginHandle = aHandle;
    g_sdk = aSdk;

    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        auto* rtti = RED4ext::CRTTISystem::Get();
        g_autoRunConfig = LoadAutoRunConfigFromEnvironment();
        rtti->AddRegisterCallback(RegisterTypes);
        rtti->AddPostRegisterCallback(PostRegisterTypes);
        if (aSdk && aSdk->gameStates)
        {
            aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &g_runningState);
        }
        if (g_autoRunConfig.probeAudioCaptureCallback)
        {
            AttachAudioCaptureCallbackProbe();
        }

        LogWarn("Loaded RedFrame. Native EngineViewport capture path currently supports Cyberpunk 2077 2.31. cwd=%s",
                std::filesystem::current_path().string().c_str());
        if (g_autoRunConfig.enabled)
        {
            LogWarn("Auto-run armed: delay=%.2fs duration=%.2fs includeAudio=%s fps=%d timeDilation=%.9f closeGame=%s setCaptureSequential=%s value=%s photoModeCapture=%s engineFrameCapture=%s screenshotMatrix=%s screenshotMatrixCase=%d screenshotMatrixEmmMode=%d nativeRunningState=%s persistentCaptureProbe=%s probeWriterVideoRoot=%s probeVideoRootScreenshot=%s probeAudioCaptureCallback=%s probeAudioSelfRegister=%s probeScreenshotOutputSubmit=%s probeScreenshotTailMode=%d probeScreenshotTailF3=%s probeScreenshotTailF4=%d",
                    g_autoRunConfig.startDelaySeconds,
                    g_autoRunConfig.durationSeconds,
                    g_autoRunConfig.includeAudio ? "true" : "false",
                    g_autoRunConfig.recordingFPS,
                    g_autoRunConfig.timeDilation,
                    g_autoRunConfig.closeGame ? "true" : "false",
                    g_autoRunConfig.setCaptureSequentialFrames ? "true" : "false",
                    g_autoRunConfig.captureSequentialFramesValue ? "true" : "false",
                    g_autoRunConfig.photoModeCapture ? "true" : "false",
                    g_autoRunConfig.engineFrameCapture ? "true" : "false",
                    g_autoRunConfig.screenshotMatrix ? "true" : "false",
                    g_autoRunConfig.screenshotMatrixCase,
                    g_autoRunConfig.screenshotMatrixEmmMode,
                    g_autoRunConfig.nativeRunningStateTrigger ? "true" : "false",
                    g_autoRunConfig.persistentCaptureProbe ? "true" : "false",
                    g_autoRunConfig.probeWriterVideoRoot ? "true" : "false",
                    g_autoRunConfig.probeVideoRootScreenshot ? "true" : "false",
                    g_autoRunConfig.probeAudioCaptureCallback ? "true" : "false",
                    g_autoRunConfig.probeAudioSelfRegister ? "true" : "false",
                    g_autoRunConfig.probeScreenshotOutputSubmit ? "true" : "false",
                    g_autoRunConfig.probeScreenshotTailMode,
                    g_autoRunConfig.probeScreenshotTailF3 ? "true" : "false",
                    g_autoRunConfig.probeScreenshotTailF4);
        }
        else
        {
            LogInfo("Auto-run disabled. Set REDFRAME_AUTO_RUN=1 to arm session-ready harness.");
        }
        break;
    }
    case RED4ext::v1::EMainReason::Unload:
    {
        StopActiveCapture();
        StopAudioSidecarCapture();
        WaitCaptureOutputFinalizer();
        ClearScreenshotListeners();
        DetachImageSavingJobProbe();
        DetachScreenshotOutputSubmitProbe();
        DetachScreenshotFileProbe();
        DetachAudioCaptureCallbackProbe();
        LogInfo("Unloaded.");
        break;
    }
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name = kProjectName;
    aInfo->author = L"Codex";
    aInfo->version = RED4EXT_V1_SEMVER(0, 1, 0);
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_LATEST;
    aInfo->sdk = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}
