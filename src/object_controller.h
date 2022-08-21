#ifndef _OBJECT_CONTROLLER_H_
#define _OBJECT_CONTROLLER_H_

#include "collision_detector.h"
#include <type_traits>

class ObjectController {
public:
  ObjectController(const CollisionObject<BoundingBox> &object,
                   const CollistionDetector &collision_detector)
      : m_object(object), m_collision_detector(collision_detector) {}

  virtual void update(float current_time) = 0;

  virtual ~ObjectController() {}

  std::optional<glm::vec3> get_collision_vector() const {
    return m_collision_detector.collision_vector(&m_object);
  }

private:
  const CollisionObject<BoundingBox> &m_object;
  const CollistionDetector &m_collision_detector;
};

#endif /* _OBJECT_CONTROLLER_H_ */
