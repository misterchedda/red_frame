module RedFrame

public native class Screenshot {
  public static native func Take(outputPath: String, mode: Int32, saveFormat: Int32, resolution: Int32, resolutionMultiplier: Int32, forceLOD0: Bool) -> Int32
  public static native func GetRequestStatus(requestId: Int32) -> Int32
  public static native func GetRequestError(requestId: Int32) -> Int32
  public static native func GetRequestPath(requestId: Int32) -> String
  public static native func GetRequestPathCount(requestId: Int32) -> Int32
  public static native func GetRequestPathAt(requestId: Int32, index: Int32) -> String
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
