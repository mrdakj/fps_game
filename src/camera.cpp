#include "camera.h"

#include <GLFW/glfw3.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>

Camera::Camera(int width, int height, glm::vec3 position)
    : m_width(width), m_height(height), m_position(std::move(position)) {}

void Camera::update_matrix(float FOV_deg, float near_plane, float far_plane) {
  // view matrix
  glm::mat4 view = glm::mat4(1.0f);
  // projection matrix
  glm::mat4 proj = glm::mat4(1.0f);

  view = glm::lookAt(m_position, m_position + m_orientation, m_up);
  proj = glm::perspective(glm::radians(FOV_deg),
                          (float)((float)m_width / m_height), near_plane,
                          far_plane);

  m_camera_matrix = proj * view;
}

