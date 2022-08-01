#ifndef _ENEMY_H_
#define _ENEMY_H_

#include "animated_mesh.h"
#include "animation_controller.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "light.h"
#include "player.h"
#include "shader.h"
#include "skinned_mesh.h"
#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <unordered_map>

class Enemy : public AnimatedMesh {
public:
  enum class Action { FallDead, RotateLeft, RotateRight, GunUp, GunDown, None };
  enum class ActionStatus { Running, Success };
  // TODO: fix these separate states and make some hierarchy
  // Alive -> Attacking (gun up), Patrolling (gun down)
  enum class EnemyState { Alive, Dead };
  enum class GunState { Up, Down };

  Enemy();

  void set_idle();

  void set_shot();
  bool is_shot() const;

  void update(float current_time);

  std::pair<glm::vec3, glm::vec3> get_gun_direction() const;
  std::pair<glm::vec3, glm::vec3> get_eye_direction() const;

  void render(Shader &shader, Shader &bounding_box_shader, const Camera &camera,
              const Light &light) const override;

  bool can_rotate_spine(bool left) const;

  std::optional<ActionStatus>
  get_action_status_and_remove_successful(Action action);
  void register_new_todo_action(Action action);
  void remove_todo_action(Action action);
  void clear_todo_actions();

  bool is_rotatng_spine_left = false;
  bool is_rotatng_spine_right = false;

  GunState gun_state() const { return m_gun_state; }
  EnemyState enemy_state() const { return m_enemy_state; }

private:
  // TODO: try not to keep track of time to rotate spine
  bool rotate(bool left, float delta);
  float m_last_update_time = -1;

  std::unordered_map<Action, AnimationController> m_action_to_animation;
  std::unordered_map<Action, ActionStatus> m_todo_actions;

  GunState m_gun_state = GunState::Down;
  EnemyState m_enemy_state = EnemyState::Alive;

  bool m_is_shot = false;
  float m_spine_angle = 0;
};

#endif /* _ENEMY_H_ */
