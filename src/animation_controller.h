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

struct AnimationData {
  float current_animation_time;
  float speed_factor = 1.0f;
  bool update_global = false;
  std::unordered_set<std::string> independent_animations = {};
};

class AnimationController {
public:
  AnimationController(AnimatedMesh &object) : m_object(object) {}

  void animation_update(float current_time) {
    if (m_last_update_time == -1) {
      // set last update time for the first time
      m_last_update_time = current_time;
    }

    float delta_time = current_time - m_last_update_time;

    std::vector<std::string> animations_to_stop;
    for (auto &active_animation : m_active_animations) {
      if (active_animation.second.current_animation_time >= 0) {
        active_animation.second.current_animation_time += delta_time;
      } else {
        active_animation.second.current_animation_time -= delta_time;
      }

      auto [animation_finished, global_transformation] =
          m_object.update(active_animation.first,
                          active_animation.second.current_animation_time,
                          active_animation.second.speed_factor);

      if (active_animation.second.update_global) {
        m_object.set_global_transformation(std::move(global_transformation));
      }

      if (animation_finished) {
        animations_to_stop.push_back(active_animation.first);
      }
    }

    for (const auto &animation : animations_to_stop) {
      on_animation_stop(animation);
    }

    m_last_update_time = current_time;
  }

  bool on_animation_start(const std::string &animation_name,
                          const AnimationData &animation_data,
                          bool stop_active_animations = false) {
    if (!m_animation_start_enabled) {
      return false;
    }

    if (stop_active_animations) {
      // stop all active animations
      m_active_animations.clear();
    }

    // add animation if not exists
    if (m_active_animations.find(animation_name) == m_active_animations.end()) {
      if (std::all_of(m_active_animations.cbegin(), m_active_animations.cend(),
                      [&](auto const &active_animation) {
                        return animation_data.independent_animations.find(
                                   active_animation.first) !=
                               animation_data.independent_animations.end();
                      })) {
        m_active_animations.emplace(animation_name, animation_data);
        return true;
      }
    }

    return false;
  }

  void on_animation_stop(const std::string &animation_name) {
    auto animation_it = m_active_animations.find(animation_name);
    assert(animation_it != m_active_animations.end() && "animation exists");
    if (animation_it->second.update_global) {
      m_object.merge_user_and_global_transformations();
    }
    m_active_animations.erase(animation_it);
  }

  void delete_animation(const std::string &animation_name) {
    auto animation_it = m_active_animations.find(animation_name);
    assert(animation_it != m_active_animations.end() && "animation exists");
    m_active_animations.erase(animation_it);
  }

  void disable_animation_start() { m_animation_start_enabled = false; }

  void enable_animation_start() { m_animation_start_enabled = true; }

  ~AnimationController() {}

private:
  // object to animate
  AnimatedMesh &m_object;
  // currently active animations
  std::unordered_map<std::string, AnimationData> m_active_animations;
  // last time the update was called
  float m_last_update_time = -1;
  // can animations start
  bool m_animation_start_enabled = true;
};

#endif /* _ANIMATION_CONTROLLER_H_ */
