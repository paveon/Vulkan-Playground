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
       if (e.RepeatCount() < 1) std::cout << m_DebugName << "::" << e << std::endl;
       return true;
    }

    bool OnKeyRelease(KeyReleaseEvent& e) override {
       std::cout << m_DebugName << "::" << e << std::endl;
       return true;
    }

    void OnUpdate() override {
    }
};


class SandboxApp : public Application {
public:
    SandboxApp() : Application("Sandbox Application") {
       PushLayer(std::make_unique<TestLayer>("Layer1"));
       ImGuiLayer* imGui = static_cast<ImGuiLayer*>(PushOverlay(std::make_unique<ImGuiLayer>(*m_Renderer)));

       imGui->SetDrawCallback([](){
           ImGui::Begin("Test window");
           static float rotation = 0.0;
           ImGui::SliderFloat("rotation", &rotation, 0.0f, 360.0f);
           static float translation[] = {0.0, 0.0};
           ImGui::SliderFloat2("position", translation, -1.0, 1.0);
           static float color[4] = { 1.0f,1.0f,1.0f,1.0f };
           ImGui::ColorEdit3("color", color);
           ImGui::End();
       });
    }

    ~SandboxApp() override = default;
};


std::unique_ptr<Application> Application::CreateApplication() { return std::make_unique<SandboxApp>(); }