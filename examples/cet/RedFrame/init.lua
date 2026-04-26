local modName = "RedFrame"
local modVersion = "0.1.0"
local logPrefix = "[" .. modName .. "] "

local overlayOpen = false
local buttonWidth = 150

local state = {
    fps = 0,
    screenshotMode = 1,
    saveFormat = 2,
    resolution = 5,
    resolutionMultiplier = 1,
    forceLOD0 = false,
    status = "Waiting for input.",
    audioActive = false,
    frameDumpActive = false,
    frameCount = 0,
    screenshotRequestId = 0,
    screenshotRequestStatus = 0,
    screenshotRequestError = 0,
    screenshotPathCount = 0,
    screenshotPrimaryPath = "",
    screenshotError = 0,
    audioError = 0,
    debugError = 0,
}

local function timestamp()
    return os.date("%Y%m%d_%H%M%S")
end

local function screenshotApi()
    return RedFrameScreenshot
end

local function audioApi()
    return RedFrameAudio
end

local function debugApi()
    return RedFrameDebug
end

local function logInfo(message)
    local line = logPrefix .. message
    print(line)
    if spdlog and spdlog.info then
        spdlog.info(line)
    end
end

local function logError(message)
    local line = logPrefix .. message
    print(line)
    if spdlog and spdlog.error then
        spdlog.error(line)
    end
end

local function hasScreenshotBindings()
    local api = screenshotApi()
    return api ~= nil
        and type(api.Take) == "function"
        and type(api.GetRequestStatus) == "function"
        and type(api.GetRequestError) == "function"
        and type(api.GetRequestPath) == "function"
        and type(api.GetRequestPathCount) == "function"
        and type(api.GetLastError) == "function"
end

local function hasAudioBindings()
    local api = audioApi()
    return api ~= nil
        and type(api.Start) == "function"
        and type(api.Stop) == "function"
        and type(api.IsActive) == "function"
        and type(api.GetLastError) == "function"
end

local function hasDebugBindings()
    local api = debugApi()
    return api ~= nil
        and type(api.StartFrameDump) == "function"
        and type(api.StopFrameDump) == "function"
        and type(api.IsFrameDumpActive) == "function"
        and type(api.GetFrameCount) == "function"
        and type(api.GetLastError) == "function"
end

local function refreshState()
    if hasAudioBindings() then
        state.audioActive = audioApi().IsActive()
        state.audioError = audioApi().GetLastError()
    else
        state.audioActive = false
    end

    if hasDebugBindings() then
        state.frameDumpActive = debugApi().IsFrameDumpActive()
        state.frameCount = debugApi().GetFrameCount()
        state.debugError = debugApi().GetLastError()
    else
        state.frameDumpActive = false
    end

    if hasScreenshotBindings() then
        state.screenshotError = screenshotApi().GetLastError()
        if state.screenshotRequestId > 0 then
            state.screenshotRequestStatus = screenshotApi().GetRequestStatus(state.screenshotRequestId)
            state.screenshotRequestError = screenshotApi().GetRequestError(state.screenshotRequestId)
            state.screenshotPathCount = screenshotApi().GetRequestPathCount(state.screenshotRequestId)
            state.screenshotPrimaryPath = screenshotApi().GetRequestPath(state.screenshotRequestId)
        end
    end
end

local function setStatus(message, isError)
    state.status = message
    if isError then
        logError(message)
    else
        logInfo(message)
    end
end

local function takeScreenshot()
    if not hasScreenshotBindings() then
        setStatus("Screenshot bindings are not available.", true)
        return
    end

    local extension = state.saveFormat == 32 and ".exr" or ".png"
    local outputPath = string.format("CET/take_%s_m%d_f%d_r%d_x%d%s",
        timestamp(),
        state.screenshotMode,
        state.saveFormat,
        state.resolution,
        state.resolutionMultiplier,
        extension)
    local requestId = screenshotApi().Take(outputPath,
        state.screenshotMode,
        state.saveFormat,
        state.resolution,
        state.resolutionMultiplier,
        state.forceLOD0)
    state.screenshotRequestId = requestId
    refreshState()
    setStatus(string.format("%s screenshot request. id=%d status=%d error=%d path=%s",
        requestId > 0 and "Queued" or "Failed to queue",
        requestId,
        state.screenshotRequestStatus,
        state.screenshotError,
        outputPath),
        requestId <= 0)
end

local function startAudio()
    if not hasAudioBindings() then
        setStatus("Audio bindings are not available.", true)
        return
    end

    local outputPath = "CET/audio_" .. timestamp()
    local ok = audioApi().Start(outputPath)
    refreshState()
    setStatus(string.format("%s audio. active=%s error=%d path=%s",
        ok and "Started" or "Failed to start",
        tostring(state.audioActive),
        state.audioError,
        outputPath),
        not ok)
end

local function stopAudio()
    if not hasAudioBindings() then
        setStatus("Audio bindings are not available.", true)
        return
    end

    local ok = audioApi().Stop()
    refreshState()
    setStatus(string.format("%s audio. active=%s error=%d",
        ok and "Stopped" or "Failed to stop",
        tostring(state.audioActive),
        state.audioError),
        not ok)
end

local function startFrameDump()
    if not hasDebugBindings() then
        setStatus("Debug bindings are not available.", true)
        return
    end

    local ok = debugApi().StartFrameDump(state.fps)
    refreshState()
    setStatus(string.format("%s frame dump. active=%s frames=%d error=%d fps=%d",
        ok and "Started" or "Failed to start",
        tostring(state.frameDumpActive),
        state.frameCount,
        state.debugError,
        state.fps),
        not ok)
end

local function stopFrameDump()
    if not hasDebugBindings() then
        setStatus("Debug bindings are not available.", true)
        return
    end

    local ok = debugApi().StopFrameDump()
    refreshState()
    setStatus(string.format("%s frame dump. active=%s frames=%d error=%d",
        ok and "Stopped" or "Failed to stop",
        tostring(state.frameDumpActive),
        state.frameCount,
        state.debugError),
        not ok)
end

local function drawWindow()
    if not ImGui.Begin("RedFrame") then
        ImGui.End()
        return
    end

    if ImGui.IsWindowCollapsed() then
        ImGui.End()
        return
    end

    refreshState()

    ImGui.Separator()

    ImGui.Text("Screenshot")
    state.screenshotMode = ImGui.InputInt("Mode", state.screenshotMode, 1, 1)
    state.saveFormat = ImGui.InputInt("Format", state.saveFormat, 1, 1)
    state.resolution = ImGui.InputInt("Resolution", state.resolution, 1, 1)
    state.resolutionMultiplier = ImGui.InputInt("Multiplier", state.resolutionMultiplier, 1, 1)
    state.forceLOD0 = ImGui.Checkbox("Force LOD0", state.forceLOD0)

    if ImGui.Button("Set PNG", buttonWidth, 0) then
        state.screenshotMode = 1
        state.saveFormat = 2
        state.resolution = 5
        state.resolutionMultiplier = 1
        setStatus("PNG preset selected.", false)
    end

    ImGui.SameLine()
    if ImGui.Button("Set EXR", buttonWidth, 0) then
        state.screenshotMode = 5
        state.saveFormat = 32
        state.resolution = 5
        state.resolutionMultiplier = 1
        setStatus("EXR preset selected.", false)
    end

    ImGui.SameLine()
    if ImGui.Button("Take", buttonWidth, 0) then
        takeScreenshot()
    end

    ImGui.Separator()

    if ImGui.Button("Start Audio", buttonWidth, 0) then
        startAudio()
    end

    ImGui.SameLine()
    if ImGui.Button("Stop Audio", buttonWidth, 0) then
        stopAudio()
    end

    ImGui.Separator()

    state.fps = ImGui.InputInt("Frame dump FPS (0 auto)", state.fps, 1, 10)

    if ImGui.Button("Start Dump", buttonWidth, 0) then
        startFrameDump()
    end

    ImGui.SameLine()
    if ImGui.Button("Stop Dump", buttonWidth, 0) then
        stopFrameDump()
    end

    ImGui.SameLine()
    if ImGui.Button("Refresh", buttonWidth, 0) then
        refreshState()
        setStatus(string.format("Refreshed. audio=%s dump=%s frames=%d",
            tostring(state.audioActive),
            tostring(state.frameDumpActive),
            state.frameCount), false)
    end

    ImGui.Separator()
    ImGui.Text("Audio active: " .. tostring(state.audioActive))
    ImGui.Text("Frame dump active: " .. tostring(state.frameDumpActive))
    ImGui.Text("Dumped frames: " .. tostring(state.frameCount))
    ImGui.Text("Screenshot request: id=" .. tostring(state.screenshotRequestId)
        .. " status=" .. tostring(state.screenshotRequestStatus)
        .. " error=" .. tostring(state.screenshotRequestError)
        .. " paths=" .. tostring(state.screenshotPathCount))
    ImGui.TextWrapped("Primary path: " .. tostring(state.screenshotPrimaryPath))
    ImGui.Text("Errors: screenshot=" .. tostring(state.screenshotError)
        .. " audio=" .. tostring(state.audioError)
        .. " debug=" .. tostring(state.debugError))
    ImGui.TextWrapped("Status: " .. state.status)
    ImGui.TextWrapped("Output: Documents\\CD Projekt Red\\Cyberpunk 2077\\screenshots\\RedFrame\\")

    ImGui.End()
end

registerForEvent("onInit", function()
    logInfo(modVersion
        .. " initialized. screenshot=" .. tostring(hasScreenshotBindings())
        .. " audio=" .. tostring(hasAudioBindings())
        .. " debug=" .. tostring(hasDebugBindings()))
end)

registerForEvent("onOverlayOpen", function()
    overlayOpen = true
end)

registerForEvent("onOverlayClose", function()
    overlayOpen = false
end)

registerForEvent("onDraw", function()
    if overlayOpen then
        drawWindow()
    end
end)

registerForEvent("onShutdown", function()
    if hasDebugBindings() and debugApi().IsFrameDumpActive() then
        debugApi().StopFrameDump()
    end
    if hasAudioBindings() and audioApi().IsActive() then
        audioApi().Stop()
    end
    logInfo("shutting down")
end)
