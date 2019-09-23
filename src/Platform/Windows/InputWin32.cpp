#include "InputWin32.h"

std::unique_ptr<Input> Input::s_Instance = std::make_unique<InputWin32>();