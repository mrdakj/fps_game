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
  unsigned int id;
  std::string name;
  float current_animation_time;
  float speed_factor = 1.0f;
  bool update_global = false;
};

class AnimationController {
public:
  AnimationController(AnimatedMesh &object,
                      std::vector<AnimationData> animations,
                      const std::vector<std::pair<unsigned int, unsigned int>>
                          &independent_animations = {})
      : m_object(object), m_animations(std::move(animations)),
        m_dependency_matrix(m_animations.size(),
                            std::vector<bool>(m_animations.size(), true)) {
    for (const auto &independent_pair : independent_animations) {
      assert((!m_animations[independent_pair.first].update_global ||
              !m_animations[independent_pair.second].update_global) &&
             "independent animations cannot both change global position");
      m_dependency_matrix[independent_pair.first][independent_pair.second] =
          false;
      m_dependency_matrix[independent_pair.second][independent_pair.first] =
          false;
    }
  }

  void animation_update(float current_time) {
    if (m_last_update_time == -1) {
      // set last update time for the first time
      m_last_update_time = current_time;
    }

    float delta_time = current_time - m_last_update_time;

    std::vector<unsigned int> animations_to_stop;
    for (unsigned int animation_id : m_active_animations) {
      auto &animation = m_animations[animation_id];
      if (animation.current_animation_time >= 0) {
        animation.current_animation_time += delta_time;
      } else {
        animation.current_animation_time -= delta_time;
      }

      auto [animation_finished, global_transformation] =
          m_object.update(animation.name, animation.current_animation_time,
                          animation.speed_factor);

      if (animation.update_global) {
        m_object.set_global_transformation(std::move(global_transformation));
      }

      if (animation_finished) {
        animations_to_stop.push_back(animation_id);
      }
    }

    for (unsigned int animation_id : animations_to_stop) {
      on_animation_stop(animation_id);
    }

    m_last_update_time = current_time;
  }

  bool on_animation_start(unsigned int animation_id,
                          bool stop_active_animations = false) {
    bool added = false;
    if (m_animation_start_enabled) {
      if (stop_active_animations) {
        // stop all active animations
        m_active_animations.clear();
      }

      // add animation if not active and if independent with currently active
      // animations
      if (!is_active(animation_id) &&
          independent_with_active_animations(animation_id)) {
        m_active_animations.insert(animation_id);
        added = true;
      }
    }

    // cannot have more than 1 active animations that change the global position
    // of object
    assert(global_active_count() <= 1 &&
           "number of active animations that change global position is valid");

    return added;
  }

  void disable_animation_start() { m_animation_start_enabled = false; }
  void enable_animation_start() { m_animation_start_enabled = true; }

  const std::string &name(unsigned int animation_id) const {
    return m_animations[animation_id].name;
  }

private:
  unsigned int global_active_count() const {
    return std::count_if(
        m_active_animations.cbegin(), m_active_animations.cend(),
        [&](unsigned int id) { return m_animations[id].update_global; });
  }

  bool independent_with_active_animations(unsigned int animation_id) const {
    return std::all_of(m_active_animations.cbegin(), m_active_animations.cend(),
                       [&](unsigned int active_animation_id) {
                         return independent(active_animation_id, animation_id);
                       });
  }

  bool independent(unsigned int first_animation_id,
                   unsigned int second_animation_id) const {
    return !m_dependency_matrix[first_animation_id][second_animation_id];
  }

  void on_animation_stop(unsigned int animation_id) {
    assert(is_active(animation_id) && "stopping active animation");
    auto &animation = m_animations[animation_id];
    if (animation.update_global) {
      m_object.merge_user_and_global_transformations();
    }

    animation.current_animation_time =
        animation.current_animation_time < 0 ? -1 : 0;

    m_active_animations.erase(animation_id);
  }

  bool is_active(unsigned int animation_id) const {
    return m_active_animations.find(animation_id) != m_active_animations.end();
  }

  // object to animate
  AnimatedMesh &m_object;

  // all animations
  std::vector<AnimationData> m_animations;
  // currently active animations, store animation id
  std::unordered_set<unsigned int> m_active_animations;

  // if m_dependency_matrix[id1][id2] is true then animations with id1 and id2
  // are dependent and cannot be active at the same time
  std::vector<std::vector<bool>> m_dependency_matrix;

  // last time the update was called
  float m_last_update_time = -1;
  // can animations start
  bool m_animation_start_enabled = true;
};

#endif /* _ANIMATION_CONTROLLER_H_ */
