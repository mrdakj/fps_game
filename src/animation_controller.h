#ifndef _ANIMATION_CONTROLLER_H_
#define _ANIMATION_CONTROLLER_H_

#include "animated_mesh.h"
#include "sound.h"
#include "utility.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <future>

struct AnimationController {
  AnimationController(AnimatedMesh &mesh, std::string name,
                      bool reversed = false, float speed_factor = 1.0f,
                      bool update_global = false);

  AnimationController(AnimatedMesh &mesh, std::string name,
                      Sound::Track sound_track, bool reversed = false,
                      float speed_factor = 1.0f, bool update_global = false);
  std::pair<bool, glm::mat4> update(float delta_time);

  void on_animation_stop();

  void reset();

private:
  void reset_animation();
  void update_animation_time(float delta_time);

public:
  AnimatedMesh &m_mesh;

  // animation name
  std::string m_name;
  // is animation reversed
  bool m_reversed;
  // default animation speed factor is 1
  // animation duration is divided by its speed factor
  float m_speed_factor;
  // does animation update global position of the object
  bool m_update_global;
  // optional sound
  std::optional<Sound::Track> m_sound_track;

  // for non reversed animations start time is 0 and increases by (positive)
  // delta time for reversed animations start time is -1 and decreases by
  // (positive) delta time
  float m_current_animation_time;

  // if first tick is true, update should perform some additional tasks because
  // this is the first update call
  // it gets reseted when animation is finished
  bool m_first_tick = true;
};

#endif /* _ANIMATION_CONTROLLER_H_ */
