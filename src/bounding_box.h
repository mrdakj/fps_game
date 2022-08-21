#ifndef _BOUNDING_BOX_H_
#define _BOUNDING_BOX_H_

#include "aabb.h"
#include "shader.h"
#include <GL/glew.h>
#include <array>
#include <glm/ext/vector_float3.hpp>

class Camera;

class BoundingBox {
public:
  BoundingBox() = default;

  BoundingBox(const AABB &aabb);
  BoundingBox(std::array<glm::vec3, 3> axes, glm::vec3 origin);

  static BoundingBox bounding_aabb(const std::vector<BoundingBox> &boxes);

  // return local axes
  std::array<glm::vec3, 3> get_axes() const;

  // project all positions to normalized vector v and return min and max
  // projected value
  std::pair<float, float> project(const glm::vec3 &v) const;

  void render(Shader &shader, const Camera &camera,
              const glm::vec3 &color) const;
  BoundingBox transform(const glm::mat4 &transformation) const;

  bool is_aabb() const;

  glm::vec3 intersects(const BoundingBox &b) const;
  // intersects a segment AB
  bool intersects(const glm::vec3 &A, const glm::vec3 &B) const;

  AABB aabb() const;

public:
  std::array<glm::vec3, 3> m_axes;
  glm::vec3 m_origin;
};

#endif /* _BOUNDING_BOX_H_ */
