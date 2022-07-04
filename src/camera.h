#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "shader.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <string>

class Camera {
public:
  Camera(int width, int height, glm::vec3 position);

  void update_matrix(float fov_deg, float near_plane, float far_plane);

  const glm::mat4 &matrix() const { return m_camera_matrix; }
  const glm::vec3 &position() const { return m_position; }
  glm::vec3 &position() { return m_position; }
  const glm::vec3 &orientation() const { return m_orientation; }
  glm::vec3 &orientation() { return m_orientation; }
  const glm::vec3 &up() const { return m_up; }
  glm::vec3 &up() { return m_up; }

  float speed() const { return m_speed; }
  float sensitivity() const { return m_sensitivity; }

  int width() const { return m_width; }
  int height() const { return m_height; }

private:
  glm::vec3 m_position;
  glm::vec3 m_orientation = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::mat4 m_camera_matrix = glm::mat4(1.0f);

  int m_width;
  int m_height;

  float m_speed = 0.1f;
  float m_sensitivity = 100.0f;
};

#endif /* _CAMERA_H_ */
