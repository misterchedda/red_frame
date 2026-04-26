import RedFrame.Audio
import RedFrame.Debug
import RedFrame.Screenshot

public class RedFrameBasicCaptureExample {
  public static func CaptureOneScreenshot() -> Int32 {
    return Screenshot.Take("Examples/example", 1, 2, 5, 1, false);
  }

  public static func GetScreenshotRequestStatus(requestId: Int32) -> Int32 {
    return Screenshot.GetRequestStatus(requestId);
  }

  public static func StartAudio() -> Bool {
    return Audio.Start("Examples/audio");
  }

  public static func StopAudio() -> Bool {
    return Audio.Stop();
  }

  public static func StartDebugFrameDump() -> Bool {
    return Debug.StartFrameDump(0);
  }

  public static func StopDebugFrameDump() -> Int32 {
    Debug.StopFrameDump();
    return Debug.GetFrameCount();
  }
}
