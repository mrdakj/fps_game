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
      std::vector<const CollisionObject<BoundingBox> *> static_objects)
      : m_dynamic_objects(std::move(dynamic_objects)),
        m_static_objects(std::move(static_objects)) {}

  void add_dynamic_object(const CollisionObject<BoundingBox> *dynamic_object);
  void add_static_object(const CollisionObject<BoundingBox> *static_object);

  std::optional<glm::vec3>
  collision_vector(const CollisionObject<BoundingBox> &object) const;
  bool collision_exists(const CollisionObject<BoundingBox> &object) const;

private:
  std::vector<const CollisionObject<BoundingBox> *> m_dynamic_objects;
  std::vector<const CollisionObject<BoundingBox> *> m_static_objects;
};

#endif /* _COLLISION_DETECTOR_H_ */
