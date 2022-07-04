#include "aabb.h"
#include <iostream>

#define FLOAT_MIN (-std::numeric_limits<float>::infinity())
#define FLOAT_MAX (std::numeric_limits<float>::infinity())

AABB::AABB()
    : min_x(FLOAT_MAX), max_x(FLOAT_MIN), min_y(FLOAT_MAX), max_y(FLOAT_MIN),
      min_z(FLOAT_MAX), max_z(FLOAT_MIN)

{}

AABB::AABB(float minx, float maxx, float miny, float maxy, float minz,
           float maxz)
    : min_x(minx), max_x(maxx), min_y(miny), max_y(maxy), min_z(minz),
      max_z(maxz) {}

void AABB::update(const glm::vec3 &position) {
  min_x = std::min(min_x, position[0]);
  max_x = std::max(max_x, position[0]);

  min_y = std::min(min_y, position[1]);
  max_y = std::max(max_y, position[1]);

  min_z = std::min(min_z, position[2]);
  max_z = std::max(max_z, position[2]);
}

void AABB::update(const AABB &other) {
  // find union
  min_x = std::min(min_x, other.min_x);
  max_x = std::max(max_x, other.max_x);

  min_y = std::min(min_y, other.min_y);
  max_y = std::max(max_y, other.max_y);

  min_z = std::min(min_z, other.min_z);
  max_z = std::max(max_z, other.max_z);
}

bool AABB::valid() const {
  return min_x != FLOAT_MAX && max_x != FLOAT_MIN && min_y != FLOAT_MAX &&
         max_y != FLOAT_MIN && min_z != FLOAT_MAX && max_z != FLOAT_MIN;
}

bool AABB::intersects(const AABB &other) const {

  return (min_x <= other.max_x && max_x >= other.min_x) &&
         (min_y <= other.max_y && max_y >= other.min_y) &&
         (min_z <= other.max_z && max_z >= other.min_z);
}
