#ifndef _PLAYER_CONTROLLER_H_
#define _PLAYER_CONTROLLER_H_

#include "animation_controller.h"
#include "collision_detector.h"
#include "input_controller.h"
#include "object_controller.h"
#include "player.h"

class PlayerController : ObjectController, InputController {
public:
  PlayerController(Player &player, const CollistionDetector &collision_detector,
                   GLFWwindow *window)
      : m_player(player), InputController(window),
        ObjectController(player, collision_detector),
        m_action_to_animation({{Player::Action::Shoot, {"shoot"}},
                               {Player::Action::Recharge, {"CINEMA_4D_Main"}}}),
        m_todo_action{Player::Action::None} {}

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

  void animation_update(float current_time);

  Player &m_player;

  std::unordered_map<Player::Action, AnimationController> m_action_to_animation;
  Player::Action m_todo_action;
};

#endif /* _PLAYER_CONTROLLER_H_ */
