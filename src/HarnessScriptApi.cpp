#include "Internal.hpp"

namespace RedFrame
{
void RegisterHarnessScriptApi(RED4ext::CRTTISystem* aRtti, RED4ext::CBaseFunction::Flags aFlags)
{
    auto* rtti = aRtti;
    const auto flags = aFlags;
    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessCaptureDiagnosticScreenshot",
                                                      "RedFrameHarnessCaptureDiagnosticScreenshot",
                                                      &RedFrameAutoRunCaptureDiagnosticScreenshot);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessApplyCaptureSequentialFrames",
                                                      "RedFrameHarnessApplyCaptureSequentialFrames",
                                                      &RedFrameAutoApplyCaptureSequentialFrames);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessRunPhotoModeCapture",
                                                      "RedFrameHarnessRunPhotoModeCapture",
                                                      &RedFrameAutoRunPhotoModeCapture);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessRunEngineFrameCapture",
                                                      "RedFrameHarnessRunEngineFrameCapture",
                                                      &RedFrameAutoRunEngineFrameCapture);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessRunScreenshotMatrix",
                                                      "RedFrameHarnessRunScreenshotMatrix",
                                                      &RedFrameAutoRunScreenshotMatrix);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func =
            RED4ext::CGlobalFunction::Create("RedFrameHarnessIsEnabled", "RedFrameHarnessIsEnabled", &RedFrameIsAutoRunEnabled);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessGetStartDelaySeconds",
                                                      "RedFrameHarnessGetStartDelaySeconds",
                                                      &RedFrameGetAutoRunStartDelaySeconds);
        func->flags = flags;
        func->SetReturnType("Float");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessGetDurationSeconds",
                                                      "RedFrameHarnessGetDurationSeconds",
                                                      &RedFrameGetAutoRunDurationSeconds);
        func->flags = flags;
        func->SetReturnType("Float");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessGetIncludeAudio",
                                                      "RedFrameHarnessGetIncludeAudio",
                                                      &RedFrameGetAutoRunIncludeAudio);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessGetFps",
                                                      "RedFrameHarnessGetFps",
                                                      &RedFrameGetAutoRunFps);
        func->flags = flags;
        func->SetReturnType("Int32");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessGetTimeDilation",
                                                      "RedFrameHarnessGetTimeDilation",
                                                      &RedFrameGetAutoRunTimeDilation);
        func->flags = flags;
        func->SetReturnType("Float");
        rtti->RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrameHarnessShouldCloseGame",
                                                      "RedFrameHarnessShouldCloseGame",
                                                      &RedFrameShouldAutoCloseGame);
        func->flags = flags;
        func->SetReturnType("Bool");
        rtti->RegisterFunction(func);
    }

    {
        auto* func =
            RED4ext::CGlobalFunction::Create("RedFrameHarnessRequestGameClose", "RedFrameHarnessRequestGameClose", &RedFrameRequestGameClose);
        func->flags = flags;
        rtti->RegisterFunction(func);
    }

}
} // namespace RedFrame
