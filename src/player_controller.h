#ifndef _PLAYER_CONTROLLER_H_
#define _PLAYER_CONTROLLER_H_

#include "animation_controller.h"
#include "collision_detector.h"
#include "input_controller.h"
#include "object_controller.h"
#include "player.h"

class PlayerController : ObjectController,
                         AnimationController,
                         InputController {
public:
  PlayerController(Player &player, const CollistionDetector &collision_detector,
                   GLFWwindow *window)
      : m_player(player), ObjectController(player, collision_detector),
        AnimationController(player), InputController(window) {}

  // object controller methods
  void update(float current_time) override;

  // input controller methods
  void process_inputs(float current_time) override;
  void process_inputs_keyboard(float current_time) override;
  void process_inputs_mouse(float current_time) override;

private:
  void process_keyboard_for_move() const;
  void process_keyboard_for_animation(float current_time);

  void process_mouse_for_rotation() const;

  Player &m_player;
};

#endif /* _PLAYER_CONTROLLER_H_ */
