#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "bounding_box.h"
#include "shader.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <string>

class Camera {
public:
  Camera(int width, int height, glm::vec3 position);

  void reset(glm::vec3 position);

  void update_matrix();

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

  BoundingBox get_bounding_box() const;

private:
  const float m_FOV_deg = 45;
  const float m_near_plane = 0.1;
  const float m_far_plane = 40;

  const int m_width;
  const int m_height;

  const float m_speed = 3.5f;
  const float m_sensitivity = 60.0f;

  glm::vec3 m_position;
  glm::vec3 m_orientation;
  glm::vec3 m_up;
  glm::mat4 m_camera_matrix;
};

#endif /* _CAMERA_H_ */
