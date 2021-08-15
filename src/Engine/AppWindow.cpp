#include "AppWindow.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include "Platform/Windows/WindowWin32.h"
#elif defined(__linux__)
#include "Platform/Linux/WindowLinux.h"
#endif

auto AppWindow::Create(uint32_t width, uint32_t height, const char* title) -> std::unique_ptr<AppWindow> {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
   return std::make_unique<WindowWin32>(width, height, title);
#elif defined(__linux__)
  return std::make_unique<WindowLinux>(width, height, title);
#else
   throw std::runtime_error("Could not create window due to unsupported OS");
#endif


}