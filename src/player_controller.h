#ifndef _PLAYER_CONTROLLER_H_
#define _PLAYER_CONTROLLER_H_

#include "animation_controller.h"
#include "collision_detector.h"
#include "input_controller.h"
#include "object_controller.h"
#include "player.h"
#include "timer.h"

class PlayerController : ObjectController, InputController {
public:
  PlayerController(Player &player, const CollistionDetector &collision_detector,
                   GLFWwindow *window);

  // object controller methods
  void update(float current_time) override;

  // input controller methods
  void process_inputs(float delta_time) override;
  void process_inputs_keyboard(float delta_time) override;
  void process_inputs_mouse(float delta_time) override;

  void reset();

  bool is_shoot_started() const;

private:
  void process_keyboard_for_move(float delta_time) const;
  void process_keyboard_for_animation();

  void process_mouse_for_rotation(float delta_time) const;

  void animation_update(float delta_time);

  Player &m_player;

  std::unordered_map<Player::Action, AnimationController> m_action_to_animation;
  bool m_shoot_started;

  Timer m_timer;

  bool m_mouse_pressed;
};

#endif /* _PLAYER_CONTROLLER_H_ */
