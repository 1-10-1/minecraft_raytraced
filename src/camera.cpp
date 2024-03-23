#include <mc/camera.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera()
{
    setLens(0.25f * glm::pi<float>(), 1.0f, 1.0f, 1.0f, 1000.0f);
}

void Camera::setLens(float verticalFov, float width, float height, float near_z, float far_z)
{
    // cache properties
    m_verticalFov     = verticalFov;
    m_aspectRatio     = height / width;
    m_near            = near_z;
    m_far             = far_z;
    m_nearPlaneHeight = 2.0f * m_near * tanf(0.5f * m_verticalFov);
    m_farPlaneHeight  = 2.0f * m_far * tanf(0.5f * m_verticalFov);

    m_projection = glm::perspectiveFovLH(m_verticalFov, width, height, m_near, m_far);
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
    m_position = glm::vec3(distance) * m_up + m_position;

    m_viewDirty = true;
}

void Camera::moveZ(float distance)
{
    m_position = glm::vec3(distance) * m_look + m_position;

    m_viewDirty = true;
}

void Camera::pitch(float angle)
{
    m_pitch += angle;

    glm::mat4 right { glm::rotate(glm::mat4 { 1.0f }, angle, glm::vec3 { 1.f, 0.f, 0.f }) };

    m_up   = glm::transpose(glm::inverse(right)) * glm::vec4(m_up, 0.0f);
    m_look = glm::transpose(glm::inverse(right)) * glm::vec4(m_look, 0.0f);

    m_viewDirty = true;
}

void Camera::yaw(float angle)
{
    m_yaw += angle;

    glm::mat4 right { glm::rotate(glm::mat4 { 1.0f }, angle, glm::vec3 { 0.f, 1.f, 0.f }) };

    m_up   = glm::transpose(glm::inverse(right)) * glm::vec4(m_up, 0.0f);
    m_look = glm::transpose(glm::inverse(right)) * glm::vec4(m_look, 0.0f);

    m_viewDirty = true;
}

void Camera::update()
{
    if (!m_viewDirty)
    {
        return;
    }

    m_look  = glm::normalize(m_look);
    m_up    = glm::normalize(glm::cross(m_look, m_right));
    m_right = glm::cross(m_up, m_look);

    float x { -glm::dot(m_position, m_right) };
    float y { -glm::dot(m_position, m_up) };
    float z { -glm::dot(m_position, m_look) };

    // clang-format off
    m_view = {
        m_right.x, m_up.x, m_look.x, 0.0f,
        m_right.y, m_up.y, m_look.y, 0.0f,
        m_right.z, m_up.z, m_look.z, 0.0f,
        x,         y,      z,        1.0f
    };
    // clang-format on

    m_viewDirty = false;
}
