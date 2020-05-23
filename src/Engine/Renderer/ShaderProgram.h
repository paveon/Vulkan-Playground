#ifndef VULKAN_SHADERPROGRAM_H
#define VULKAN_SHADERPROGRAM_H


#include <memory>

class ShaderProgram {
protected:
   ShaderProgram() = default;

public:
   static auto Create(const char* filepath) -> std::unique_ptr<ShaderProgram>;
};


#endif //VULKAN_SHADERPROGRAM_H
