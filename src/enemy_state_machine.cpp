#include "enemy_state_machine.h"
#include "enemy.h"
#include "sound.h"
#include <glm/gtx/vector_angle.hpp>

#define EPS 0.001

#define TRANSITION_TO_FALL_DEAD_ANIMATION (0.1)
#define TRANSITION_TO_ROTATE_ANIMATION (0.1)
#define TRANSITION_TO_WALK_ANIMATION (0.3)

#define TRANSITION_TO_ATTACKING_POSITION (0.3)
#define TRANSITION_TO_STANDING_POSITION (0.3)

// ---------- StateMachine ---------------

// ----------- static --------------------
std::unordered_map<StateMachine::Position, std::string>
    StateMachine::s_positions = {{Position::Standing, "standing"},
                                 {Position::Attacking, "attacking"}};

const std::string &StateMachine::position_name(Position position) {
  return s_positions[position];
}
// ---------------------------------------

StateMachine::StateMachine(Enemy &owner)
    : m_owner(owner), m_is_shot(false), m_is_shooting(false),
      m_player_noticed_time(0),
      m_action_to_animation(
          {{Action::RotateLeft, {owner, "rotate", true, 2.5f, true}},
           {Action::RotateRight, {owner, "rotate", false, 2.5f, true}},
           {Action::FallDead,
            {owner, "fall_dead", Sound::Track::FallDown, false, 1.0f}},
           {Action::Walk, {owner, "walk", false, 0.8f, false}},
           {Action::Shoot,
            {owner, "shoot", Sound::Track::RifleShoot, false, 2.0f}},
           {Action::Transition, {owner, "transition", false}}}) {

  // create Attacking, Chasing, Patrolling and Dead states
  m_states.emplace(StateName::Attacking, std::make_unique<Attacking>(*this));
  m_states.emplace(StateName::Chasing, std::make_unique<Chasing>(*this));
  m_states.emplace(StateName::Patrolling, std::make_unique<Patrolling>(*this));
  m_states.emplace(StateName::Dead, std::make_unique<Dead>(*this));

  // set current state to Patrolling
  m_current_state = m_states[StateName::Patrolling].get();
  // set init position to Patrolling start position
  m_owner.set_bones_position(
      static_cast<Patrolling *>(m_current_state)->get_start_position());

  // there is no transitioning state at this moment
  m_transitioning_state = nullptr;
}

void StateMachine::reset() {
  // reset enemy state
  m_is_shot = false;
  m_is_shooting = false;
  m_player_noticed_time = 0;

  // reset animtions
  for (auto &action_animation : m_action_to_animation) {
    action_animation.second.reset();
  }

  // reset states
  for (auto &name_state : m_states) {
    name_state.second->exit();
  }

  // set current state to Patrolling
  m_current_state = m_states[StateName::Patrolling].get();
  // set init position to Patrolling start position
  m_owner.set_bones_position(
      static_cast<Patrolling *>(m_current_state)->get_start_position());

  // there is no transitioning state at this moment
  m_transitioning_state = nullptr;
}

AnimationController &StateMachine::get_animation(Action action) {
  // find animation controller for action
  auto animation_it = m_action_to_animation.find(action);
  assert(animation_it != m_action_to_animation.end() && "valid action");
  return animation_it->second;
}

const AnimationController &StateMachine::get_animation(Action action) const {
  // find animation controller for action
  return get_animation(action);
}

bool StateMachine::change_state(StateMachine::StateName state_name) {
  // return true if current state is the same as given state,
  // otherwise transition to the given state and return false

  // if in enemy is in state then m_current_state is not null and
  // m_transitioning_state is null, if enemy is in transition then
  // m_current_state is null and m_transitioning_state is not null
  assert((m_current_state && !m_transitioning_state) ||
         (!m_current_state && m_transitioning_state) &&
             "Enemy is either in state or in transition.");

  if (m_current_state) {
    // currently not in transition
    if (m_current_state->name() == state_name) {
      // already in this state
      return true;
    }
    // transitioning to the given state
    // exit the current state
    m_current_state->exit();
    // set transitioning to the given state
    m_current_state = nullptr;
    m_transitioning_state = m_states[state_name].get();
  } else {
    // currently in transition
    if (m_transitioning_state->name() != state_name) {
      // already transitioning but to other state - change transitioning to the
      // given state

      // exit transitioning state
      m_transitioning_state->exit();
      // set transitioning to the given state
      m_current_state = nullptr;
      m_transitioning_state = m_states[state_name].get();
    }
    // else {
    //   // already transitioning to the given state
    // }
  }

  // transition not yet done
  return false;
}

void StateMachine::update(float delta_time) {
  assert((m_current_state && !m_transitioning_state) ||
         (!m_current_state && m_transitioning_state) &&
             "Enemy is either in state or in transition.");

  if (m_current_state) {
    // not transitioning
    m_current_state->execute(delta_time);
  } else {
    // transitioning
    if (m_transitioning_state->enter(delta_time)) {
      // transitioning done - swap current (null) and transitioning state
      std::swap(m_current_state, m_transitioning_state);
    }
  }
}

void StateMachine::set_path(Path path) {
  // path can be set only in Patrolling state
  assert(in_state(StateName::Patrolling) && "set path in Patrolling state");
  static_cast<Patrolling *>(m_current_state)->set_path(std::move(path));
}

std::optional<StateMachine::ActionStatus>
StateMachine::get_action_status(Action action) const {
  return m_current_state
             ?
             // return action status from the current state if exists
             m_current_state->get_action_status(action)
             // there is no action when transitioning
             : std::nullopt;
}

void StateMachine::register_todo_action(Action action) {
  // action cannot be registered in transitioning state
  assert(m_current_state && "not in transition when registering new action");
  m_current_state->register_todo_action(action);
}

void StateMachine::remove_todo_action(Action action) {
  // make sure action exists and remove it
  assert(m_current_state && "not in transition when removing action");
  // remove action and check it existed
  m_current_state->remove_todo_action(action);
  // stop action's animation
  get_animation(action).on_animation_stop();
}

void StateMachine::create_transition_to_position_animation(
    Position position, float animatin_duration) {
  if (position == Position::Attacking) {
    // if transitioning to attacking, ignore the spine bone because it is
    // controlled manually
    m_owner.create_transition_animation(StateMachine::position_name(position),
                                        animatin_duration, Enemy::SPINE_BONE);
  } else {
    m_owner.create_transition_animation(StateMachine::position_name(position),
                                        animatin_duration);
  }
  reset_transition_animation();
}

void StateMachine::create_transition_to_animation(Action action,
                                                  float animatin_duration) {
  m_owner.create_transition_animation(get_animation(action).m_name,
                                      animatin_duration);
  reset_transition_animation();
}

void StateMachine::reset_transition_animation() {
  // reset transition animation
  get_animation(Action::Transition).on_animation_stop();
}

bool StateMachine::in_state(StateName state_name) const {
  return m_current_state && m_current_state->name() == state_name;
}

bool StateMachine::transitioning_to_state(StateName state_name) const {
  return m_transitioning_state && m_transitioning_state->name() == state_name;
}

// ------------------ EnemyState ------------------------
EnemyState::EnemyState(StateMachine &owner, StateMachine::StateName state_name,
                       StateMachine::Action entering_action)
    : m_owner(owner), m_state_name(state_name),
      m_entering_action{entering_action, StateMachine::ActionStatus::Created} {}

void EnemyState::register_todo_action(StateMachine::Action action) {
  // make sure action doesn't exist
  assert(m_todo_actions.find(action) == m_todo_actions.end() &&
         "action doesn't exist in todo actions");
  // create new todo action with state Created
  m_todo_actions.emplace(action, StateMachine::ActionStatus::Created);
}

void EnemyState::remove_todo_action(StateMachine::Action action) {
  int number_of_removed_elements = m_todo_actions.erase(action);
  assert(number_of_removed_elements == 1 && "action removed");
}

std::optional<StateMachine::ActionStatus>
EnemyState::get_action_status(StateMachine::Action action) {
  auto todo_action_it = m_todo_actions.find(action);
  if (todo_action_it != m_todo_actions.end()) {
    if (todo_action_it->second == StateMachine::ActionStatus::Success) {
      // action is done, return success
      return StateMachine::ActionStatus::Success;
    } else {
      // always return running no matter which stage action is currently in
      return StateMachine::ActionStatus::Running;
    }
  }

  // action is not in todo actions
  return std::nullopt;
}

StateMachine::StateName EnemyState::name() const { return m_state_name; }

void EnemyState::exit() {
  // stop all actions that are not done
  for (auto &action_status : m_todo_actions) {
    if (action_status.second != StateMachine::ActionStatus::Success) {
      m_owner.get_animation(action_status.first).on_animation_stop();
    }
  }

  // remove all todo actions
  m_todo_actions.clear();
  // restore status of entering action to created so it can be properly entered
  // later again
  m_entering_action.second = StateMachine::ActionStatus::Created;
}

// ------------------ Alive State ------------------------------
Alive::Alive(StateMachine &owner, StateMachine::StateName name,
             StateMachine::Position start_position)
    : EnemyState(owner, name, StateMachine::Action::Transition),
      m_start_position(start_position) {}

StateMachine::Position Alive::get_start_position() const {
  return m_start_position;
}

void Alive::do_rotate_action(
    std::pair<const StateMachine::Action, StateMachine::ActionStatus>
        &action_status,
    float delta_time) {
  assert((action_status.first == StateMachine::Action::RotateLeft ||
          action_status.first == StateMachine::Action::RotateRight) &&
         "performing rotate action");

  if (action_status.second == StateMachine::ActionStatus::Created) {
    // create "transition" animation from current position to the first
    // frame of rotate animation
    m_owner.create_transition_to_animation(action_status.first,
                                           TRANSITION_TO_ROTATE_ANIMATION);
    action_status.second = StateMachine::ActionStatus::Preparing;
  }

  if (action_status.second == StateMachine::ActionStatus::Preparing) {
    // update transitioning animation
    auto [finished, global_transformation] =
        m_owner.get_animation(StateMachine::Action::Transition)
            .update(delta_time);

    if (finished) {
      action_status.second = StateMachine::ActionStatus::Running;
    }
  } else if (action_status.second == StateMachine::ActionStatus::Running) {
    // update rotate animation
    auto [finished, global_transformation] =
        m_owner.get_animation(action_status.first).update(delta_time);

    if (finished) {
      action_status.second = StateMachine::ActionStatus::Success;
    }
  }
}

bool Alive::go_to_attacking_position(float delta_time) {
  // make sure enemy is transitioning to this state
  assert(!m_owner.m_current_state && m_owner.m_transitioning_state &&
         m_owner.m_transitioning_state->name() == m_state_name &&
         "enemy transitioning valid");

  if (m_entering_action.second == StateMachine::ActionStatus::Created) {
    // create "transition" animation from current position to the start position
    // - Attacking
    m_owner.create_transition_to_position_animation(
        m_start_position, TRANSITION_TO_ATTACKING_POSITION);
    m_entering_action.second = StateMachine::ActionStatus::Running;
  }

  if (m_entering_action.second == StateMachine::ActionStatus::Running) {
    // update transitioning animation
    auto [finished, global_transformation] =
        m_owner.get_animation(m_entering_action.first).update(delta_time);

    // rotate spine bone manually in order to follow the moving target
    m_owner.m_owner.rotate_spine(delta_time);

    if (finished) {
      // entering done
      m_entering_action.second = StateMachine::ActionStatus::Success;
    }

    return finished;
  }

  assert(false);
}

// ---------------- Dead State ----------------------------------
Dead::Dead(StateMachine &owner)
    : EnemyState(owner, StateMachine::StateName::Dead,
                 StateMachine::Action::FallDead) {}

bool Dead::enter(float delta_time) {
  if (m_owner.in_state(m_state_name)) {
    // already in this state
    return true;
  }

  // make sure enemy is transitioning to this state
  assert(!m_owner.m_current_state && m_owner.m_transitioning_state &&
         m_owner.m_transitioning_state->name() == m_state_name &&
         "enemy transitioning valid");

  if (m_entering_action.second == StateMachine::ActionStatus::Created) {
    // create "transition" animation from current position to the first frame of
    // entering action - fall dead
    m_owner.create_transition_to_animation(m_entering_action.first,
                                           TRANSITION_TO_FALL_DEAD_ANIMATION);
    m_entering_action.second = StateMachine::ActionStatus::Preparing;
  }

  if (m_entering_action.second == StateMachine::ActionStatus::Preparing) {
    // update transitioning to the first frame of the entering action
    auto [finished, global_transformation] =
        m_owner.get_animation(StateMachine::Action::Transition)
            .update(delta_time);

    if (finished) {
      m_entering_action.second = StateMachine::ActionStatus::Running;
    }

    return false;
  }

  if (m_entering_action.second == StateMachine::ActionStatus::Running) {
    // update entering action
    auto [finished, global_transformation] =
        m_owner.get_animation(m_entering_action.first).update(delta_time);

    if (finished) {
      m_entering_action.second = StateMachine::ActionStatus::Success;
    }

    return finished;
  }

  // if action status is success, enemy should be in this state
  assert(false);
}

void Dead::execute(float delta_time) {
  // nothing to do in dead state
}

void Dead::register_todo_action(StateMachine::Action action) {
  assert(false && "dead state cannot have any action");
}

// ---------------- Attacking State --------------------------
Attacking::Attacking(StateMachine &owner)
    : Alive(owner, StateMachine::StateName::Attacking,
            StateMachine::Position::Attacking) {}

bool Attacking::enter(float delta_time) {
  if (m_owner.in_state(m_state_name)) {
    // already in this state
    return true;
  }

  return Alive::go_to_attacking_position(delta_time);
}
unsigned int player_shot = 0;

void Attacking::execute(float delta_time) {
  // execute todo actions
  // only RotateLeft and RotateRight are supported in Attacking state
  for (auto &action_status : m_todo_actions) {
    switch (action_status.first) {
    case StateMachine::Action::RotateLeft:
    case StateMachine::Action::RotateRight:
      Alive::do_rotate_action(action_status, delta_time);
      break;

    case StateMachine::Action::Shoot: {
      auto [finished, global_transformation] =
          m_owner.get_animation(action_status.first).update(delta_time);

      if (finished) {
        m_owner.m_owner.shoot_player();
        action_status.second = StateMachine::ActionStatus::Success;
      }
    }

    break;

    default:
      assert(false && "not supported action in attacking state");
    }
  }

  // rotate spine bone manually in order to follow the moving target
  m_owner.m_owner.rotate_spine(delta_time);
}

void Attacking::register_todo_action(StateMachine::Action action) {
  assert((action == StateMachine::Action::RotateLeft ||
          action == StateMachine::Action::RotateRight ||
          action == StateMachine::Action::Shoot) &&
         "valid Attacking action");
  EnemyState::register_todo_action(action);
}

// ----------------- Chasing state -------------------------------
Chasing::Chasing(StateMachine &owner)
    : Alive(owner, StateMachine::StateName::Chasing,
            StateMachine::Position::Attacking) {}

bool Chasing::enter(float delta_time) {
  if (m_owner.in_state(m_state_name)) {
    // already in this state
    return true;
  }

  return Alive::go_to_attacking_position(delta_time);
}

void Chasing::execute(float delta_time) {
  for (auto &action_status : m_todo_actions) {
    switch (action_status.first) {
    case StateMachine::Action::RotateLeft:
    case StateMachine::Action::RotateRight:
      Alive::do_rotate_action(action_status, delta_time);
      break;

    default:
      assert(false && "not supported action in chasing state");
    }
  }

  // rotate spine bone manually in order to follow the moving target
  m_owner.m_owner.rotate_spine(delta_time);
}

void Chasing::register_todo_action(StateMachine::Action action) {
  assert((action == StateMachine::Action::RotateLeft ||
          action == StateMachine::Action::RotateRight) &&
         "valid Chasing action");
  EnemyState::register_todo_action(action);
}

// --------------------- Patrolling State -----------------------------
Patrolling::Patrolling(StateMachine &owner)
    : Alive(owner, StateMachine::StateName::Patrolling,
            StateMachine::Position::Standing) {}

bool Patrolling::enter(float delta_time) {
  if (m_owner.in_state(m_state_name)) {
    // already in this state
    return true;
  }

  // make sure enemy is transitioning to this state
  assert(!m_owner.m_current_state && m_owner.m_transitioning_state &&
         m_owner.m_transitioning_state->name() == m_state_name &&
         "enemy transitioning valid");

  if (m_entering_action.second == StateMachine::ActionStatus::Created) {
    // create "transition" animation from current position to the start position
    // - Standing
    m_owner.create_transition_to_position_animation(
        m_start_position, TRANSITION_TO_STANDING_POSITION);
    m_entering_action.second = StateMachine::ActionStatus::Running;
  }

  if (m_entering_action.second == StateMachine::ActionStatus::Running) {
    // update transitioning animation
    auto [finished, global_transformation] =
        m_owner.get_animation(m_entering_action.first).update(delta_time);

    if (finished) {
      // entering done
      m_entering_action.second = StateMachine::ActionStatus::Success;
    }

    return finished;
  }

  // if action status is success, enemy should be in this state
  assert(false);
}

void Patrolling::execute(float delta_time) {
  for (auto &action_status : m_todo_actions) {
    switch (action_status.first) {
    case StateMachine::Action::Walk:

      if (action_status.second == StateMachine::ActionStatus::Created) {
        // find an engle the enemy needs to be rotated in order to start walking
        // along the path
        m_angle_to_rotate = get_angle_to_rotate_user_transformation();
        // create "transition" animation from the current position to the first
        // frame of walking animation
        m_owner.create_transition_to_animation(action_status.first,
                                               TRANSITION_TO_WALK_ANIMATION);
        action_status.second = StateMachine::ActionStatus::Preparing;
      }

      if (action_status.second == StateMachine::ActionStatus::Preparing) {
        if (fabs(m_angle_to_rotate) > EPS) {
          // angle to rotate the enemy is not zero so rotate the entire enemy
          rotate_user_transformation(delta_time);
        } else {
          // enemy is facing the path, update transitioning to the first frame
          // of walking animtion
          auto [finished, global_transformation] =
              m_owner.get_animation(StateMachine::Action::Transition)
                  .update(delta_time);

          if (finished) {
            action_status.second = StateMachine::ActionStatus::Running;
          }
        }
      } else if (action_status.second == StateMachine::ActionStatus::Running) {
        auto [finished, global_transformation] =
            m_owner.get_animation(action_status.first).update(delta_time);

        float delta_distance =
            get_delta_distance_and_update_origin(global_transformation);

        if (finished) {
          // one cycle is finished, reset local origin
          m_local_origin = glm::vec3(0, 0, 0);
        }

        auto point_direction =
            m_path.get_next_point_and_direction(delta_distance);
        point_direction.second[1] = 0;

        m_owner.m_owner.set_transformation(
            point_direction.first,
            get_orientation_angle_for_user_transformation(
                point_direction.second));

        if (m_path.is_path_done()) {
          // walking along the path is done
          m_path = {};
          m_local_origin = glm::vec3(0, 0, 0);
          // create "transition" animation from the current position to the
          // standing position
          m_owner.create_transition_to_position_animation(
              StateMachine::Position::Standing,
              TRANSITION_TO_STANDING_POSITION);
          action_status.second = StateMachine::ActionStatus::Finishing;
        }
      } else if (action_status.second ==
                 StateMachine::ActionStatus::Finishing) {

        // update transition animtion to the standing position
        auto [finished, global_transformation] =
            m_owner.get_animation(StateMachine::Action::Transition)
                .update(delta_time);

        if (finished) {
          // transition is done
          action_status.second = StateMachine::ActionStatus::Success;
        }
      }

      break;

    default:
      assert(false && "not supported action");
      break;
    }
  }
}

void Patrolling::exit() {
  EnemyState::exit();
  m_local_origin = glm::vec3(0, 0, 0);
  m_path = {};
}

void Patrolling::register_todo_action(StateMachine::Action action) {
  assert(action == StateMachine::Action::Walk && "valid Patrolling action");
  EnemyState::register_todo_action(action);
}

void Patrolling::set_path(Path path) { m_path = std::move(path); }

float Patrolling::get_angle_to_rotate_user_transformation() {
  // angle to rotate is angle between enemy's face direction and first path
  // direction

  // get direction of the first point on the curve path(0+delta) - path(0)
  auto first_direction = m_path.get_next_point_and_direction(0).second;
  first_direction[1] = 0;

  return glm::degrees(glm::orientedAngle(
      // enemy's front direction vector
      glm::normalize(m_owner.m_owner.get_front_direction().second),
      // first path direction
      glm::normalize(first_direction),
      // reference in order to determine sign for oriented angle
      glm::vec3(0, 1, 0)));
}

void Patrolling::rotate_user_transformation(float delta_time) {
  float delta_angle = m_angle_to_rotate > 0
                          ? std::min(100 * delta_time, m_angle_to_rotate)
                          : std::max(-100 * delta_time, m_angle_to_rotate);
  // rotate the entire enemy for some small angle
  m_owner.m_owner.rotate_transformation(delta_angle);
  // update angle to rotate enemy
  m_angle_to_rotate -= delta_angle;
}

float Patrolling::get_orientation_angle_for_user_transformation(
    const glm::vec3 &target_orientation) const {
  // find target angle (not delta angle) that needs to be set in order to enemy
  // to face the target orientation vector

  // find angle between enemy's original front direction and target orientation
  // vector because we will set this angle in enemy transformation which is
  // applied to enemy's original position and orientation
  return glm::degrees(glm::orientedAngle(
      // enemy's (original !!!) front direction vector
      Enemy::FRONT_DIRECTION,
      // target orientation
      glm::normalize(target_orientation),
      // reference in order to determine sign for oriented angle
      glm::vec3(0, 1, 0)));
}

float Patrolling::get_delta_distance_and_update_origin(
    const glm::mat4 &global_transformation) {
  // enemy's origin in user transformation system = global_transformation *
  // (0,0,0)

  // previous time was i, current time is j
  // enemy's origin in user t. system:   (0,0,0)  |
  // global_transformation_i*(0,0,0)   |     global_transformation_j*(0,0,0)
  // animation time:                        0     |                i | j
  //                                                         m_local_origin
  //                                                         new_local_origin
  glm::vec3 new_local_origin = global_transformation * glm::vec4(0, 0, 0, 1);

  // in order to find how much enemy's position is changed, we need to take into
  // account user transformation scale
  float delta_distance =
      Enemy::SCALING_FACTOR * glm::length(new_local_origin - m_local_origin);

  m_local_origin = new_local_origin;

  return delta_distance;
}
