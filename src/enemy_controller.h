#ifndef _ENEMY_CONTROLLER_H_
#define _ENEMY_CONTROLLER_H_

#include "animation_controller.h"
#include "collision_detector.h"
#include "enemy.h"
#include "object_controller.h"

class EnemyController : ObjectController, AnimationController {
public:
  EnemyController(Enemy &enemy, const CollistionDetector &collision_detector)
      : m_enemy(enemy), ObjectController(enemy, collision_detector),
        AnimationController(enemy) {}

  // object controller methods
  void update(float current_time) override;

private:
  bool check_if_dead_and_animate();
  void shoot();
  void rotate(float current_time);
  float get_rotation_angle(float delta_time) const;
  void start_rotate_left();
  void start_rotate_right();

  Enemy &m_enemy;
  // how much we rotated so far
  float m_rotation_angle = 0;
  // how many degrees to rotate in one second
  float m_rotation_speed = 90;
  float m_last_update_time = -1;

  bool m_is_shooting = false;
};

#endif /* _ENEMY_CONTROLLER_H_ */
