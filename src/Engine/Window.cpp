#include "Window.h"
#include "Platform/Windows/WindowWin32.h"

std::unique_ptr<Window> Window::Create(uint32_t width, uint32_t height, const char* title) {
#if defined(_WIN32)
   return std::make_unique<WindowWin32>(width, height, title);
#else
   throw std::runtime_error("Could not create window due to unsupported OS");
#endif


}