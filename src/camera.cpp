#include "camera.h"
#include "aabb.h"

#include <GLFW/glfw3.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>

Camera::Camera(int width, int height, glm::vec3 position)
    : m_width(width), m_height(height), m_position(std::move(position)),
      m_orientation(glm::vec3(0.0f, 0.0f, -1.0f)),
      m_up(glm::vec3(0.0f, 1.0f, 0.0f)), m_camera_matrix(glm::mat4(1.0f)) {}

void Camera::reset(glm::vec3 position) {
  m_position = std::move(position);
  m_orientation = glm::vec3(0.0f, 0.0f, -1.0f);
  m_up = glm::vec3(0.0f, 1.0f, 0.0f);
  m_camera_matrix = glm::mat4(1.0f);
}

void Camera::update_matrix() {
  auto view = glm::lookAt(m_position, m_position + m_orientation, m_up);
  auto proj = glm::perspective(glm::radians(m_FOV_deg),
                               (float)((float)m_width / m_height), m_near_plane,
                               m_far_plane);

  m_camera_matrix = proj * view;
}

BoundingBox Camera::get_bounding_box() const {
  // camera matrix maps frusum world coordinates to unit cube such that 8 points
  // match
  glm::vec4 cube_homogenous[8]{glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
                               glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
                               glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                               glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
                               glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
                               glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
                               glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),
                               glm::vec4(1.0f, -1.0f, -1.0f, 1.0f)};

  AABB aabb;
  glm::mat4 camera_inverse = glm::inverse(m_camera_matrix);

  for (int i = 0; i < 8; ++i) {
    cube_homogenous[i] = camera_inverse * cube_homogenous[i];
    assert(cube_homogenous[i][3] != 0 && "cube coordinate valid");
    cube_homogenous[i] /= cube_homogenous[i][3];
    aabb.update(cube_homogenous[i]);
  }

  return aabb;
}
