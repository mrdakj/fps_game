#ifndef _ENEMY_BEHAVIOR_TREE_H_
#define _ENEMY_BEHAVIOR_TREE_H_

#include "enemy.h"
#include "node.h"

#include <glm/gtx/vector_angle.hpp>
#include <memory>

class IsShot;
class FallDead;
class PlayerVisible;
class PlayerInRange;
class Gun;
class Rotate;
class Idle;
class Patrolling;

class EnemyBT {
  friend class IsShot;
  friend class FallDead;
  friend class PlayerVisible;
  friend class PlayerInRange;
  friend class Gun;
  friend class Rotate;
  friend class Idle;
  friend class Patrolling;

public:
  EnemyBT(Enemy *enemy, const Player &player);
  void update();

private:
  enum class BlackboardKeys { GunPlayerAngle };

  std::unique_ptr<Node> construct_bt();

  Enemy *m_enemy;
  const Player &m_player;
  std::unique_ptr<Node> m_bt_root;
  std::unordered_map<BlackboardKeys, float> m_blackboard;
};

class EnemyBTAction : public Node {
protected:
  EnemyBTAction(EnemyBT &bt, Enemy::Action action = Enemy::Action::None);

  EnemyBT &m_bt;
  Enemy::Action m_action;
};

class EnemyBTCondition : public Node {
protected:
  EnemyBTCondition(EnemyBT &bt);

  EnemyBT &m_bt;
};

class IsShot : public EnemyBTCondition {
public:
  IsShot(EnemyBT &bt);
  NodeState tick() override;
};

class FallDead : public EnemyBTAction {
public:
  FallDead(EnemyBT &bt);
  NodeState tick() override;
  void halt() override;
};

class PlayerVisible : public EnemyBTCondition {
public:
  PlayerVisible(EnemyBT &bt);
  NodeState tick() override;
};

class Gun : public EnemyBTAction {
public:
  Gun(EnemyBT &bt, bool up);
  NodeState tick() override;
  void halt() override;
};

class Rotate : public EnemyBTAction {
public:
  Rotate(EnemyBT &bt, bool in_place);
  NodeState tick() override;
  void halt() override;

private:
  bool m_in_place;
};

class Idle : public EnemyBTAction {
public:
  Idle(EnemyBT &bt);
  NodeState tick() override;
};

class Patrolling : public EnemyBTAction {
public:
  Patrolling(EnemyBT &bt);
  NodeState tick() override;
};

#endif /* _ENEMY_BEHAVIOR_TREE_H_ */
