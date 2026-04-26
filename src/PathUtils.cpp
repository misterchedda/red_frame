#include "Internal.hpp"

namespace RedFrame
{
std::string TrimCopy(std::string_view aValue)
{
    const auto begin = aValue.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos)
    {
        return {};
    }

    const auto end = aValue.find_last_not_of(" \t\r\n");
    return std::string(aValue.substr(begin, end - begin + 1));
}

std::optional<std::string> ReadEnvironmentVariable(const char* aName)
{
    const auto required = GetEnvironmentVariableA(aName, nullptr, 0);
    if (required == 0)
    {
        return std::nullopt;
    }

    std::string value(required, '\0');
    const auto written = GetEnvironmentVariableA(aName, value.data(), required);
    if (written == 0 || written >= required)
    {
        return std::nullopt;
    }

    value.resize(written);
    return value;
}

bool ParseEnvironmentBool(std::string_view aValue, const bool aFallback)
{
    const auto normalized = TrimCopy(aValue);
    if (normalized.empty())
    {
        return aFallback;
    }

    if (normalized == "1" || normalized == "true" || normalized == "TRUE" || normalized == "yes" ||
        normalized == "YES" || normalized == "on" || normalized == "ON")
    {
        return true;
    }

    if (normalized == "0" || normalized == "false" || normalized == "FALSE" || normalized == "no" ||
        normalized == "NO" || normalized == "off" || normalized == "OFF")
    {
        return false;
    }

    return aFallback;
}

float ParseEnvironmentFloat(std::string_view aValue, const float aFallback)
{
    const auto normalized = TrimCopy(aValue);
    if (normalized.empty())
    {
        return aFallback;
    }

    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(normalized.c_str(), &end);
    if (end == normalized.c_str() || *end != '\0' || errno != 0)
    {
        return aFallback;
    }

    return static_cast<float>(parsed);
}

std::int32_t ParseEnvironmentInt32(std::string_view aValue, const std::int32_t aFallback)
{
    const auto normalized = TrimCopy(aValue);
    if (normalized.empty())
    {
        return aFallback;
    }

    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtol(normalized.c_str(), &end, 0);
    if (end == normalized.c_str() || *end != '\0' || errno != 0 || parsed < std::numeric_limits<std::int32_t>::min() ||
        parsed > std::numeric_limits<std::int32_t>::max())
    {
        return aFallback;
    }

    return static_cast<std::int32_t>(parsed);
}


CaptureOutputDirectory MakeCaptureOutputDirectory()
{
    const auto now = std::chrono::system_clock::now();
    const auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#if defined(_MSC_VER)
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    const auto sessionIndex = ++g_captureSessionIndex;
    const auto hhmmss = static_cast<std::uint32_t>((localTime.tm_hour * 10000) + (localTime.tm_min * 100) +
                                                   localTime.tm_sec);
    const auto directoryIndex = static_cast<std::uint32_t>(((localTime.tm_yday + 1) * 1000000) + hhmmss);
    char directoryName[80]{};
    sprintf_s(directoryName,
              "RedFrame_%04d%02d%02d_%02d%02d%02d_%03d",
              localTime.tm_year + 1900,
              localTime.tm_mon + 1,
              localTime.tm_mday,
              localTime.tm_hour,
              localTime.tm_min,
              localTime.tm_sec,
              sessionIndex);
    return {directoryName, directoryIndex};
}

std::filesystem::path GetGameScreenshotsDirectory()
{
    if (const char* userProfile = std::getenv("USERPROFILE"))
    {
        return std::filesystem::path(userProfile) / "Documents" / "CD Projekt Red" / "Cyberpunk 2077" / "screenshots";
    }

    return std::filesystem::path{};
}

std::filesystem::path GetRedFrameOutputDirectory()
{
    const auto screenshotsDirectory = GetGameScreenshotsDirectory();
    if (screenshotsDirectory.empty())
    {
        return {};
    }

    return screenshotsDirectory / "RedFrame";
}

std::string ToLowerCopy(std::string aValue)
{
    std::transform(aValue.begin(), aValue.end(), aValue.begin(), [](const unsigned char aChar) {
        return static_cast<char>(std::tolower(aChar));
    });
    return aValue;
}

bool IsReservedWindowsDeviceName(const std::string& aName)
{
    auto stem = aName;
    if (const auto dot = stem.find('.'); dot != std::string::npos)
    {
        stem.resize(dot);
    }

    stem = ToLowerCopy(stem);
    if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul")
    {
        return true;
    }

    if (stem.size() == 4 && (stem.starts_with("com") || stem.starts_with("lpt")) && stem[3] >= '1' && stem[3] <= '9')
    {
        return true;
    }

    return false;
}

bool IsSafeRelativePathComponent(const std::filesystem::path& aComponent)
{
    const auto value = aComponent.string();
    if (value.empty() || value == "." || value == ".." || IsReservedWindowsDeviceName(value))
    {
        return false;
    }

    for (const auto ch : value)
    {
        if (static_cast<unsigned char>(ch) < 0x20 || ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '|' ||
            ch == '?' || ch == '*')
        {
            return false;
        }
    }

    return true;
}

std::optional<std::filesystem::path> ResolvePublicScreenshotPath(const RED4ext::CString& aRequestedPath)
{
    return ResolvePublicScreenshotPath(aRequestedPath, RED4ext::ESaveFormat::SF_PNG);
}

std::optional<std::filesystem::path> ResolvePublicScreenshotPath(const RED4ext::CString& aRequestedPath, const RED4ext::ESaveFormat aSaveFormat)
{
    auto requestedPath = std::filesystem::path(TrimCopy(aRequestedPath.c_str()));
    if (requestedPath.empty() || requestedPath.is_absolute() || requestedPath.has_root_name() ||
        requestedPath.has_root_directory() || !requestedPath.has_filename())
    {
        return std::nullopt;
    }

    for (const auto& component : requestedPath)
    {
        if (!IsSafeRelativePathComponent(component))
        {
            return std::nullopt;
        }
    }

    if (!requestedPath.has_extension())
    {
        requestedPath.replace_extension(aSaveFormat == RED4ext::ESaveFormat::SF_EXR ? ".exr" : ".png");
    }
    else
    {
        const auto extension = ToLowerCopy(requestedPath.extension().string());
        if (aSaveFormat == RED4ext::ESaveFormat::SF_EXR)
        {
            if (extension != ".exr")
            {
                return std::nullopt;
            }
        }
        else if (extension != ".png")
        {
            return std::nullopt;
        }
    }

    const auto root = GetRedFrameOutputDirectory();
    if (root.empty())
    {
        return std::nullopt;
    }

    return root / requestedPath.lexically_normal();
}

std::optional<std::filesystem::path> ResolvePublicAdvancedScreenshotPath(const RED4ext::CString& aRequestedPath)
{
    auto requestedPath = std::filesystem::path(TrimCopy(aRequestedPath.c_str()));
    if (requestedPath.empty() || requestedPath.is_absolute() || requestedPath.has_root_name() ||
        requestedPath.has_root_directory() || !requestedPath.has_filename())
    {
        return std::nullopt;
    }

    for (const auto& component : requestedPath)
    {
        if (!IsSafeRelativePathComponent(component))
        {
            return std::nullopt;
        }
    }

    const auto root = GetRedFrameOutputDirectory();
    if (root.empty())
    {
        return std::nullopt;
    }

    return root / requestedPath.lexically_normal();
}

std::optional<std::filesystem::path> ResolvePublicAudioPath(const RED4ext::CString& aRequestedPath)
{
    auto requestedPath = std::filesystem::path(TrimCopy(aRequestedPath.c_str()));
    if (requestedPath.empty() || requestedPath.is_absolute() || requestedPath.has_root_name() ||
        requestedPath.has_root_directory() || !requestedPath.has_filename())
    {
        return std::nullopt;
    }

    for (const auto& component : requestedPath)
    {
        if (!IsSafeRelativePathComponent(component))
        {
            return std::nullopt;
        }
    }

    if (!requestedPath.has_extension())
    {
        requestedPath.replace_extension(".wav");
    }
    else if (ToLowerCopy(requestedPath.extension().string()) != ".wav")
    {
        return std::nullopt;
    }

    const auto root = GetRedFrameOutputDirectory();
    if (root.empty())
    {
        return std::nullopt;
    }

    return root / requestedPath.lexically_normal();
}

} // namespace RedFrame
