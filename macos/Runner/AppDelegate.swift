import Cocoa
import FlutterMacOS

@main
class AppDelegate: FlutterAppDelegate {
  private var lastActiveAppPID: pid_t?

  override func applicationDidFinishLaunching(_ notification: Notification) {
    let controller = mainFlutterWindow?.contentViewController as! FlutterViewController
    let channel = FlutterMethodChannel(name: "com.hali.clip/native_utils", binaryMessenger: controller.engine.binaryMessenger)

    channel.setMethodCallHandler { [weak self] (call, result) in
      switch call.method {
      case "recordActiveApp":
        self?.recordActiveApp()
        result(nil)
      case "restoreAndPaste":
        self?.restoreAndPaste()
        result(nil)
      default:
        result(FlutterMethodNotImplemented)
      }
    }
    super.applicationDidFinishLaunching(notification)
  }

  private func recordActiveApp() {
    // 记录当前除了本应用之外的最前面的应用
    if let frontApp = NSWorkspace.shared.frontmostApplication,
       frontApp.bundleIdentifier != Bundle.main.bundleIdentifier {
        lastActiveAppPID = frontApp.processIdentifier
        print("📍 Native: Recorded active app: \(frontApp.localizedName ?? "Unknown") (PID: \(lastActiveAppPID!))")
    }
  }

  private func restoreAndPaste() {
    guard let pid = lastActiveAppPID,
          let app = NSRunningApplication(processIdentifier: pid) else {
        print("⚠️ Native: No app recorded to restore focus")
        return
    }

    // 1. 恢复焦点
    app.activate(options: .activateIgnoringOtherApps)

    // 2. 稍微延迟一点点确保焦点切换完成
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        self.simulatePaste()
    }
  }

  private func simulatePaste() {
    let source = CGEventSource(stateID: .combinedSessionState)
    
    // Command 键码是 0x37，V 键码是 0x09
    let vKey: UInt16 = 0x09
    
    let keyDown = CGEvent(keyboardEventSource: source, virtualKey: vKey, keyDown: true)
    keyDown?.flags = .maskCommand
    
    let keyUp = CGEvent(keyboardEventSource: source, virtualKey: vKey, keyDown: false)
    keyUp?.flags = .maskCommand
    
    keyDown?.post(tap: .cghidEventTap)
    keyUp?.post(tap: .cghidEventTap)
    
    print("✅ Native: Simulated Command+V sent")
  }

  override func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
    return false
  }

  override func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
    return true
  }
}
