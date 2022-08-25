#ifndef _ENEMY_STATE_MACHINE_H_
#define _ENEMY_STATE_MACHINE_H_

#include "animation_controller.h"
#include "nav_mesh.h"
#include <cassert>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

class Enemy;
class EnemyState;
class Alive;
class Dead;
class Attacking;
class Chasing;
class Patrolling;

class StateMachine {
  friend class Enemy;
  friend class EnemyState;
  friend class Alive;
  friend class Dead;
  friend class Attacking;
  friend class Chasing;
  friend class Patrolling;

public:
  StateMachine(Enemy &owner);

  // Alive state - Attacking, Chasing, Patrolling
  // Dead state
  enum class StateName { Attacking, Chasing, Patrolling, Dead };

  enum class Action {
    FallDead,
    RotateLeft,
    RotateRight,
    Walk,
    Shoot,
    Transition
  };

  enum class ActionStatus { Created, Preparing, Running, Finishing, Success };

  enum class Position { Standing, Attacking };

  static std::string state_name_string(StateName state_name) {
    switch (state_name) {
    case StateName::Attacking:
      return "Attacking";
    case StateName::Chasing:
      return "Chasing";
    case StateName::Patrolling:
      return "Patrolling";
    case StateName::Dead:
      return "Dead";
    default:
      assert(false && "unknown state");
    }
  }

  static std::string action_string(Action action) {
    switch (action) {
    case Action::FallDead:
      return "falldead";
    case Action::RotateLeft:
      return "RotateLeft";
    case Action::RotateRight:
      return "RotateRight";
    case Action::Walk:
      return "Walk";
    case Action::Shoot:
      return "Shoot";
    case Action::Transition:
      return "Transition";
    }

    return "unknown";
  }

  static std::string action_status_string(ActionStatus status) {
    switch (status) {
    case ActionStatus::Running:
      return "Running";
    case ActionStatus::Success:
      return "Success";
    case ActionStatus::Preparing:
      return "Preparing";
    case ActionStatus::Finishing:
      return "Finishing";
    case ActionStatus::Created:
      return "Created";
    }

    return "unknown";
  }

  // update current state or transition to some other state
  void update(float delta_time);

  // return true if current state is the same as given state,
  // otherwise transition to the given state and return false
  bool change_state(StateName state_name);

  // return action status if action is in todo action, otherwise return null
  std::optional<ActionStatus> get_action_status(Action action) const;
  void register_todo_action(Action action);
  void remove_todo_action(Action action);

  // set path (only if enemy is in Patrolling state)
  void set_path(Path path);

  static const std::string &position_name(Position position);

private:
  AnimationController &get_animation(Action action);
  const AnimationController &get_animation(Action action) const;

  // create "transition" animation that transforms enemy from its current
  // position to the given position (position = animation with only one
  // keyframe)
  void create_transition_to_position_animation(Position position,
                                               float animatin_duration);
  // create "transition" animation that transforms enemy from its current
  // position to the first frame of the action's animation
  void create_transition_to_animation(Action action, float animatin_duration);
  void reset_transition_animation();

  // return true if enemy is in the given state
  bool in_state(StateName state_name) const;
  bool transitioning_to_state(StateName state_name) const;

private:
  static std::unordered_map<Position, std::string> s_positions;

  std::unordered_map<Action, AnimationController> m_action_to_animation;

  std::unordered_map<StateName, std::unique_ptr<EnemyState>> m_states;

  EnemyState *m_current_state;
  EnemyState *m_transitioning_state;

  // ------------- enemy state ------------
  bool m_is_shot = false;
  bool m_is_shooting = false;
  std::time_t m_player_seen_time = 0;
  glm::vec3 m_player_seen_position;
  // --------------------------------------

  Enemy &m_owner;
};

// ------------------------- States -----------------------------------------
class EnemyState {
public:
  EnemyState(StateMachine &owner, StateMachine::StateName state_name,
             StateMachine::Action entering_action);

  virtual ~EnemyState() = default;

  // when entering a state - return true when the state is entered
  virtual bool enter(float delta_time) = 0;
  // update state every (other) frame
  virtual void execute(float delta_time) = 0;
  // when exiting this state
  virtual void exit();

  virtual void register_todo_action(StateMachine::Action action);

  // if action exists in todo actions return "success" if action is done and
  // "running" otherwise, if action doesn't exists return null
  std::optional<StateMachine::ActionStatus>
  get_action_status(StateMachine::Action action);
  void remove_todo_action(StateMachine::Action action);

  StateMachine::StateName name() const;

protected:
  StateMachine::StateName m_state_name;

  // actions to be done
  std::unordered_map<StateMachine::Action, StateMachine::ActionStatus>
      m_todo_actions;

  // action to perform when entering the state - enemy is in transition
  // when action is done, state is entered - enemy is not in transition
  std::pair<StateMachine::Action, StateMachine::ActionStatus> m_entering_action;

  StateMachine &m_owner;
};

class Alive : public EnemyState {
public:
  Alive(StateMachine &owner, StateMachine::StateName name,
        StateMachine::Position start_position);

  StateMachine::Position get_start_position() const;

protected:
  void do_rotate_action(std::pair<const StateMachine::Action,
                                  StateMachine::ActionStatus> &action_status,
                        float delta_time);

  StateMachine::Position m_start_position;
};

class Dead : public EnemyState {
public:
  Dead(StateMachine &owner);

  bool enter(float delta_time) override;
  void execute(float delta_time) override;
  void exit() override;

  void register_todo_action(StateMachine::Action action) override;
};

class Attacking : public Alive {
public:
  Attacking(StateMachine &owner);

  bool enter(float delta_time) override;
  void execute(float delta_time) override;

  void register_todo_action(StateMachine::Action action) override;
};

class Chasing : public Alive {
public:
  Chasing(StateMachine &owner);

  bool enter(float delta_time) override;
  void execute(float delta_time) override;

  void register_todo_action(StateMachine::Action action) override;
};

class Patrolling : public Alive {
public:
  Patrolling(StateMachine &owner);

  void set_path(Path path);

  bool enter(float delta_time) override;
  void execute(float delta_time) override;
  void exit() override;

  void register_todo_action(StateMachine::Action action) override;

private:
  float get_angle_to_rotate_user_transformation();
  float get_orientation_angle_for_user_transformation(
      const glm::vec3 &target_orientation) const;

  void rotate_user_transformation(float delta_time);

  float
  get_delta_distance_and_update_origin(const glm::mat4 &global_transformation);

  Path m_path;
  glm::vec3 m_local_origin = glm::vec3(0, 0, 0);
  float m_angle_to_rotate;
};

#endif /* _ENEMY_STATE_MACHINE_H_ */
