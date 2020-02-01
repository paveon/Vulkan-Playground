#include "Input.h"
#include "Platform/Windows/InputWin32.h"

std::unique_ptr<Input> Input::s_Instance;

void Input::InitInputSubsystem() {
#if defined(_WIN32)
   s_Instance = std::make_unique<InputWin32>();
#else
   throw std::runtime_error("Could not initialize input subsystem due to unsupported OS");
#endif
}