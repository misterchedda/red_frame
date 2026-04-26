#include "Internal.hpp"

namespace RedFrame
{
void FinalizeCaptureOutputFolder(const std::string aOutputDirectoryName,
                                 const std::filesystem::file_time_type aSourceFrameCutoff)
{
    if (g_captureOutputFinalizeThread.joinable())
    {
        g_captureOutputFinalizeCancel.store(true);
        g_captureOutputFinalizeThread.join();
    }

    g_captureOutputFinalizeCancel.store(false);
    g_captureOutputFinalizeThread = std::thread([aOutputDirectoryName, aSourceFrameCutoff]() {
        for (auto i = 0; i < 70; ++i)
        {
            if (g_captureOutputFinalizeCancel.load())
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        const auto screenshotsDirectory = GetGameScreenshotsDirectory();
        if (screenshotsDirectory.empty())
        {
            LogWarn("Capture output finalize skipped: USERPROFILE was unavailable.");
            return;
        }

        const auto destinationDirectory = screenshotsDirectory / aOutputDirectoryName;
        std::error_code ec;
        std::filesystem::create_directories(destinationDirectory, ec);
        if (ec)
        {
            LogWarn("Capture output finalize failed: could not create %s (%d)",
                    destinationDirectory.string().c_str(),
                    ec.value());
            return;
        }

        std::uint32_t moved = 0;
        const auto moveRecentFrames = [&](const std::filesystem::path& aSourceDirectory, const bool aRequireFramePrefix) {
            if (g_captureOutputFinalizeCancel.load())
            {
                return;
            }
            if (!std::filesystem::exists(aSourceDirectory, ec))
            {
                ec.clear();
                return;
            }

            for (const auto& entry : std::filesystem::directory_iterator(aSourceDirectory, ec))
            {
                if (g_captureOutputFinalizeCancel.load())
                {
                    return;
                }
                if (ec)
                {
                    break;
                }

                if (!entry.is_regular_file(ec) || ToLowerCopy(entry.path().extension().string()) != ".png")
                {
                    continue;
                }

                const auto fileName = ToLowerCopy(entry.path().filename().string());
                if (aRequireFramePrefix && !fileName.starts_with("frame"))
                {
                    continue;
                }

                const auto writeTime = entry.last_write_time(ec);
                if (ec || writeTime < aSourceFrameCutoff)
                {
                    continue;
                }

                const auto destinationPath = destinationDirectory / entry.path().filename();
                std::filesystem::rename(entry.path(), destinationPath, ec);
                if (!ec)
                {
                    ++moved;
                    continue;
                }

                ec.clear();
                std::filesystem::copy_file(entry.path(),
                                           destinationPath,
                                           std::filesystem::copy_options::overwrite_existing,
                                           ec);
                if (!ec)
                {
                    std::filesystem::remove(entry.path(), ec);
                    ++moved;
                }
                ec.clear();
            }
            ec.clear();
        };

        moveRecentFrames(screenshotsDirectory / "sh000_Scene", false);
        moveRecentFrames(screenshotsDirectory, true);

        LogInfo("Capture output finalized: moved=%u destination=%s",
                moved,
                destinationDirectory.string().c_str());
    });
}

void StopCaptureOutputFinalizer()
{
    g_captureOutputFinalizeCancel.store(true);
    if (g_captureOutputFinalizeThread.joinable())
    {
        g_captureOutputFinalizeThread.join();
    }
    g_captureOutputFinalizeCancel.store(false);
}

void WaitCaptureOutputFinalizer()
{
    if (g_captureOutputFinalizeThread.joinable())
    {
        g_captureOutputFinalizeThread.join();
    }
    g_captureOutputFinalizeCancel.store(false);
}
} // namespace RedFrame
