#ifndef GAME_ENGINE_CAMERA_H
#define GAME_ENGINE_CAMERA_H

#include <mathlib.h>
#include <iostream>
#include <glm/matrix.hpp>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>


// TODO: create more sensible camera hierarchy with different camera types such as free/lookAt, etc.
class Camera {
protected:
    glm::mat4 m_Projection;
    glm::mat4 m_View;
    glm::vec3 m_Position;
    glm::vec3 m_Direction;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    float m_Yaw = -HALF_PI_F;
    float m_Pitch = 0.0f;

    Camera() = default;

    Camera(const glm::mat4 &projection,
           const glm::mat4 &view,
           const glm::vec3 &position,
           const glm::vec3 &direction) :
            m_Projection(projection),
            m_View(view),
            m_Position(position),
            m_Direction(glm::normalize(direction)),
            m_Up(0.0f, 0.0f, 1.0f),
            m_Right(glm::normalize(glm::cross(m_Direction, m_Up))),
            m_Yaw(std::atan2(m_Direction.y, m_Direction.x)) {
        
//        m_Projection[1][1] *= -1;
    };

    void CalculateDirection() {
        SetDirection(glm::vec3(
                std::cos(m_Yaw) * std::cos(m_Pitch),
                std::sin(m_Yaw) * std::cos(m_Pitch),
                std::sin(m_Pitch)
        ));
    }

public:
    auto GetProjection() const -> const glm::mat4 & { return m_Projection; }

    auto GetView() const -> const glm::mat4 & { return m_View; }

    auto GetProjectionView() const -> glm::mat4 { return m_Projection * m_View; }

    void SetPosition(const glm::vec3 &position) {
        m_Position = position;
        m_View = glm::lookAt(m_Position, m_Position + m_Direction, m_Up);
    }

    void Move(const glm::vec3 &delta) { SetPosition(m_Position + delta); }

    void MoveX(float delta) { SetPosition(m_Position + glm::vec3(delta, 0.0f, 0.0f)); }

    void MoveY(float delta) { SetPosition(m_Position + glm::vec3(0.0f, delta, 0.0f)); }

    void MoveZ(float delta) { SetPosition(m_Position + glm::vec3(0.0f, 0.0f, delta)); }

    void MoveInDirection(float delta) { SetPosition(m_Position + (m_Direction * delta)); }

    void MoveSideways(float delta) { SetPosition(m_Position + (m_Right * delta)); }

    void SetDirection(const glm::vec3 &direction) {
        m_Direction = glm::normalize(direction);
        m_Right = glm::normalize(glm::cross(m_Direction, m_Up));
        m_View = glm::lookAt(m_Position, m_Position + m_Direction, m_Up);
    }

    auto GetDirection() -> const glm::vec3& { return m_Direction; }

    void ChangeYawPitch(float yawDelta, float pitchDelta) {
        m_Yaw += yawDelta;
        m_Pitch = CLAMP(-HALF_PI_F + 0.01f, m_Pitch + pitchDelta, HALF_PI_F - 0.01f);
        CalculateDirection();
    }

    void SetYawPitch(float yaw, float pitch) {
        m_Yaw = yaw;
        m_Pitch = CLAMP(-HALF_PI_F + 0.01f, pitch, HALF_PI_F - 0.01f);
        CalculateDirection();
    }

    void SetPitch(float pitch) { SetYawPitch(m_Yaw, pitch); }

    void SetYaw(float yaw) { SetYawPitch(yaw, m_Pitch); }

    void ChangePitch(float delta) { ChangeYawPitch(0.0f, delta); }

    void ChangeYaw(float delta) { ChangeYawPitch(delta, 0.0f); }

    auto GetPosition() const -> const glm::vec3 & { return m_Position; }
};


class PerspectiveCamera : public Camera {
    float m_AspectRatio = 0;
    float m_FovY = 0;
    float m_Near = 0;
    float m_Far = 0;

public:
    PerspectiveCamera() = default;

    PerspectiveCamera(const glm::vec3 &position,
                      const glm::vec3 &direction,
                      float aspectRatio,
                      float near,
                      float far,
                      float fov = math::radians(45.0f)) :
            Camera(
                    glm::perspective(fov, aspectRatio, near, far),
                    glm::lookAt(position,
                                 direction,
                                glm::vec3(0.0f, 0.0f, 1.0f)),
                    position,
                    direction
            ),
            m_AspectRatio(aspectRatio),
            m_FovY(fov),
            m_Near(near),
            m_Far(far) {}

    void SetAspectRatio(float ratio) {
        m_AspectRatio = ratio;
        m_Projection = glm::perspective(m_FovY, m_AspectRatio, m_Near, m_Far);
//        m_Projection[1][1] *= -1;
    }

    auto GetAspectRatio() const -> float { return m_AspectRatio; }

    void SetFOV(float fov) {
        m_FovY = fov;
        m_Projection = glm::perspective(m_FovY, m_AspectRatio, m_Near, m_Far);
//        m_Projection[1][1] *= -1;
    }

    auto GetFOV() const -> float { return m_FovY; }
};


#endif //GAME_ENGINE_CAMERA_H
