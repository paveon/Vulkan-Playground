#include "Input.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    #include "Platform/Windows/InputWin32.h"
#elif defined(__linux__)
    #include "Platform/Linux/InputLinux.h"
#elif defined(__APPLE__)
    #include "Platform/MacOS/InputMacOS.h"
#endif

std::unique_ptr<Input> Input::s_Instance;

void Input::InitInputSubsystem() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    s_Instance = std::make_unique<InputWin32>();
#elif defined(__linux__)
    s_Instance = std::make_unique<InputLinux>();
#elif defined(__APPLE__)
    s_Instance = std::make_unique<InputMacOS>();
#else
    throw std::runtime_error("Could not initialize input subsystem due to unsupported OS");
#endif
}