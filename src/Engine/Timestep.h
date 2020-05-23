#ifndef GAME_ENGINE_TIMESTEP_H
#define GAME_ENGINE_TIMESTEP_H

#include <chrono>

class Timestep {
private:
    std::chrono::duration<float> m_Time{};

public:
    explicit Timestep(std::chrono::duration<float> step) : m_Time(step) {}

    auto GetSeconds() const -> float { return m_Time.count(); }

    auto GetMiliseconds() const -> float { return m_Time.count() * 1000.0f; }

    operator float() const { return m_Time.count(); }
};

#endif //GAME_ENGINE_TIMESTEP_H
