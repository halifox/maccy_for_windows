#include "flutter_window.h"

#include <optional>
#include <vector>
#include <windows.h>
#include <dwmapi.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

#include "flutter/generated_plugin_registrant.h"

#pragma comment(lib, "dwmapi.lib")

// Windows 11 backdrop attribute (if not in SDK)
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

FlutterWindow::FlutterWindow(const flutter::DartProject& project)
    : project_(project) {}

FlutterWindow::~FlutterWindow() {}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);
  
  if (!flutter_controller_->engine() || !flutter_controller_->view()) {
    return false;
  }
  RegisterPlugins(flutter_controller_->engine());

  // Register MethodChannel
  auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      flutter_controller_->engine()->messenger(), "com.hali.clip/native_utils",
      &flutter::StandardMethodCodec::GetInstance());

  channel->SetMethodCallHandler(
      [this](const flutter::MethodCall<flutter::EncodableValue>& call,
             std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        if (call.method_name().compare("recordActiveApp") == 0) {
          this->RecordActiveApp();
          result->Success();
        } else if (call.method_name().compare("restoreAndPaste") == 0) {
          this->RestoreAndPaste();
          result->Success();
        } else if (call.method_name().compare("simulatePaste") == 0) {
          this->SimulatePaste();
          result->Success();
        } else {
          result->NotImplemented();
        }
      });

  SetChildContent(flutter_controller_->view()->GetNativeWindow());
  flutter_controller_->ForceRedraw();

  // Enable acrylic blur effect for Windows 10/11
  EnableBlurEffect();

  return true;
}

void FlutterWindow::EnableBlurEffect() {
  HWND hwnd = GetHandle();

  // Try Windows 11 backdrop first (value 3 = DWMSBT_TRANSIENTWINDOW)
  int backdropType = 3;
  HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));

  if (FAILED(hr)) {
    // Fallback to Windows 10 blur behind
    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    DwmEnableBlurBehindWindow(hwnd, &bb);
    if (bb.hRgnBlur) {
      DeleteObject(bb.hRgnBlur);
    }
  }
}

void FlutterWindow::RecordActiveApp() {
  HWND foreground = GetForegroundWindow();
  if (foreground != GetHandle()) {
    last_active_window_ = foreground;
  }
}

void FlutterWindow::RestoreAndPaste() {
  if (last_active_window_ && IsWindow(last_active_window_)) {
    // 1. Restore window if minimized and bring to front
    if (IsIconic(last_active_window_)) {
      ShowWindow(last_active_window_, SW_RESTORE);
    }
    SetForegroundWindow(last_active_window_);

    // 2. Wait for focus to switch (max 200ms)
    int attempts = 20;
    while (GetForegroundWindow() != last_active_window_ && attempts > 0) {
      Sleep(10);
      attempts--;
    }

    // 3. Prepare input sequence
    std::vector<INPUT> inputs;
    auto AddKey = [&](WORD vk, bool up) {
      INPUT input = {0};
      input.type = INPUT_KEYBOARD;
      input.ki.wVk = vk;
      if (up) input.ki.dwFlags = KEYEVENTF_KEYUP;
      inputs.push_back(input);
    }; // Added missing semicolon here

    // 4. Release physical modifiers to avoid combos like Ctrl+Alt+V
    if (GetKeyState(VK_SHIFT) & 0x8000) AddKey(VK_SHIFT, true);
    if (GetKeyState(VK_MENU) & 0x8000) AddKey(VK_MENU, true);
    if (GetKeyState(VK_LWIN) & 0x8000) AddKey(VK_LWIN, true);
    if (GetKeyState(VK_RWIN) & 0x8000) AddKey(VK_RWIN, true);

    // 5. Simulate Ctrl+V
    AddKey(VK_CONTROL, false);
    AddKey('V', false);
    AddKey('V', true);
    AddKey(VK_CONTROL, true);

    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
  }
}

void FlutterWindow::SimulatePaste() {
  std::vector<INPUT> inputs;
  auto AddKey = [&](WORD vk, bool up) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    if (up) input.ki.dwFlags = KEYEVENTF_KEYUP;
    inputs.push_back(input);
  };

  // Simulate Ctrl+V
  AddKey(VK_CONTROL, false);
  AddKey('V', false);
  AddKey('V', true);
  AddKey(VK_CONTROL, true);

  SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

void FlutterWindow::OnDestroy() {
  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }

  Win32Window::OnDestroy();
}

LRESULT
FlutterWindow::MessageHandler(HWND hwnd, UINT const message,
                              WPARAM const wparam,
                              LPARAM const lparam) noexcept {
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam,
                                                      lparam);
    if (result) {
      return *result;
    }
  }

  switch (message) {
    case WM_FONTCHANGE:
      flutter_controller_->engine()->ReloadSystemFonts();
      break;
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}