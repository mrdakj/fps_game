#include "enemy_behavior_tree.h"

// helper function to convert enemy action status to node status
NodeState from_action_status(Enemy::ActionStatus action_status) {
  return action_status == Enemy::ActionStatus::Success ? NodeState::Success
                                                       : NodeState::Running;
}

// ------------ EnemyBT -------------------
EnemyBT::EnemyBT(Enemy *enemy, const Player &player)
    : m_enemy(enemy), m_player(player), m_bt_root(construct_bt()) {}

void EnemyBT::update() { m_bt_root->tick(); }

std::unique_ptr<Node> EnemyBT::construct_bt() {
  // ------ Dead state ---------
  auto is_shot_condition = std::make_unique<IsShot>(*this);
  auto fall_dead_action = std::make_unique<FallDead>(*this);
  auto dead_state = std::make_unique<SequenceNode>(std::move(is_shot_condition),
                                                   std::move(fall_dead_action));
  // ------ Dead state end ---------

  // ------- Attacking state ----------------
  // check if enemy is visible
  auto enemy_visible_condition = std::make_unique<PlayerVisible>(*this);

  // move the gun up
  auto gun_up_action = std::make_unique<Gun>(*this, true);

  // rotate spine
  auto rotate_spine_action1 = std::make_unique<Rotate>(*this, true);
  // always try to rotate spine
  auto rotate_spine_action2 = std::make_unique<Rotate>(*this, true);
  auto always_rotate_spine2 =
      std::make_unique<ForceSuccess>(std::move(rotate_spine_action2));
  // rotate body
  auto rotate_body_action = std::make_unique<Rotate>(*this, false);
  // rotate body and spine together even if one of them failed before (rotate
  // spine can fail)
  auto rotate_body_and_spine_action = std::make_unique<ReactiveSequenceNode>(
      std::move(always_rotate_spine2), std::move(rotate_body_action));
  // first try to rotate only spine, if that fails try to rotate body and spine
  // together
  auto chase_player_action = std::make_unique<FallbackNode>(
      std::move(rotate_spine_action1), std::move(rotate_body_and_spine_action));

  // try to move gun up and to chanse player in parallel
  auto parallel_gun_chase_action = std::make_unique<ParallelSequenceNode>(
      std::move(gun_up_action), std::move(chase_player_action));

  auto attacking_state = std::make_unique<SequenceNode>(
      std::move(enemy_visible_condition), std::move(parallel_gun_chase_action));
  // ------- Attacking state end ----------------

  // --------- Idling/Patrolling state -------------
  // put the gun down
  auto gun_down_action = std::make_unique<Gun>(*this, false);

  auto idle_patrolling_state = std::make_unique<SequenceNode>(
      std::move(gun_down_action), std::make_unique<Idle>(*this),
      std::make_unique<Patrolling>(*this));
  // --------- Idling/Patrolling state end -------------

  // --------- Alive state -----------------------
  auto alive_state = std::make_unique<FallbackNode>(
      std::move(attacking_state), std::move(idle_patrolling_state));
  // --------- Alive state end -----------------------

  // always first check if enemy is dead and if he is, halt everything running
  // in alive state
  return std::make_unique<ReactiveFallbackNode>(std::move(dead_state),
                                                std::move(alive_state));
}
// ------------ EnemyBT End -------------------

// ------------ EnemyBTAction -----------
EnemyBTAction::EnemyBTAction(EnemyBT &bt, Enemy::Action action)
    : m_bt(bt), m_action(action) {}
// ------------ EnemyBTAction End -----------

// ------------ EnemyBTCondition -----------
EnemyBTCondition::EnemyBTCondition(EnemyBT &bt) : m_bt(bt) {}
// ------------ EnemyBTCondition End -----------

// --------- IsShot -------------
IsShot::IsShot(EnemyBT &bt) : EnemyBTCondition(bt) {}

NodeState IsShot::tick() {
  return (m_bt.m_enemy->is_shot()) ? NodeState::Success : NodeState::Failure;
}
// --------- IsShot End -------------

// --------- FallDead -------------
FallDead::FallDead(EnemyBT &bt) : EnemyBTAction(bt, Enemy::Action::FallDead) {}

NodeState FallDead::tick() {
  auto action_status = m_bt.m_enemy->get_action_status_and_remove_successful(
      Enemy::Action::FallDead);

  if (action_status) {
    return from_action_status(*action_status);
  }

  // action is not active - create a new one if needed
  if (m_bt.m_enemy->enemy_state() != Enemy::EnemyState::Dead) {
    m_bt.m_enemy->register_new_todo_action(m_action);
    return NodeState::Running;
  }
  return NodeState::Success;
}

void FallDead::halt() { m_bt.m_enemy->remove_todo_action(m_action); }
// --------- FallDead End -------------

// --------- PlayerVisible -------------
PlayerVisible::PlayerVisible(EnemyBT &bt) : EnemyBTCondition(bt) {}

NodeState PlayerVisible::tick() {
  auto [gun_O, gun_direction] = m_bt.m_enemy->get_gun_direction();
  auto player_position = m_bt.m_player.camera().position();
  auto player_direction = player_position - gun_O;
  player_direction[1] = 0;
  gun_direction[1] = 0;
  gun_direction = glm::normalize(gun_direction);
  player_direction = glm::normalize(player_direction);
  auto cross_product = glm::cross(gun_direction, player_direction);
  float angle = (cross_product[1] > 0 ? 1 : -1) *
                glm::degrees(glm::angle(gun_direction, player_direction));

  m_bt.m_blackboard[EnemyBT::BlackboardKeys::GunPlayerAngle] = angle;

  // TODO fix magic numbers
  if (angle < 100 && angle > -100) {
    return NodeState::Success;
  }

  return NodeState::Failure;
}
// --------- PlayerVisible End -------------

// --------- Gun -------------
Gun::Gun(EnemyBT &bt, bool up)
    : EnemyBTAction(bt, up ? Enemy::Action::GunUp : Enemy::Action::GunDown) {}

NodeState Gun::tick() {
  auto action_status =
      m_bt.m_enemy->get_action_status_and_remove_successful(m_action);

  if (action_status) {
    return from_action_status(*action_status);
  }

  // action is not active - create a new one if needed
  if (m_action == Enemy::Action::GunUp &&
          m_bt.m_enemy->gun_state() != Enemy::GunState::Up ||
      m_action == Enemy::Action::GunDown &&
          m_bt.m_enemy->gun_state() != Enemy::GunState::Down) {
    m_bt.m_enemy->register_new_todo_action(m_action);
    return NodeState::Running;
  }

  return NodeState::Success;
}

void Gun::halt() { m_bt.m_enemy->remove_todo_action(m_action); }
// --------- Gun End -------------

// --------- Rotate -------------
Rotate::Rotate(EnemyBT &bt, bool in_place)
    : EnemyBTAction(bt), m_in_place(in_place) {}

NodeState Rotate::tick() {
  auto gun_player_angle_it =
      m_bt.m_blackboard.find(EnemyBT::BlackboardKeys::GunPlayerAngle);
  assert(gun_player_angle_it != m_bt.m_blackboard.end() &&
         "gun palyer angle properly set");
  float gun_player_angle = gun_player_angle_it->second;

  if (m_in_place) {
    // TODO fix copy paste code
    // need to get the latest enemy gun-player angle for spine rotation
    // because spine rotation is not an animation
    // if some other animation is running (gun up, rotate body) enemy-player
    // angle in blackboard won't be updated
    auto [gun_O, gun_direction] = m_bt.m_enemy->get_gun_direction();
    auto player_position = m_bt.m_player.camera().position();
    auto player_direction = player_position - gun_O;
    player_direction[1] = 0;
    gun_direction[1] = 0;
    gun_direction = glm::normalize(gun_direction);
    player_direction = glm::normalize(player_direction);
    auto cross_product = glm::cross(gun_direction, player_direction);
    float rotate_angle =
        (cross_product[1] > 0 ? 1 : -1) *
        glm::degrees(glm::angle(gun_direction, player_direction));

    NodeState spine_rotation_state = NodeState::Success;
    if (fabs(rotate_angle) < 5) {
      // stop rotating and return success
      m_bt.m_enemy->is_rotatng_spine_left = false;
      m_bt.m_enemy->is_rotatng_spine_right = false;
      spine_rotation_state = NodeState::Success;
    } else {

      bool left = rotate_angle > 0;
      // stop current rotation and try to start a new one
      auto can_rotate = m_bt.m_enemy->can_rotate_spine(left);
      if (left) {
        m_bt.m_enemy->is_rotatng_spine_left = can_rotate;
        m_bt.m_enemy->is_rotatng_spine_right = false;
      } else {
        m_bt.m_enemy->is_rotatng_spine_left = false;
        m_bt.m_enemy->is_rotatng_spine_right = can_rotate;
      }
      spine_rotation_state =
          can_rotate ? NodeState::Running : NodeState::Failure;
    }

    return spine_rotation_state;
  }

  m_action = gun_player_angle > 0 ? Enemy::Action::RotateLeft
                                  : Enemy::Action::RotateRight;

  auto action_status =
      m_bt.m_enemy->get_action_status_and_remove_successful(m_action);

  if (action_status) {
    return from_action_status(*action_status);
  }

  // action is not active - create a new one
  m_bt.m_enemy->register_new_todo_action(m_action);
  return NodeState::Running;
}

void Rotate::halt() {
  if (m_in_place) {
    m_bt.m_enemy->is_rotatng_spine_left = false;
    m_bt.m_enemy->is_rotatng_spine_right = false;
  } else {
    m_bt.m_enemy->remove_todo_action(m_action);
  }
}
// --------- Rotate End -----------------

// ---------- Idle --------------
Idle::Idle(EnemyBT &bt) : EnemyBTAction(bt) {}

// TODO implement
NodeState Idle::tick() { return NodeState::Success; }
// ---------- Idle End --------------

// ---------- Patrolling --------------
Patrolling::Patrolling(EnemyBT &bt) : EnemyBTAction(bt) {}

// TODO implement
NodeState Patrolling::tick() { return NodeState::Success; }
// ---------- Patrolling End --------------
