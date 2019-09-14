#ifndef VULKAN_ENTRYPOINT_H
#define VULKAN_ENTRYPOINT_H

#include <memory>
#include <Engine/Application.h>

extern std::unique_ptr<Application> CreateApplication();

int main(int argc, char* argv[]);

#endif //VULKAN_ENTRYPOINT_H
