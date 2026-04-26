#include "Internal.hpp"

namespace RedFrame
{
std::filesystem::path MakeProbeScreenshotPath()
{
    const auto index = ++g_screenshotIndex;
    char fileName[64]{};
    sprintf_s(fileName, "take_screenshot_probe_%04d.png", index);
    return std::filesystem::current_path() / "RedFrameHarness" / fileName;
}

ScreenshotRequest* FindScreenshotRequest(const std::int32_t aRequestId)
{
    for (auto& request : g_screenshotRequests)
    {
        if (request.id == aRequestId)
        {
            return &request;
        }
    }

    return nullptr;
}

bool IsLikelyScreenshotOutputForRequest(const ScreenshotRequest& aRequest, const std::filesystem::path& aCandidatePath)
{
    const auto requestedName = aRequest.outputPath.filename().string();
    const auto requestedStem = aRequest.outputPath.stem().string();
    const auto requestedExtension = aRequest.outputPath.extension().string();
    const auto candidateName = aCandidatePath.filename().string();

    if (candidateName == requestedName)
    {
        return true;
    }

    if (!requestedName.empty() && candidateName.starts_with(requestedName))
    {
        return true;
    }

    if (!requestedStem.empty() && !requestedExtension.empty() && candidateName.starts_with(requestedStem))
    {
        const auto suffix = candidateName.substr(requestedStem.size());
        if (suffix.starts_with(requestedExtension) ||
            suffix.starts_with(".png") ||
            suffix.starts_with(".exr") ||
            suffix.starts_with("HIGH_RES_") ||
            suffix.starts_with("_HIGH_RES_"))
        {
            return true;
        }
    }

    return false;
}

std::vector<std::filesystem::path> FindScreenshotOutputsForRequest(const ScreenshotRequest& aRequest)
{
    std::vector<std::filesystem::path> paths;
    std::error_code ec;
    const auto parentPath = aRequest.outputPath.parent_path();
    const auto minWriteTime = aRequest.queuedFileTime - std::chrono::seconds(1);

    if (std::filesystem::exists(aRequest.outputPath, ec))
    {
        const auto writeTime = std::filesystem::last_write_time(aRequest.outputPath, ec);
        if (!ec && writeTime >= minWriteTime)
        {
            paths.push_back(aRequest.outputPath);
        }
    }
    ec.clear();

    if (!parentPath.empty() && std::filesystem::exists(parentPath, ec))
    {
        for (const auto& entry : std::filesystem::directory_iterator(parentPath, ec))
        {
            if (ec)
            {
                break;
            }

            if (!entry.is_regular_file(ec))
            {
                ec.clear();
                continue;
            }

            const auto candidatePath = entry.path();
            if (!IsLikelyScreenshotOutputForRequest(aRequest, candidatePath))
            {
                continue;
            }

            const auto writeTime = entry.last_write_time(ec);
            if (ec)
            {
                ec.clear();
                continue;
            }

            if (writeTime < minWriteTime)
            {
                continue;
            }

            if (std::find(paths.begin(), paths.end(), candidatePath) == paths.end())
            {
                paths.push_back(candidatePath);
            }
        }
    }

    std::sort(paths.begin(), paths.end());
    return paths;
}

void UpdateScreenshotRequestStatus(ScreenshotRequest& aRequest)
{
    if (aRequest.status == kScreenshotRequestFailed ||
        aRequest.status == kScreenshotRequestTimeout)
    {
        return;
    }

    const auto now = Clock::now();
    std::error_code ec;
    auto outputPaths = FindScreenshotOutputsForRequest(aRequest);
    if (aRequest.status == kScreenshotRequestComplete)
    {
        if (!outputPaths.empty() && outputPaths != aRequest.outputPaths)
        {
            LogInfo("Screenshot request %d discovered late output file(s): previous=%zu current=%zu primary=%s",
                    aRequest.id,
                    aRequest.outputPaths.size(),
                    outputPaths.size(),
                    outputPaths.front().string().c_str());
            aRequest.outputPaths = std::move(outputPaths);
        }

        if (now - aRequest.completedAt > std::chrono::seconds(kScreenshotRequestOutputDiscoverySeconds))
        {
            return;
        }

        return;
    }

    if (outputPaths.empty())
    {
        if (now - aRequest.queuedAt > std::chrono::seconds(kScreenshotRequestTimeoutSeconds))
        {
            aRequest.status = kScreenshotRequestTimeout;
            aRequest.error = kCaptureErrorScreenshotFailed;
            LogWarn("Screenshot request %d timed out waiting for %s",
                    aRequest.id,
                    aRequest.outputPath.string().c_str());
        }
        return;
    }

    if (aRequest.outputPaths != outputPaths)
    {
        LogInfo("Screenshot request %d discovered %zu output file(s): requested=%s primary=%s",
                aRequest.id,
                outputPaths.size(),
                aRequest.outputPath.string().c_str(),
                outputPaths.front().string().c_str());
        aRequest.outputPaths = std::move(outputPaths);
    }

    std::uintmax_t totalSize = 0;
    std::filesystem::file_time_type latestWriteTime{};
    for (const auto& path : aRequest.outputPaths)
    {
        const auto size = std::filesystem::file_size(path, ec);
        if (ec)
        {
            aRequest.status = kScreenshotRequestWriting;
            return;
        }

        const auto writeTime = std::filesystem::last_write_time(path, ec);
        if (ec)
        {
            aRequest.status = kScreenshotRequestWriting;
            return;
        }

        totalSize += size;
        latestWriteTime = std::max(latestWriteTime, writeTime);
    }

    if (aRequest.status != kScreenshotRequestWriting ||
        aRequest.observedTotalSize != totalSize ||
        aRequest.observedLatestWriteTime != latestWriteTime)
    {
        aRequest.status = kScreenshotRequestWriting;
        aRequest.observedTotalSize = totalSize;
        aRequest.observedLatestWriteTime = latestWriteTime;
        aRequest.lastObservedAt = now;
        return;
    }

    if (now - aRequest.lastObservedAt >= std::chrono::milliseconds(kScreenshotRequestStableMilliseconds))
    {
        aRequest.status = kScreenshotRequestComplete;
        aRequest.error = kCaptureErrorNone;
        aRequest.completedAt = now;
        LogInfo("Screenshot request %d completed with %zu output file(s), primary=%s",
                aRequest.id,
                aRequest.outputPaths.size(),
                aRequest.outputPaths.empty() ? "" : aRequest.outputPaths.front().string().c_str());
    }
}

std::int32_t QueueScreenshot(const std::filesystem::path& aOutputPath,
                             const std::int32_t aMode,
                             const std::int32_t aSaveFormat,
                             const std::int32_t aResolution,
                             const std::int32_t aResolutionMultiplier,
                             const bool aForceLOD0)
{
    auto* engine = RED4ext::CGameEngine::Get();
    if (!engine)
    {
        LogError("CGameEngine::Get() returned null while queuing screenshot.");
        return 0;
    }

    if (aOutputPath.empty())
    {
        LogError("Screenshot output path was empty.");
        return 0;
    }

    std::error_code ec;
    const auto parentPath = aOutputPath.parent_path();
    if (!parentPath.empty())
    {
        std::filesystem::create_directories(parentPath, ec);
        if (ec)
        {
            LogError("Failed to create screenshot directory '%s' (%d)", parentPath.string().c_str(), ec.value());
            return 0;
        }
    }

    ScreenshotRequest request{};
    request.id = ++g_screenshotRequestIndex;
    request.outputPath = aOutputPath;
    request.queuedAt = Clock::now();
    request.queuedFileTime = std::filesystem::file_time_type::clock::now();
    request.lastObservedAt = request.queuedAt;

    auto data = std::make_unique<NativeSingleScreenShotData>();
    data->mode = static_cast<RED4ext::rend::ScreenshotMode>(aMode);
    data->outputPath.Path = RED4ext::CString(aOutputPath.string().c_str());
    data->resolution = static_cast<RED4ext::rend::dim::EPreset>(aResolution);
    data->resolutionMultiplier = static_cast<RED4ext::rend::ResolutionMultiplier>(aResolutionMultiplier);
    data->forceLOD0 = aForceLOD0;
    data->saveFormat = static_cast<RED4ext::ESaveFormat>(aSaveFormat);
    data->emmModes = RED4ext::DynArray<RED4ext::EEnvManagerModifier>(RED4ext::Memory::ScriptAllocator::Get());
    data->emmModes.PushBack(RED4ext::EEnvManagerModifier::EMM_None);

    engine->TakeScreenshot(*data, false);

    request.data = std::move(data);
    g_screenshotRequests.push_back(std::move(request));

    LogInfo("Queued TakeScreenshot request %d -> %s",
            g_screenshotRequestIndex,
            aOutputPath.string().c_str());
    return g_screenshotRequestIndex;
}

bool QueueDefaultRootScreenshot(const bool aVideoRoot)
{
    auto* engine = RED4ext::CGameEngine::Get();
    auto* viewport = GetEngineViewportProbe();
    if (!engine || !viewport)
    {
        LogError("Default-root TakeScreenshot probe failed: engine=%p viewport=%p", engine, viewport);
        return false;
    }

    auto& captureParameters = viewport->captureParameters;
    const auto previousContext = captureParameters.captureContextType;
    const auto previousOutputPath = captureParameters.outputPath.Path;
    const auto previousOutputDirectoryName = captureParameters.outputDirectoryName;
    const auto previousOutputDirectoryNameSuffix = captureParameters.outputDirectoryNameSuffix;
    const auto previousOutputDirectoryIndex = captureParameters.outputDirectoryIndex;
    captureParameters.captureContextType =
        static_cast<RED4ext::rend::CaptureContextType>(aVideoRoot ? 1 : 0);
    if (aVideoRoot)
    {
        captureParameters.outputPath.Path = RED4ext::CString("");
        captureParameters.outputDirectoryName = RED4ext::CString("RedFrameVideoProbe");
        captureParameters.outputDirectoryNameSuffix = RED4ext::CString("");
        captureParameters.outputDirectoryIndex = 0;
    }

    auto data = std::make_unique<NativeSingleScreenShotData>();
    data->mode = RED4ext::rend::ScreenshotMode::NORMAL;
    data->resolution = RED4ext::rend::dim::EPreset::_1280x720;
    data->resolutionMultiplier = RED4ext::rend::ResolutionMultiplier::X1;
    data->forceLOD0 = false;
    data->saveFormat = RED4ext::ESaveFormat::SF_PNG;
    data->emmModes = RED4ext::DynArray<RED4ext::EEnvManagerModifier>(RED4ext::Memory::ScriptAllocator::Get());
    data->emmModes.PushBack(RED4ext::EEnvManagerModifier::EMM_None);
    if (g_autoRunConfig.probeScreenshotTailMode >= 0)
    {
        const auto tailMode = g_autoRunConfig.probeScreenshotTailMode;
        TryWriteBool(data.get(), 0xF0, tailMode == 0);
        TryWriteBool(data.get(), 0xF1, tailMode == 5);
    }
    if (g_autoRunConfig.probeScreenshotTailF3)
    {
        TryWriteBool(data.get(), 0xF3, true);
    }
    if (g_autoRunConfig.probeScreenshotTailF4 != 0)
    {
        TryWriteU32(data.get(), 0xF4, static_cast<std::uint32_t>(g_autoRunConfig.probeScreenshotTailF4));
    }

    std::uint32_t contextValue = 0;
    bool tailF0 = false;
    bool tailF1 = false;
    bool tailF3 = false;
    std::uint32_t tailF4 = 0;
    TryReadU32(viewport, 0x16C, contextValue);
    TryReadBool(data.get(), 0xF0, tailF0);
    TryReadBool(data.get(), 0xF1, tailF1);
    TryReadBool(data.get(), 0xF3, tailF3);
    TryReadU32(data.get(), 0xF4, tailF4);
    LogInfo("Queueing default-root TakeScreenshot probe: videoRoot=%s captureContextType(+0x16C)=%u outputPath=%s outputDirectoryName=%s tailF0=%s tailF1=%s tailF3=%s tailF4=%u",
            aVideoRoot ? "true" : "false",
            contextValue,
            captureParameters.outputPath.Path.c_str(),
            captureParameters.outputDirectoryName.c_str(),
            tailF0 ? "true" : "false",
            tailF1 ? "true" : "false",
            tailF3 ? "true" : "false",
            tailF4);

    engine->TakeScreenshot(*data, false);
    if (!aVideoRoot)
    {
        captureParameters.captureContextType = previousContext;
        captureParameters.outputPath.Path = previousOutputPath;
        captureParameters.outputDirectoryName = previousOutputDirectoryName;
        captureParameters.outputDirectoryNameSuffix = previousOutputDirectoryNameSuffix;
        captureParameters.outputDirectoryIndex = previousOutputDirectoryIndex;
    }
    else
    {
        LogWarn("Default-root video screenshot probe is leaving capture params in video-root shape for async path setup.");
    }
    g_probeScreenshotRequests.push_back(std::move(data));
    return true;
}

bool QueueProbeScreenshot()
{
    if (g_autoRunConfig.probeVideoRootScreenshot)
    {
        const auto success = QueueDefaultRootScreenshot(true);
        if (!success)
        {
            g_captureSession.lastError = kCaptureErrorScreenshotFailed;
        }

        return success;
    }

    const auto requestId = QueueScreenshot(MakeProbeScreenshotPath(),
                                           static_cast<std::int32_t>(RED4ext::rend::ScreenshotMode::NORMAL),
                                           static_cast<std::int32_t>(RED4ext::ESaveFormat::SF_PNG),
                                           static_cast<std::int32_t>(RED4ext::rend::dim::EPreset::_1280x720),
                                           static_cast<std::int32_t>(RED4ext::rend::ResolutionMultiplier::X1),
                                           false);
    if (requestId <= 0)
    {
        g_captureSession.lastError = kCaptureErrorScreenshotFailed;
    }

    return requestId > 0;
}

bool QueueScreenshotMatrix()
{
    struct MatrixCase
    {
        const char* name = nullptr;
        const char* extension = nullptr;
        std::int32_t mode = 1;
        std::int32_t saveFormat = 2;
        std::int32_t resolution = 5;
        std::int32_t multiplier = 1;
    };

    constexpr MatrixCase cases[] = {
        {"normal_png_baseline_m1_f2_r5_x1", ".png", 1, 2, 5, 1},
        {"normal_multisample_png_m2_f2_r5_x1", ".png", 2, 2, 5, 1},
        {"hires_png_m5_f2_r5_x1", ".png", 5, 2, 5, 1},
        {"hires_exr_m5_f32_r5_x1", ".exr", 5, 32, 5, 1},
        {"hires_png_and_exr_m5_f34_r5_x1", ".exr", 5, 34, 5, 1},
        {"hires_exr_m5_f32_r5_x2", ".exr", 5, 32, 5, 2},
        {"hires_exr_m5_f32_r5_x4", ".exr", 5, 32, 5, 4},
        {"hires_exr_ultrawide_m5_f32_r11_x1", ".exr", 5, 32, 11, 1},
        {"hires_exr_wide_m5_f32_r12_x1", ".exr", 5, 32, 12, 1},
        {"hires_layered_exr_m6_f32_r5_x1", ".exr", 6, 32, 5, 1},
    };

    const auto runIndex = ++g_screenshotIndex;
    char runName[64]{};
    sprintf_s(runName, "screenshot_matrix_%04d", runIndex);
    const auto outputDirectory = GetRedFrameOutputDirectory() / "Harness" / runName;

    std::int32_t queuedCount = 0;
    for (const auto& testCase : cases)
    {
        const auto outputPath = outputDirectory / (std::string(testCase.name) + testCase.extension);
        const auto requestId = QueueScreenshot(outputPath,
                                               testCase.mode,
                                               testCase.saveFormat,
                                               testCase.resolution,
                                               testCase.multiplier,
                                               false);
        if (requestId > 0)
        {
            ++queuedCount;
            LogInfo("Screenshot matrix queued: run=%s request=%d name=%s mode=%d saveFormat=%d resolution=%d multiplier=%d requested=%s",
                    runName,
                    requestId,
                    testCase.name,
                    testCase.mode,
                    testCase.saveFormat,
                    testCase.resolution,
                    testCase.multiplier,
                    outputPath.string().c_str());
        }
        else
        {
            LogWarn("Screenshot matrix failed to queue: run=%s name=%s requested=%s",
                    runName,
                    testCase.name,
                    outputPath.string().c_str());
        }
    }

    LogWarn("Screenshot matrix queued %d/%zu cases into %s",
            queuedCount,
            std::size(cases),
            outputDirectory.string().c_str());
    return queuedCount == std::size(cases);
}

} // namespace RedFrame
