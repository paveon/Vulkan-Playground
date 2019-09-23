#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <Engine/Include/Engine.h>

class TestLayer : public Layer {
public:
    explicit TestLayer(const char* name) : Layer(name) {}

    void OnAttach() override  { std::cout << m_DebugName << "::Attached" << std::endl; }

    void OnDetach() override  { std::cout << m_DebugName << "::Detached" << std::endl; }

    bool OnKeyPress(KeyPressEvent& e) override {
       std::cout << m_DebugName << "::" << e << std::endl;
       return true;
    }

    bool OnKeyRelease(KeyReleaseEvent& e) override {
       std::cout << m_DebugName << "::" << e << std::endl;
       return true;
    }

    void OnUpdate() override {
       if (Input::KeyPressed(IO_KEY_A)) {
          std::cout << "A key is pressed" << std::endl;
       }
    }
};


class SandboxApp : public Application {
public:
    SandboxApp() : Application("Sandbox Application") {
       PushLayer(std::make_unique<TestLayer>("Layer1"));
       PushOverlay(std::make_unique<ImGuiLayer>(*m_Renderer));
    }

    ~SandboxApp() override = default;
};


std::unique_ptr<Application> Application::CreateApplication() { return std::make_unique<SandboxApp>(); }