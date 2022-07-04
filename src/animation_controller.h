#ifndef _ANIMATION_CONTROLLER_H_
#define _ANIMATION_CONTROLLER_H_

#include <iostream>

class AnimationController {
public:
  AnimationController() : m_animation_ongoing(false), m_reversed(false) {}

  virtual bool animation_can_start(const std::string& animation_name) const {
    return m_animation_ongoing == false;
  }

  virtual void animation_update(float current_time) = 0;

  virtual void on_animation_start(const std::string& animation_name, float current_time) {
    m_animation_ongoing = true;
    m_reversed = false;
    m_animation_name = animation_name;
    m_animation_start_time = current_time;
  }

  virtual void on_animation_stop() {
    m_animation_ongoing = false;
    m_reversed = false;
    m_animation_name.clear();
  }

  bool animation_ongoing() const {
      return m_animation_ongoing;
  }

  const std::string& animation_name() const {
      return m_animation_name;
  }

  virtual ~AnimationController() {}

protected:
  bool m_animation_ongoing;
  bool m_reversed;
  std::string m_animation_name;
  float m_animation_start_time;
};

#endif /* _ANIMATION_CONTROLLER_H_ */
