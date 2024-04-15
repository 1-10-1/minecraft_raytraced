#include <mc/camera.hpp>
#include <mc/logger.hpp>

#include <algorithm>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera() : m_position { 0.f, 0.f, 10.f } {}

void Camera::setLens(float verticalFov, float width, float height, float near_z, float far_z)
{
    m_verticalFov     = verticalFov;
    m_aspectRatio     = height / width;
    m_near            = near_z;
    m_far             = far_z;
    m_nearPlaneHeight = 2.0f * m_near * tanf(0.5f * m_verticalFov);
    m_farPlaneHeight  = 2.0f * m_far * tanf(0.5f * m_verticalFov);

    m_projection = glm::perspectiveFovRH(m_verticalFov, width, height, m_near, m_far);

    m_projection[1][1] *= -1.0f;
}

void Camera::lookAt(glm::vec3 const& position, glm::vec3 const& target, glm::vec3 const& up)
{
    m_position = position;
    m_look     = glm::normalize(target - position);
    m_right    = glm::normalize(glm::cross(up, m_look));
    m_up       = glm::cross(m_look, m_right);

    m_viewDirty = true;
}

void Camera::moveX(float distance)
{
    m_position = glm::vec3(distance) * m_right + m_position;

    m_viewDirty = true;
}

void Camera::moveY(float distance)
{
    m_position = glm::vec3(distance) * glm::vec3 { 0.f, 1.f, 0.f } + m_position;

    m_viewDirty = true;
}

void Camera::moveZ(float distance)
{
    m_position = glm::vec3(distance) * m_look + m_position;

    m_viewDirty = true;
}

void Camera::yaw(float angle)
{
    m_yaw += angle;

    m_viewDirty = true;
}

void Camera::pitch(float angle)
{
    m_pitch = std::clamp(m_pitch + angle, -89.f, 89.f);

    m_viewDirty = true;
}

void Camera::onUpdate(AppUpdateEvent const& event)
{
    if (!m_viewDirty)
    {
        return;
    }

    m_look = glm::normalize(glm::vec3 { cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)),
                                        sin(glm::radians(m_pitch)),
                                        sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)) });

    m_right = glm::normalize(glm::cross(m_look, glm::vec3 { 0.f, 1.f, 0.f }));

    m_up = glm::normalize(glm::cross(m_right, m_look));

    m_view = glm::lookAtRH(m_position, m_position + m_look, m_up);

    m_viewDirty = false;
}

void Camera::onFramebufferResize(WindowFramebufferResizeEvent const& event)
{
    setLens(
        m_verticalFov, static_cast<float>(event.dimensions.x), static_cast<float>(event.dimensions.y), m_near, m_far);
};
