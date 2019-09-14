#include "EntryPoint.h"
#include <Engine/Core.h>
#include <thread>

int main(int argc, char* argv[]) {
   std::cout << currentTime() << "[Engine] Initializing..." << std::endl;
   int result = EXIT_SUCCESS;
   try {
      std::unique_ptr<Application> app(CreateApplication());
      app->Run();
   }
   catch (const std::exception& e) {
      std::cout << "[Exception] " << e.what() << std::endl;
      result = EXIT_FAILURE;
   }

   TerminateGLFW();
   std::cout << currentTime() << "[Engine] Finalizing..." << std::endl;
   return result;
}

