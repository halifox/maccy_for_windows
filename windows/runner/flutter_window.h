#ifndef RUNNER_FLUTTER_WINDOW_H_
#define RUNNER_FLUTTER_WINDOW_H_

#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>

#include <memory>

#include "win32_window.h"

// A window that does nothing but host a Flutter view.
class FlutterWindow : public Win32Window {
 public:
  explicit FlutterWindow(const flutter::DartProject& project);
  virtual ~FlutterWindow();

 protected:
  // Win32Window:
  bool OnCreate() override;
  void OnDestroy() override;
  LRESULT MessageHandler(HWND hwnd, UINT const message, WPARAM const wparam,
                         LPARAM const lparam) noexcept override;

 private:
  // The project to run.
  flutter::DartProject project_;

  // The flutter controller for this window.
  std::unique_ptr<flutter::FlutterViewController> flutter_controller_;

  // 记录上一个活动窗口的句柄
  HWND last_active_window_ = nullptr;
  void RecordActiveApp();
  void RestoreAndPaste();
  void EnableBlurEffect();
};

#endif  // RUNNER_FLUTTER_WINDOW_H_
