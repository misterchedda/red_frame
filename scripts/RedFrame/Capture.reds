module RedFrame

public native class Screenshot {
  public static native func Take(outputPath: String, mode: rendScreenshotMode, saveFormat: ESaveFormat, resolution: renddimEPreset, resolutionMultiplier: rendResolutionMultiplier, forceLOD0: Bool) -> Int32
  public static native func GetRequestStatus(requestId: Int32) -> Int32
  public static native func GetRequestError(requestId: Int32) -> Int32
  public static native func GetRequestPath(requestId: Int32) -> String
  public static native func GetRequestPathCount(requestId: Int32) -> Int32
  public static native func GetRequestPathAt(requestId: Int32, index: Int32) -> String
  public static native func RegisterListener(target: ref<IScriptable>, functionName: CName) -> Int32
  public static native func UnregisterListener(listenerId: Int32) -> Bool
  public static native func GetLastError() -> Int32
}

public native func RedFrameScreenshotPump()

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

public class RedFrameScreenshotPumpCallback extends DelayCallback {
  protected let m_service: wref<RedFrameScreenshotService>;
  protected let m_token: Int32;

  public static func Create(service: wref<RedFrameScreenshotService>, token: Int32) -> ref<RedFrameScreenshotPumpCallback> {
    let self: ref<RedFrameScreenshotPumpCallback> = new RedFrameScreenshotPumpCallback();
    self.m_service = service;
    self.m_token = token;
    return self;
  }

  public func Call() {
    if IsDefined(this.m_service) {
      this.m_service.OnPump(this.m_token);
    }
  }
}

public class RedFrameScreenshotService extends ScriptableService {
  private let m_sessionReady: Bool;
  private let m_pumpPending: Bool;
  private let m_token: Int32;

  private cb func OnLoad() {
    let callbackSystem = GameInstance.GetCallbackSystem();
    callbackSystem.RegisterCallback(n"Session/Ready", this, n"OnSessionReady");
    callbackSystem.RegisterCallback(n"Session/End", this, n"OnSessionEnd");
  }

  private cb func OnInitialize() {
    this.SchedulePump();
  }

  private cb func OnUninitialize() {
    this.m_sessionReady = false;
    this.m_pumpPending = false;
    this.m_token += 1;
  }

  private cb func OnSessionReady(event: ref<GameSessionEvent>) {
    this.m_sessionReady = true;
    this.SchedulePump();
  }

  private cb func OnSessionEnd(event: ref<GameSessionEvent>) {
    this.m_sessionReady = false;
    this.m_pumpPending = false;
    this.m_token += 1;
  }

  private func SchedulePump() {
    if !this.m_sessionReady || this.m_pumpPending {
      return;
    }

    this.m_pumpPending = true;
    GameInstance.GetDelaySystem(GetGameInstance()).DelayCallback(
      RedFrameScreenshotPumpCallback.Create(this, this.m_token),
      0.25,
      false
    );
  }

  public func OnPump(token: Int32) {
    if token != this.m_token || !this.m_sessionReady {
      return;
    }

    this.m_pumpPending = false;
    RedFrameScreenshotPump();
    this.SchedulePump();
  }
}
