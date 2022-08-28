#include "enemy_behavior_tree.h"
#include "enemy.h"
#include <memory>

#include <glm/gtx/vector_angle.hpp>

// helper function to convert enemy action status to node status
NodeState from_action_status(StateMachine::ActionStatus action_status) {
  return action_status == StateMachine::ActionStatus::Success
             ? NodeState::Success
             : NodeState::Running;
}

// ------------ EnemyBT -------------------
EnemyBT::EnemyBT(Enemy &enemy) : m_enemy(enemy), m_bt_root(construct_bt()) {}

void EnemyBT::reset() { m_bt_root = construct_bt(); }

void EnemyBT::update() {
  m_blackboard.clear();
  m_bt_root->tick();
}

std::unique_ptr<Node> EnemyBT::construct_bt() {
  // ------ Dead state ---------
  auto check_dead_state = std::make_unique<SequenceNode>(
      // is shot
      std::make_unique<IsShot>(*this),
      // go to dead state
      std::make_unique<SetState>(*this, StateMachine::StateName::Dead));
  // ------ Dead state end ---------

  // ------- Attacking state ----------------
  auto shoot = std::make_unique<SequenceNode>(
      // player under aim
      std::make_unique<UnderAim>(*this),
      // shoot
      std::make_unique<Shoot>(*this));

  auto rotate = std::make_unique<SequenceNode>(
      // not rotating spine
      std::make_unique<Invert>(std::make_unique<CanRotateSpine>(*this)),
      // rotate body
      std::make_unique<Rotate>(*this));

  auto shoot_or_aim = std::make_unique<FallbackNode>(
      // shoot if player is under aim
      std::move(shoot),
      // try to aim
      std::move(rotate));

  auto attacking_state = std::make_unique<SequenceNode>(
      // player is visible
      std::make_unique<PlayerVisible>(*this),
      // attack
      std::make_unique<SetState>(*this, StateMachine::StateName::Attacking),
      // force success so we don't go to chasing state if this node fails
      std::make_unique<ForceSuccess>(std::move(shoot_or_aim)));

  // don't use reactive sequence because we will go to a chasing state if player
  // became invisible
  auto check_attacking_state = std::make_unique<SequenceNode>(
      // player not dead
      std::make_unique<Invert>(std::make_unique<PlayerDead>(*this)),
      // player visible
      std::make_unique<PlayerVisible>(*this), std::move(attacking_state));
  // ------- Attacking state end ----------------

  // ------- Chasing state -----------------------
  auto chasing_state = std::make_unique<SequenceNode>(
      std::make_unique<Invert>(std::make_unique<UnderAim>(*this)),
      // not rotating spine
      std::make_unique<Invert>(std::make_unique<CanRotateSpine>(*this)),
      // rotate body
      std::make_unique<Rotate>(*this));

  auto check_chasing_state = std::make_unique<SequenceNode>(
      // player not dead
      std::make_unique<Invert>(std::make_unique<PlayerDead>(*this)),
      // player seen duration lasts for 10 seconds
      std::make_unique<PlayerSeen>(*this, 10),
      std::make_unique<SetState>(*this, StateMachine::StateName::Chasing),
      // force success so we don't go to patrolling state if this node fails
      std::make_unique<ForceSuccess>(std::move(chasing_state)));
  // ------- Chasing state end -----------------------

  // --------- Idling/Patrolling state -------------
  auto patrolling_state = std::make_unique<SequenceNode>(
      // go to patrlling state
      std::make_unique<SetState>(*this, StateMachine::StateName::Patrolling),
      // find a path
      std::make_unique<FindPath>(*this),
      // execute the path
      std::make_unique<ExecutePath>(*this));

  auto check_patrolling_state = std::make_unique<ReactiveFallbackNode>(
      std::make_unique<SequenceNode>(
          // player not dead
          std::make_unique<Invert>(std::make_unique<PlayerDead>(*this)),
          // player visible
          std::make_unique<PlayerVisible>(*this)),
      std::move(patrolling_state));
  // --------- Idling/Patrolling state end -------------

  // --------- Alive state -----------------------
  auto alive_state = std::make_unique<FallbackNode>(
      std::move(check_attacking_state), std::move(check_chasing_state),
      std::move(check_patrolling_state));
  // --------- Alive state end -----------------------

  // always first check if enemy is dead and if he is, halt everything running
  // in alive state
  return std::make_unique<ReactiveFallbackNode>(std::move(check_dead_state),
                                                std::move(alive_state));
}
// ------------ EnemyBT End -------------------

// ------------- EnemyBTNode -----------------------
EnemyBTNode::EnemyBTNode(EnemyBT &bt) : m_bt(bt) {}
// ------------- EnemyBTNode End -----------------------

// ------------ EnemyBTAction -----------
EnemyBTAction::EnemyBTAction(EnemyBT &bt, StateMachine::Action action)
    : EnemyBTNode(bt), m_action(action) {}
// ------------ EnemyBTAction End -----------

// --------- IsShot -------------
IsShot::IsShot(EnemyBT &bt) : EnemyBTNode(bt) {}

NodeState IsShot::tick() {
  return m_bt.m_enemy.is_shot() ? NodeState::Success : NodeState::Failure;
}
// --------- IsShot End -------------

// --------- PlayerVisible -------------
PlayerVisible::PlayerVisible(EnemyBT &bt) : EnemyBTNode(bt) {}

NodeState PlayerVisible::tick() {
  auto player_visible_it =
      m_bt.m_blackboard.find(EnemyBT::BlackboardKeys::PlayerVisible);

  bool player_visible = player_visible_it != m_bt.m_blackboard.end()
                            ? player_visible_it->second == 1
                            : m_bt.m_enemy.is_player_visible();

  m_bt.m_blackboard[EnemyBT::BlackboardKeys::PlayerVisible] = player_visible;

  if (player_visible) {
    // set player's current position and current time as the last seen
    m_bt.m_enemy.set_player_seen();
  }

  return player_visible ? NodeState::Success : NodeState::Failure;
}
// --------- PlayerVisible End -------------

// ----------- PlayerSeen -------------------
PlayerSeen::PlayerSeen(EnemyBT &bt, unsigned int duration_seconds)
    : EnemyBTNode(bt), m_duration_seconds(duration_seconds) {}

NodeState PlayerSeen::tick() {
  return
      // player was seen at some point
      m_bt.m_enemy.is_player_seen() &&
              // player was seen recentrly
              m_bt.m_enemy.player_seen_seconds_passed() < m_duration_seconds
          ? NodeState::Success
          : NodeState::Failure;
}
// ----------- PlayerSeen End -------------------

// -------------- PlayerDead ----------------------
PlayerDead::PlayerDead(EnemyBT &bt) : EnemyBTNode(bt) {}

NodeState PlayerDead::tick() {
  return m_bt.m_enemy.is_player_dead() ? NodeState::Success
                                       : NodeState::Failure;
}
// -------------- PlayerDead End ----------------------

// --------- Rotate -------------
Rotate::Rotate(EnemyBT &bt) : EnemyBTNode(bt) {}

NodeState Rotate::tick() {
  auto rotate_left_action_status =
      m_bt.m_enemy.get_action_status(StateMachine::Action::RotateLeft);
  auto rotate_right_action_status =
      m_bt.m_enemy.get_action_status(StateMachine::Action::RotateRight);

  assert((!rotate_left_action_status || !rotate_right_action_status) &&
         "cannot have rotate left and right actions at the same time");

  if (rotate_left_action_status) {
    auto node_state = from_action_status(*rotate_left_action_status);
    if (node_state == NodeState::Success) {
      m_bt.m_enemy.remove_todo_action(StateMachine::Action::RotateLeft);
    }
    return node_state;
  }

  if (rotate_right_action_status) {
    auto node_state = from_action_status(*rotate_right_action_status);
    if (node_state == NodeState::Success) {
      m_bt.m_enemy.remove_todo_action(StateMachine::Action::RotateRight);
    }
    return node_state;
  }

  auto aim_it = m_bt.m_blackboard.find(EnemyBT::BlackboardKeys::Aim);
  assert(aim_it != m_bt.m_blackboard.end() && "gun palyer angle properly set");

  bool left = static_cast<Enemy::Aiming>(aim_it->second) == Enemy::Aiming::Left;

  // action is not active - create a new one
  m_bt.m_enemy.register_todo_action(left ? StateMachine::Action::RotateLeft
                                         : StateMachine::Action::RotateRight);
  return NodeState::Running;
}
// --------- Rotate End -----------------

// ------------------------ FindPath -----------------------------------------
FindPath::FindPath(EnemyBT &bt) : EnemyBTNode(bt) {}

NodeState FindPath::tick() {
  return m_bt.m_enemy.find_path() ? NodeState::Success : NodeState::Failure;
}
// -------------------- FindPath End -----------------------------------------

// --------------------- ExecutePath ---------------------------------
ExecutePath::ExecutePath(EnemyBT &bt)
    : EnemyBTAction(bt, StateMachine::Action::Walk) {}

NodeState ExecutePath::tick() {
  auto action_status = m_bt.m_enemy.get_action_status(m_action);

  if (action_status) {
    auto node_state = from_action_status(*action_status);
    if (node_state == NodeState::Success) {
      m_bt.m_enemy.remove_todo_action(m_action);
    }
    return node_state;
  }

  // action is not active - create a new one
  m_bt.m_enemy.register_todo_action(m_action);
  return NodeState::Running;
}
// --------------------- ExecutePath End ---------------------------------

// --------------------- SetState ---------------------------------
SetState::SetState(EnemyBT &bt, StateMachine::StateName state_name)
    : EnemyBTNode(bt), m_state_name(state_name) {}

NodeState SetState::tick() {
  if (m_bt.m_enemy.change_state(m_state_name)) {
    // transition is done
    return NodeState::Success;
  }

  // transition is in progress
  return NodeState::Running;
}
// --------------------- SetState End ---------------------------------

// --------------------- Shoot ---------------------------------
Shoot::Shoot(EnemyBT &bt) : EnemyBTAction(bt, StateMachine::Action::Shoot) {}

NodeState Shoot::tick() {
  auto action_status = m_bt.m_enemy.get_action_status(m_action);

  if (action_status) {
    auto node_state = from_action_status(*action_status);
    if (node_state == NodeState::Success) {
      m_bt.m_enemy.remove_todo_action(m_action);
    }
    return node_state;
  }

  // action is not active - create a new one
  m_bt.m_enemy.start_shooting();
  m_bt.m_enemy.register_todo_action(m_action);
  return NodeState::Running;
}

void Shoot::halt() { m_bt.m_enemy.stop_shooting(); }
// --------------------- Shoot End ---------------------------------

// --------------------- UnderAim ---------------------------------
UnderAim::UnderAim(EnemyBT &bt) : EnemyBTNode(bt) {}

NodeState UnderAim::tick() {
  auto player_under_aim_it =
      m_bt.m_blackboard.find(EnemyBT::BlackboardKeys::Aim);

  if (player_under_aim_it != m_bt.m_blackboard.end()) {
    return player_under_aim_it->second == 0 ? NodeState::Success
                                            : NodeState::Failure;
  }

  Enemy::Aiming aim = m_bt.m_enemy.get_aim();

  m_bt.m_blackboard[EnemyBT::BlackboardKeys::Aim] = static_cast<int>(aim);

  return aim == Enemy::Aiming::UnderAim ? NodeState::Success
                                        : NodeState::Failure;
}
// --------------------- UnderAim End ---------------------------------

// --------------------- CanRotateSpine ---------------------------------
CanRotateSpine::CanRotateSpine(EnemyBT &bt) : EnemyBTNode(bt) {}

NodeState CanRotateSpine::tick() {
  auto aim_it = m_bt.m_blackboard.find(EnemyBT::BlackboardKeys::Aim);
  assert(aim_it != m_bt.m_blackboard.end() && "gun palyer angle properly set");

  bool left = static_cast<Enemy::Aiming>(aim_it->second) == Enemy::Aiming::Left;

  return m_bt.m_enemy.can_rotate_spine(left) ? NodeState::Success
                                             : NodeState::Failure;
}
// --------------------- CanRotateSpine End ---------------------------------
