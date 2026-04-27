# RedFrame

RedFrame is a RED4ext plugin for Cyberpunk 2077 screenshot and audio capture.

> [!NOTE]
> RedFrame is in alpha. API may break between builds.

Current game version supported: 2.31.

## Features

Working today:

- sandboxed screenshot requests through the native TakeScreenshot path
- experimental WAV audio capture through Wwise output buffers
- debug viewport frame dumping to PNG sequences
- CET overlay example for manual testing

RedFrame does not encode, mux, or stitch frames into video files.

## Install

For release builds, extract `RedFrame-X.Y.Z.zip` into the Cyberpunk 2077 game
folder.

Manual install paths:

```text
Cyberpunk 2077\red4ext\plugins\RedFrame\RedFrame.dll
Cyberpunk 2077\r6\scripts\RedFrame\Capture.reds
```

Optional CET UI:

```text
Cyberpunk 2077\bin\x64\plugins\cyber_engine_tweaks\mods\RedFrame\init.lua
```

Examples for redscript and cet usage under `examples/`

## API

```swift
module RedFrame

public native class Screenshot {
  public static native func Take(outputPath: String, mode: rendScreenshotMode, saveFormat: ESaveFormat, resolution: renddimEPreset, resolutionMultiplier: rendResolutionMultiplier, opt forceLOD0: Bool, opt emmMode: EEnvManagerModifier) -> Int32
  public static native func GetRequestStatus(requestId: Int32) -> Int32
  public static native func GetRequestError(requestId: Int32) -> Int32
  public static native func GetRequestPath(requestId: Int32) -> String
  public static native func GetRequestPathCount(requestId: Int32) -> Int32
  public static native func GetRequestPathAt(requestId: Int32, index: Int32) -> String
  public static native func RegisterListener(target: ref<IScriptable>, functionName: CName) -> Int32
  public static native func UnregisterListener(listenerId: Int32) -> Bool
  public static native func GetLastError() -> Int32
}

public native class Audio {
  public static native func Start(outputPath: String) -> Bool
  public static native func Stop() -> Bool
  public static native func IsActive() -> Bool
  public static native func GetLastError() -> Int32
}

public native class Debug {
  public static native func StartFrameDump(fps: Int32) -> Bool
  public static native func StopFrameDump() -> Bool
  public static native func IsFrameDumpActive() -> Bool
  public static native func GetFrameCount() -> Int32
  public static native func GetLastError() -> Int32
}
```

## Screenshots

```swift
import RedFrame.Screenshot

let requestId = Screenshot.Take("MyMod/shot_0001", rendScreenshotMode.NORMAL, ESaveFormat.SF_PNG, renddimEPreset._1280x720, rendResolutionMultiplier.X1);
```

`Take` returns a request id. A value greater than `0` means the request was
queued; `0` means it failed before queueing. Use `GetLastError()` for the
immediate failure reason. You can either poll the request id or register a
listener for terminal request events.

Request status values:

- `0`: invalid or unknown request
- `1`: queued
- `2`: file appeared and is still being observed
- `3`: complete
- `4`: failed
- `5`: timed out

Screenshot paths are relative to:

```text
%USERPROFILE%\Documents\CD Projekt Red\Cyberpunk 2077\screenshots\RedFrame\
```

Subfolders are allowed.

For PNG output:

```swift
Screenshot.Take(
  "MyMod/shot_0001.png",
  rendScreenshotMode.NORMAL,
  ESaveFormat.SF_PNG,
  renddimEPreset._1280x720,
  rendResolutionMultiplier.X1,
  false, // force LOD0
  EEnvManagerModifier.EMM_None
);
```

For EXR output:

```swift
Screenshot.Take(
  "MyMod/shot_0001.exr",
  rendScreenshotMode.HIGH_RESOLUTION,
  ESaveFormat.SF_EXR,
  renddimEPreset._1280x720,
  rendResolutionMultiplier.X1,
  true, // force LOD0
  EEnvManagerModifier.EMM_None
);
```

`Take` passes renderer options through to the engine. RedFrame validates the
output path sandbox, but otherwise exposes the engine enums directly. Unusual
enum combinations may fail, write a different filename/extension than requested,
or use engine fallback behavior.

Known useful values:

- modes: [`rendScreenshotMode`](https://nativedb.red4ext.com/e/4953059295556816) values such as `NORMAL`, `NORMAL_MULTISAMPLE`, `HIGH_RESOLUTION`, `HIGH_RESOLUTION_LAYERED`
- save formats: [`ESaveFormat`](https://nativedb.red4ext.com/e/2730838978865828) values such as `SF_PNG`, `SF_EXR`, `SF_PNG_AND_EXR`
- resolution presets: [`renddimEPreset`](https://nativedb.red4ext.com/e/2483326025215296) values such as `_1280x720`, `_2560x1080`, `_3440x1440`, `_3840x1600`
- multipliers: [`rendResolutionMultiplier`](https://nativedb.red4ext.com/e/7066925478832773) values such as `X1`, `X2`, `X4`
- `forceLOD0`: asks the screenshot renderer to prefer highest LOD assets
- EMM modes: [`EEnvManagerModifier`](https://nativedb.red4ext.com/e/699463329698701) values such as `EMM_None`, `EMM_ClayView`,
  `EMM_SurfaceBaseColor`, `EMM_SurfaceEmissive`, `EMM_Depth`, etc.

`forceLOD0` and `emmMode` are optional. When omitted, they default to `false`
and `EEnvManagerModifier.EMM_None`.

`emmMode` selects the renderer's environment/debug output mode for high-res
captures. Pass `EMM_None` for normal screenshots. Some modes work well, some
produce dark or unexpected buffers in retail, and
`HIGH_RESOLUTION_LAYERED` uses the engine's hardcoded layer set rather than the
single EMM value.

The engine may append suffixes such as `HIGH_RES_EMM_None_None.exr`. Always use
`GetRequestPath(requestId)` after completion to read the primary actual file
path.

Some modes create more than one file. `GetRequestPathCount(requestId)` returns
the number of discovered output files, and `GetRequestPathAt(requestId, index)`
returns each actual path using zero-based indexes. `GetRequestPath(requestId)`
is a convenience alias for `GetRequestPathAt(requestId, 0)`.

Request status becomes complete once RedFrame observes the engine close the
expected output file handles. For multi-output or very large captures, RedFrame
keeps discovering late companion files for a short grace window after
completion, so callers that need every output should enumerate paths after
completion and may poll again a moment later.

Listener callbacks are delivered by RedFrame's bundled redscript pump service
when a request reaches a terminal status: complete, failed, or timed out. The
listener method must accept `(requestId: Int32, status: Int32, error: Int32)`
and return `Void`.

```swift
import RedFrame.Screenshot

public class MyScreenshotListener {
  private let listenerId: Int32;

  public func Start() {
    this.listenerId = Screenshot.RegisterListener(this, n"OnScreenshotFinished");
  }

  public func Stop() {
    Screenshot.UnregisterListener(this.listenerId);
  }

  public func OnScreenshotFinished(requestId: Int32, status: Int32, error: Int32) -> Void {
    if Equals(status, 3) {
      let primaryPath = Screenshot.GetRequestPath(requestId);
      let pathCount = Screenshot.GetRequestPathCount(requestId);
    }
  }
}
```

Observed on Cyberpunk 2077 2.31:

| Options | Output |
| --- | --- |
| `NORMAL, SF_PNG, _1280x720, X1` | PNG beauty frame |
| `NORMAL_MULTISAMPLE, SF_PNG, _1280x720, X1` | PNG with high-res naming suffix |
| `HIGH_RESOLUTION, SF_PNG, _1280x720, X1` | PNG with high-res naming suffix |
| `HIGH_RESOLUTION, SF_EXR, _1280x720, X1` | EXR beauty frame |
| `HIGH_RESOLUTION, SF_PNG_AND_EXR, _1280x720, X1` | PNG and EXR beauty frames |
| `HIGH_RESOLUTION, SF_EXR, _1280x720, X2` | larger EXR output |
| `HIGH_RESOLUTION, SF_EXR, _1280x720, X4` | very large EXR output; about 1 GB in one test scene |
| `HIGH_RESOLUTION_LAYERED, SF_EXR, _1280x720, X1` | multiple layered EXRs, including `None`, `Depth`, `MaskSSAO`, `PureReflectionView`, and `SurfaceEmissive` |

Resolution presets and multipliers are passed through to the engine, but the
final image size can still follow engine/viewport behavior. In one test scene,
PNG output and 1x EXR output followed a 3840x2160 live viewport, while EXR
multipliers increased output size substantially.

## Audio

```swift
import RedFrame.Audio

Audio.Start("MyMod/audio");
Audio.Stop();
```

Audio paths are relative to the same RedFrame screenshot folder.

Current audio output is 48 kHz stereo IEEE float PCM. 

## Debug Frame Dump

```swift
import RedFrame.Debug

Debug.StartFrameDump(10);
Debug.StopFrameDump();
```

`Debug.StartFrameDump` enables the native viewport frame-dump path. It writes
PNG frames under:

```text
%USERPROFILE%\Documents\CD Projekt Red\Cyberpunk 2077\screenshots\RedFrame_YYYYMMDD_HHMMSS_NNN\
```

IMPORTANT: This is heavy and can visibly stall gameplay/script scheduling so it's useless for realtime video recording. This exists mostly in the shape of a capture sink that was used as a diagnostic or automated-scene tool. For mod-owned frame-by-frame recording workflows, prefer `Screenshot.Take`.

`fps` is the requested dump cadence, or `0` to leave the engine/default value. Frame resolution follows the live game viewport.

## Error Codes

`GetLastError()` returns:

- `0`: no error
- `1`: already active
- `3`: native start/stop failed
- `4`: viewport unavailable
- `5`: screenshot failed
- `6`: invalid output path
- `7`: invalid options


## Credits

Thanks to [Rayshader](https://github.com/rayshader/), [Mozzy](https://github.com/Mozz3d/), and [dragonzkiller](https://next.nexusmods.com/profile/dragonzkiller/mods) for screenshot-renderer research, and of course [wopss](https://github.com/wopss) and team for RED4Ext itself.
