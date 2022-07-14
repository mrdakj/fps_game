#ifndef _COLLISION_DETECTOR_H_
#define _COLLISION_DETECTOR_H_

#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "player.h"
#include "room.h"
#include <optional>

class CollistionDetector {
public:
  CollistionDetector() = default;

  CollistionDetector(
      std::vector<const CollisionObject<BoundingBox> *> dynamic_objects,
      const CollisionObject<BoundingBox> *static_object)
      : m_dynamic_objects(std::move(dynamic_objects)),
        m_static_object(std::move(static_object)) {}

  std::optional<glm::vec3>
  collision_vector(const CollisionObject<BoundingBox> &object) const;

private:
  std::vector<const CollisionObject<BoundingBox> *> m_dynamic_objects;
  const CollisionObject<BoundingBox> *m_static_object;
};

#endif /* _COLLISION_DETECTOR_H_ */
