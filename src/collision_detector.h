#ifndef _COLLISION_DETECTOR_H_
#define _COLLISION_DETECTOR_H_

#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "enemy.h"
#include "map.h"
#include "player.h"
#include "skinned_mesh.h"
#include <optional>

class CollistionDetector {
public:
  // get vector in which direction and intensity to move dynamic object in order
  // to solve a collision
  std::optional<glm::vec3>
  collision_vector(const CollisionObject<BoundingBox> *dynamic_object) const;

  void set_objects(
      std::vector<const CollisionObject<BoundingBox> *> static_objects,
      std::vector<const CollisionObject<BoundingBox> *> dynamic_objects) {
    m_static_objects = std::move(static_objects);
    m_dynamic_objects = std::move(dynamic_objects);
  }

private:
  std::optional<glm::vec3>
  collision_vector(const BoundingBox &box,
                   const BVHNode<BoundingBox> &node) const;

  std::vector<const CollisionObject<BoundingBox> *> m_static_objects;
  std::vector<const CollisionObject<BoundingBox> *> m_dynamic_objects;
};

#endif /* _COLLISION_DETECTOR_H_ */
