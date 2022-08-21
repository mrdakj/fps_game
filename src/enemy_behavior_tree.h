#ifndef _ENEMY_BEHAVIOR_TREE_H_
#define _ENEMY_BEHAVIOR_TREE_H_

#include "enemy_state_machine.h"
#include "node.h"

#include <memory>
#include <unordered_map>

class Enemy;

class IsShot;
class PlayerVisible;
class PlayerInRange;
class Rotate;
class Idle;
class Patrolling;
class FindPath;
class ExecutePath;
class SetState;
class UnderAim;
class CanRotateSpine;
class Shoot;

class EnemyBT {
  friend class IsShot;
  friend class PlayerVisible;
  friend class PlayerInRange;
  friend class Rotate;
  friend class Idle;
  friend class Patrolling;
  friend class FindPath;
  friend class ExecutePath;
  friend class SetState;
  friend class UnderAim;
  friend class CanRotateSpine;
  friend class Shoot;

public:
  EnemyBT(Enemy &enemy);
  void update();

private:
  enum class BlackboardKeys { PlayerVisible, Aim };

  std::unique_ptr<Node> construct_bt();

  Enemy &m_enemy;
  std::unique_ptr<Node> m_bt_root;
  std::unordered_map<BlackboardKeys, int> m_blackboard;
};

class EnemyBTNode : public Node {
protected:
  EnemyBTNode(EnemyBT &bt);

  EnemyBT &m_bt;
};

class EnemyBTAction : public EnemyBTNode {
protected:
  EnemyBTAction(EnemyBT &bt, StateMachine::Action action);

  StateMachine::Action m_action;
};

class IsShot : public EnemyBTNode {
public:
  IsShot(EnemyBT &bt);
  NodeState tick() override;
};

class PlayerVisible : public EnemyBTNode {
public:
  PlayerVisible(EnemyBT &bt);
  NodeState tick() override;
};

class UnderAim : public EnemyBTNode {
public:
  UnderAim(EnemyBT &bt);
  NodeState tick() override;
};

class CanRotateSpine : public EnemyBTNode {
public:
  CanRotateSpine(EnemyBT &bt);
  NodeState tick() override;
};

class Rotate : public EnemyBTNode {
public:
  Rotate(EnemyBT &bt);
  NodeState tick() override;
};

class FindPath : public EnemyBTNode {
public:
  FindPath(EnemyBT &bt);
  NodeState tick() override;
};

class SetState : public EnemyBTNode {
public:
  SetState(EnemyBT &bt, StateMachine::StateName state_name);
  NodeState tick() override;

private:
  StateMachine::StateName m_state_name;
};

class Shoot : public EnemyBTAction {
public:
  Shoot(EnemyBT &bt);
  NodeState tick() override;
};

class ExecutePath : public EnemyBTAction {
public:
  ExecutePath(EnemyBT &bt);
  NodeState tick() override;
};

#endif /* _ENEMY_BEHAVIOR_TREE_H_ */