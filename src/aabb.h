#ifndef _AABB_H_
#define _AABB_H_

#include <glm/ext/vector_float3.hpp>
#include <vector>

class AABB {
public:
  AABB();

  AABB(float min_x, float max_x, float min_y, float max_y, float min_z,
       float max_z);

  void update(const glm::vec3 &position);
  void update(const AABB &other);
  bool valid() const;

  bool intersects(const AABB &other) const;

public:
  float min_x, max_x;
  float min_y, max_y;
  float min_z, max_z;
};

#endif /* _AABB_H_ */
