#ifndef VULKAN_SHADERPROGRAM_H
#define VULKAN_SHADERPROGRAM_H


#include <memory>

class ShaderProgram {
protected:
   ShaderProgram() = default;

public:
   static std::unique_ptr<ShaderProgram> Create(const char* filepath);
};


#endif //VULKAN_SHADERPROGRAM_H
