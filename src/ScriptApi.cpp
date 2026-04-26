#include "Internal.hpp"

namespace RedFrame
{
void RegisterTypes()
{
    auto* rtti = RED4ext::CRTTISystem::Get();

    const auto registerClass = [rtti](RED4ext::CClass& aClass, const char* aNativeName, const char* aScriptName) {
        const auto nativeName = RED4ext::CNamePool::Add(aNativeName);
        const auto scriptName = RED4ext::CNamePool::Add(aScriptName);

        aClass.flags = {.isNative = true};
        if (!rtti->scriptToNative.Get(scriptName))
        {
            rtti->scriptToNative.Insert(scriptName, nativeName);
            rtti->nativeToScript.Insert(nativeName, scriptName);
        }
        rtti->RegisterType(&aClass);
    };

    registerClass(g_screenshotApiClass, "RedFrameScreenshot", "RedFrame.Screenshot");
    registerClass(g_audioApiClass, "RedFrameAudio", "RedFrame.Audio");
    registerClass(g_debugApiClass, "RedFrameDebug", "RedFrame.Debug");
}

template<typename T>
RED4ext::CClassStaticFunction* CreateStaticMethod(RED4ext::CClass& aClass, const char* aName, T aFunction)
{
    auto* func = RED4ext::CClassStaticFunction::Create(&aClass, aName, aName, aFunction);
    func->flags = {.isNative = true, .isStatic = true};
    return func;
}

void PostRegisterTypes()
{
    auto* rtti = RED4ext::CRTTISystem::Get();
    RED4ext::CBaseFunction::Flags flags = {.isNative = true, .isStatic = true};
    g_screenshotApiClass.parent = rtti->GetClass("IScriptable");
    g_audioApiClass.parent = rtti->GetClass("IScriptable");
    g_debugApiClass.parent = rtti->GetClass("IScriptable");

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "Take", &RedFrameCaptureScreenshot);
        func->SetReturnType("Int32");
        func->AddParam("String", "outputPath");
        func->AddParam("rendScreenshotMode", "mode");
        func->AddParam("ESaveFormat", "saveFormat");
        func->AddParam("renddimEPreset", "resolution");
        func->AddParam("rendResolutionMultiplier", "resolutionMultiplier");
        func->AddParam("Bool", "forceLOD0");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "GetLastError", &RedFrameScreenshotGetLastError);
        func->SetReturnType("Int32");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "GetRequestStatus", &RedFrameScreenshotGetRequestStatus);
        func->SetReturnType("Int32");
        func->AddParam("Int32", "requestId");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "GetRequestError", &RedFrameScreenshotGetRequestError);
        func->SetReturnType("Int32");
        func->AddParam("Int32", "requestId");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "GetRequestPath", &RedFrameScreenshotGetRequestPath);
        func->SetReturnType("String");
        func->AddParam("Int32", "requestId");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "GetRequestPathCount", &RedFrameScreenshotGetRequestPathCount);
        func->SetReturnType("Int32");
        func->AddParam("Int32", "requestId");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "GetRequestPathAt", &RedFrameScreenshotGetRequestPathAt);
        func->SetReturnType("String");
        func->AddParam("Int32", "requestId");
        func->AddParam("Int32", "index");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "RegisterListener", &RedFrameScreenshotRegisterListener);
        func->SetReturnType("Int32");
        func->AddParam("IScriptable", "target");
        func->AddParam("CName", "functionName");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_screenshotApiClass, "UnregisterListener", &RedFrameScreenshotUnregisterListener);
        func->SetReturnType("Bool");
        func->AddParam("Int32", "listenerId");
        g_screenshotApiClass.RegisterFunction(func);
    }

    {
        auto* func = RED4ext::CGlobalFunction::Create("RedFrame.RedFrameScreenshotPump",
                                                      "RedFrame.RedFrameScreenshotPump",
                                                      &RedFrameScreenshotPump);
        func->flags = flags;
        rtti->RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_audioApiClass, "Start", &RedFrameAudioStart);
        func->SetReturnType("Bool");
        func->AddParam("String", "outputPath");
        g_audioApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_audioApiClass, "Stop", &RedFrameAudioStop);
        func->SetReturnType("Bool");
        g_audioApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_audioApiClass, "IsActive", &RedFrameAudioIsActive);
        func->SetReturnType("Bool");
        g_audioApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_audioApiClass, "GetLastError", &RedFrameAudioGetLastError);
        func->SetReturnType("Int32");
        g_audioApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_debugApiClass, "StartFrameDump", &RedFrameDebugStartFrameDump);
        func->SetReturnType("Bool");
        func->AddParam("Int32", "fps");
        g_debugApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_debugApiClass, "StopFrameDump", &RedFrameStopCapture);
        func->SetReturnType("Bool");
        g_debugApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_debugApiClass, "IsFrameDumpActive", &RedFrameIsCapturing);
        func->SetReturnType("Bool");
        g_debugApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_debugApiClass, "GetFrameCount", &RedFrameGetCapturedFrameCount);
        func->SetReturnType("Int32");
        g_debugApiClass.RegisterFunction(func);
    }

    {
        auto* func = CreateStaticMethod(g_debugApiClass, "GetLastError", &RedFrameGetLastError);
        func->SetReturnType("Int32");
        g_debugApiClass.RegisterFunction(func);
    }

    RegisterHarnessScriptApi(rtti, flags);

    LogInfo("Registered RedFrame Screenshot, Audio, Debug APIs and external harness hooks.");
}
} // namespace RedFrame
