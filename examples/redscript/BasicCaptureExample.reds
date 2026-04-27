import RedFrame.Audio
import RedFrame.Debug
import RedFrame.Screenshot

public class RedFrameBasicCaptureExample {
  public static func CaptureOneScreenshot() -> Int32 {
    return Screenshot.Take("Examples/example", rendScreenshotMode.NORMAL, ESaveFormat.SF_PNG, renddimEPreset._1280x720, rendResolutionMultiplier.X1);
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

public class RedFrameScreenshotListenerExample {
  private let m_listenerId: Int32;

  public func Register() -> Bool {
    this.m_listenerId = Screenshot.RegisterListener(this, n"OnScreenshotFinished");
    return this.m_listenerId > 0;
  }

  public func Unregister() -> Bool {
    return Screenshot.UnregisterListener(this.m_listenerId);
  }

  public func CaptureOneScreenshot() -> Int32 {
    return Screenshot.Take("Examples/listener_example", rendScreenshotMode.NORMAL, ESaveFormat.SF_PNG, renddimEPreset._1280x720, rendResolutionMultiplier.X1);
  }

  public func OnScreenshotFinished(requestId: Int32, status: Int32, error: Int32) -> Void {
    let primaryPath = Screenshot.GetRequestPath(requestId);
    let outputCount = Screenshot.GetRequestPathCount(requestId);
  }
}
