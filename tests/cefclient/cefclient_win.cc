// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <windows.h>

#include <memory>

#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "tests/cefclient/browser/main_context_impl.h"
#include "tests/cefclient/browser/main_message_loop_multithreaded_win.h"
#include "tests/cefclient/browser/resource.h"
#include "tests/cefclient/browser/root_window_manager.h"
#include "tests/cefclient/browser/test_runner.h"
#include "tests/shared/browser/client_app_browser.h"
#include "tests/shared/browser/main_message_loop_external_pump.h"
#include "tests/shared/browser/main_message_loop_std.h"
#include "tests/shared/common/client_app_other.h"
#include "tests/shared/common/client_switches.h"
#include "tests/shared/renderer/client_app_renderer.h"

namespace client {
namespace {

int RunMain(HINSTANCE hInstance, int nCmdShow, void* sandbox_info) {
  CefMainArgs main_args(hInstance);

  // Parse command-line arguments.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromString(::GetCommandLineW());

  // Create a ClientApp of the correct type.
  CefRefPtr<CefApp> app;
  ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
  if (process_type == ClientApp::BrowserProcess) {
    app = new ClientAppBrowser();
  } else if (process_type == ClientApp::RendererProcess) {
    app = new ClientAppRenderer();
  } else if (process_type == ClientApp::OtherProcess) {
    app = new ClientAppOther();
  }

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
  if (exit_code >= 0) {
    return exit_code;
  }

  // Create the main context object.
  auto context = std::make_unique<MainContextImpl>(command_line, true);

  CefSettings settings;

  if (!sandbox_info) {
    settings.no_sandbox = true;
  }

  // Populate the settings based on command line arguments.
  context->PopulateSettings(&settings);

  // Set the ID for the ICON resource that will be loaded from the main
  // executable and used when creating default Chrome windows such as DevTools
  // and Task Manager. Only used with the Chrome runtime.
  settings.chrome_app_icon_id = IDR_MAINFRAME;

  // Create the main message loop object.
  std::unique_ptr<MainMessageLoop> message_loop;
  if (settings.multi_threaded_message_loop) {
    message_loop = std::make_unique<MainMessageLoopMultithreadedWin>();
  } else if (settings.external_message_pump) {
    message_loop = MainMessageLoopExternalPump::Create();
  } else {
    message_loop = std::make_unique<MainMessageLoopStd>();
  }

  // Initialize the CEF browser process. May return false if initialization
  // fails or if early exit is desired (for example, due to process singleton
  // relaunch behavior).
  if (!context->Initialize(main_args, settings, app, sandbox_info)) {
    return CefGetExitCode();
  }

  // Register scheme handlers.
  test_runner::RegisterSchemeHandlers();

  auto window_config = std::make_unique<RootWindowConfig>();
  window_config->always_on_top =
      command_line->HasSwitch(switches::kAlwaysOnTop);
  window_config->with_osr =
      settings.windowless_rendering_enabled ? true : false;

  // Create the first window.
  context->GetRootWindowManager()->CreateRootWindow(std::move(window_config));

  // Run the message loop. This will block until Quit() is called by the
  // RootWindowManager after all windows have been destroyed.
  int result = message_loop->Run();

  // Shut down CEF.
  context->Shutdown();

  // Release objects in reverse order of creation.
  message_loop.reset();
  context.reset();

  return result;
}

}  // namespace
}  // namespace client

#if defined(CEF_USE_BOOTSTRAP)

// Entry point called by bootstrap.exe when built as a DLL.
CEF_BOOTSTRAP_EXPORT int RunWinMain(HINSTANCE hInstance,
                                    LPTSTR lpCmdLine,
                                    int nCmdShow,
                                    void* sandbox_info) {
  return client::RunMain(hInstance, nCmdShow, sandbox_info);
}

#else  // !defined(CEF_USE_BOOTSTRAP)

// Program entry point function.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

#if defined(ARCH_CPU_32_BITS)
  // Run the main thread on 32-bit Windows using a fiber with the preferred 4MiB
  // stack size. This function must be called at the top of the executable entry
  // point function (`main()` or `wWinMain()`). It is used in combination with
  // the initial stack size of 0.5MiB configured via the `/STACK:0x80000` linker
  // flag on executable targets. This saves significant memory on threads (like
  // those in the Windows thread pool, and others) whose stack size can only be
  // controlled via the linker flag.
  int exit_code = CefRunWinMainWithPreferredStackSize(wWinMain, hInstance,
                                                      lpCmdLine, nCmdShow);
  if (exit_code >= 0) {
    // The fiber has completed so return here.
    return exit_code;
  }
#endif

  void* sandbox_info = nullptr;

#if defined(CEF_USE_SANDBOX)
  // Manage the life span of the sandbox information object. This is necessary
  // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  return client::RunMain(hInstance, nCmdShow, sandbox_info);
}

#endif  // !defined(CEF_USE_BOOTSTRAP)
