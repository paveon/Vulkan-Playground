#include "EntryPoint.h"
#include "Input.h"
#include <Engine/Core.h>
#include <thread>

int main(int, char**) {
   std::cout << currentTime() << "[Engine] Initializing..." << std::endl;
   int result = EXIT_SUCCESS;
   try {
      Input::InitInputSubsystem();

      std::unique_ptr<Application> app(Application::CreateApplication());
      app->Run();
   }
   catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
      result = EXIT_FAILURE;
   }

   TerminateGLFW();
   std::cout << currentTime() << "[Engine] Finalizing..." << std::endl;
   return result;
}

