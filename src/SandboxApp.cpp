#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <Engine/Include/Engine.h>


class SandboxApp : public Application {
public:
    SandboxApp() = default;

    ~SandboxApp() override = default;

    bool OnKeyPress(const KeyPressEvent& e) override {
       std::cout << "[Sandbox] " << e << std::endl;
       return true;
    }
};


std::unique_ptr<Application> CreateApplication() { return std::make_unique<SandboxApp>(); }