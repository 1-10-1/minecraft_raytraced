#pragma once

#include <cmath>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

class Camera
{
public:
    Camera();

    Camera(Camera const&)                    = delete;
    Camera(Camera&&)                         = delete;
    auto operator=(Camera const&) -> Camera& = delete;
    auto operator=(Camera&&) -> Camera&      = delete;

    ~Camera() = default;

    void setPosition(glm::vec3 pos)
    {
        m_position  = pos;
        m_viewDirty = true;
    }

    [[nodiscard]] auto getPosition() const -> glm::vec3 { return m_position; }

    [[nodiscard]] auto getRight() const -> glm::vec3 { return m_right; }

    [[nodiscard]] auto getUp() const -> glm::vec3 { return m_up; }

    [[nodiscard]] auto getLook() const -> glm::vec3 { return m_look; }

    [[nodiscard]] auto getView() const -> glm::mat4 { return m_view; }

    [[nodiscard]] auto getProj() const -> glm::mat4 { return m_projection; }

    [[nodiscard]] auto getNearZ() const -> float { return m_near; }

    [[nodiscard]] auto getFarZ() const -> float { return m_far; }

    [[nodiscard]] auto getAspect() const -> float { return m_aspectRatio; }

    [[nodiscard]] auto getVerticalFov() const -> float { return m_verticalFov; }

    [[nodiscard]] auto getHorizontalFov() const -> double
    {
        return 2.0 * std::atan((0.5f * getNearWindowWidth()) / m_near);
    }

    [[nodiscard]] auto getNearWindowWidth() const -> float
    {
        return m_aspectRatio * m_nearPlaneHeight;
    }

    [[nodiscard]] auto getNearWindowHeight() const -> float { return m_nearPlaneHeight; }

    [[nodiscard]] auto getFarWindowWidth() const -> float
    {
        return m_aspectRatio * m_farPlaneHeight;
    }

    [[nodiscard]] auto getFarWindowHeight() const -> float { return m_farPlaneHeight; }

    [[nodiscard]] auto getPitch() const -> float { return m_pitch; }

    [[nodiscard]] auto getYaw() const -> float { return m_yaw; }

    void setLens(float verticalFov, float width, float height, float near_z, float far_z);

    void setLens(float verticalFov, glm::uvec2 dimensions, float near_z, float far_z)
    {
        setLens(verticalFov,
                static_cast<float>(dimensions.x),
                static_cast<float>(dimensions.y),
                near_z,
                far_z);
    }

    void lookAt(glm::vec3 const& position, glm::vec3 const& target, glm::vec3 const& world_up);

    void moveX(float distance);
    void moveZ(float distance);
    void moveY(float distance);

    void pitch(float angle);
    void yaw(float angle);

    void update();

private:
    glm::vec3 m_position { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_right { 1.0f, 0.0f, 0.0f };
    glm::vec3 m_up { 0.0f, 1.0f, 0.0f };
    glm::vec3 m_look { 0.0f, 0.0f, 1.0f };

    float m_near { 0.0f };
    float m_far { 0.0f };
    float m_aspectRatio { 0.0f };
    float m_verticalFov { 0.0f };
    float m_nearPlaneHeight { 0.0f };
    float m_farPlaneHeight { 0.0f };

    float m_pitch { 0.0f };
    float m_yaw { 0.0f };

    bool m_viewDirty { true };

    glm::mat4 m_view { 1.0f };
    glm::mat4 m_projection { 1.0f };
};
