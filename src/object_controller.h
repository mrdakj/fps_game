#ifndef _OBJECT_CONTROLLER_H_
#define _OBJECT_CONTROLLER_H_

#include "collision_detector.h"
#include <type_traits>

template <typename T> class ObjectController {
public:
  ObjectController(T &&object, const CollistionDetector &collision_detector)
      : m_object(std::forward<T>(object)),
        m_collision_detector(collision_detector) {}

  ObjectController(T &object, const CollistionDetector &collision_detector)
      : m_object(object),
        m_collision_detector(collision_detector) {}

  virtual void update(float current_time) = 0;

  virtual ~ObjectController() {}

protected:
  std::decay_t<T> &m_object;
  const CollistionDetector &m_collision_detector;
};

#endif /* _OBJECT_CONTROLLER_H_ */
