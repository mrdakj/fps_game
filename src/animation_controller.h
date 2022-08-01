#ifndef _ANIMATION_CONTROLLER_H_
#define _ANIMATION_CONTROLLER_H_

#include "animated_mesh.h"
#include "utility.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct AnimationController {
  AnimationController() = default;
  AnimationController(std::string name, bool reversed = false,
                      float speed_factor = 1.0f, bool update_global = false);
  bool update(AnimatedMesh &mesh, float current_time);

private:
  void reset_animation();
  void update_animation_time(float current_time);
  void on_animation_stop(AnimatedMesh &mesh);

private:
  // animation name
  std::string m_name;
  // is animation reversed
  bool m_reversed;
  // default animation speed factor is 1
  // animation duration is divided by its speed factor
  float m_speed_factor;
  // does animation update global position of the object
  bool m_update_global;

  // for non reversed animations start time is 0 and increases by (positive)
  // delta time for reversed animations start time is -1 and decreases by
  // (positive) delta time
  float m_current_animation_time;
  // when was the last update called
  float m_last_update_time;
  // if first tick is true, update should perform some additional tasks because
  // this is the first update call
  // it gets reseted when animation is finished
  bool m_first_tick = true;
};

#endif /* _ANIMATION_CONTROLLER_H_ */
