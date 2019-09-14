#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <Engine/Include/Engine.h>


class TestLayer : public Layer {
public:
    TestLayer(const char* name) : Layer(name) {}

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
};


class TestLayer2 : public TestLayer {
public:
    TestLayer2() : TestLayer("Layer2") {}

    bool OnKeyPress(KeyPressEvent& e) override {
       std::cout << m_DebugName << "::" << e << std::endl;
       return true;
    }

    bool OnKeyRelease(KeyReleaseEvent& e) override {
       std::cout << m_DebugName << "::" << e << std::endl;
       e.Handled = true;
       return true;
    }
};


class SandboxApp : public Application {
public:
    SandboxApp() {
       PushLayer(std::make_unique<TestLayer>("Layer1"));
       PushLayer(std::make_unique<TestLayer2>());
    }

    ~SandboxApp() override = default;
};


std::unique_ptr<Application> CreateApplication() { return std::make_unique<SandboxApp>(); }