#ifndef _ENEMY_BEHAVIOR_TREE_H_
#define _ENEMY_BEHAVIOR_TREE_H_

#include "enemy_state_machine.h"
#include "node.h"

#include <memory>
#include <unordered_map>

class Enemy;

class IsShot;
class PlayerVisible;
class PlayerNoticed;
class PlayerDead;
class PlayerUnderDomain;
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
  friend class PlayerNoticed;
  friend class PlayerDead;
  friend class PlayerUnderDomain;
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
  void reset();
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

class PlayerNoticed : public EnemyBTNode {
public:
  PlayerNoticed(EnemyBT &bt, unsigned int duration_seconds);
  NodeState tick() override;

private:
  // how many seconds should player noticed last
  unsigned int m_duration_seconds;
};

class PlayerDead : public EnemyBTNode {
public:
  PlayerDead(EnemyBT &bt);
  NodeState tick() override;
};

class PlayerUnderDomain : public EnemyBTNode {
public:
  PlayerUnderDomain(EnemyBT &bt);
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

  void halt() override;
};

class ExecutePath : public EnemyBTAction {
public:
  ExecutePath(EnemyBT &bt);
  NodeState tick() override;
};

#endif /* _ENEMY_BEHAVIOR_TREE_H_ */
