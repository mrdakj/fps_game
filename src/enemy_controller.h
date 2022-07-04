#ifndef _ENEMY_CONTROLLER_H_
#define _ENEMY_CONTROLLER_H_

#include "animation_controller.h"
#include "collision_detector.h"
#include "enemy.h"
#include "object_controller.h"

class EnemyController : ObjectController<Enemy>, AnimationController {
public:
  EnemyController(Enemy &enemy, const CollistionDetector &collision_detector)
      : ObjectController<Enemy>(enemy, collision_detector),
        AnimationController() {}

  // object controller methods
  void update(float current_time) override;

  // animation controller methods
  void animation_update(float current_time) override;
  void on_animation_stop() override;

private:
  bool m_last_animation_triggered = false;
};

#endif /* _ENEMY_CONTROLLER_H_ */
