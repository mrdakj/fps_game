#ifndef _NODE_H_
#define _NODE_H_

#include "enemy.h"
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_set>

enum class NodeState { Success, Running, Failure };

class Node {
public:
  Node() = default;
  virtual ~Node() {}
  virtual NodeState tick() = 0;
  virtual void halt() {}
};

class ControlFlowNode : public Node {
protected:
  ControlFlowNode(std::unique_ptr<Node> child1) {
    m_children.push_back(std::move(child1));
  }

  ControlFlowNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2) {
    m_children.push_back(std::move(child1));
    m_children.push_back(std::move(child2));
  }

  ControlFlowNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
                  std::unique_ptr<Node> child3) {
    m_children.push_back(std::move(child1));
    m_children.push_back(std::move(child2));
    m_children.push_back(std::move(child3));
  }

  void halt() override {
    for (auto &m_child : m_children) {
      m_child->halt();
    }
    m_index = 0;
  }

  std::vector<std::unique_ptr<Node>> m_children;
  unsigned int m_index = 0;
};

class DecoratorNode : public Node {
public:
  DecoratorNode(std::unique_ptr<Node> child) : m_child(std::move(child)) {}

protected:
  void halt() override { m_child->halt(); }

  std::unique_ptr<Node> m_child;
};

class SequenceNode : public ControlFlowNode {
public:
  SequenceNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2)
      : ControlFlowNode(std::move(child1), std::move(child2)) {}

  SequenceNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
               std::unique_ptr<Node> child3)
      : ControlFlowNode(std::move(child1), std::move(child2),
                        std::move(child3)) {}

  NodeState tick() override {
    while (m_index < m_children.size()) {
      auto child_status = m_children[m_index]->tick();

      if (child_status == NodeState::Success) {
        ++m_index;
      } else if (child_status == NodeState::Running) {
        // at the next tick index will be the same
        return NodeState::Running;
      } else {
        // reset all children because this one failed
        halt();
        return NodeState::Failure;
      }
    }

    // all the children returned success
    halt();
    return NodeState::Success;
  }
};

class ParallelSequenceNode : public ControlFlowNode {
public:
  ParallelSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2)
      : ControlFlowNode(std::move(child1), std::move(child2)) {}

  ParallelSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2,
                       std::unique_ptr<Node> child3)
      : ControlFlowNode(std::move(child1), std::move(child2),
                        std::move(child3)) {}

  NodeState tick() override {
    NodeState result = NodeState::Success;

    for (int index = 0; index < m_children.size(); ++index) {
      if (m_done.find(index) != m_done.end()) {
        // don't run successfully finished child
        continue;
      }

      auto child_status = m_children[index]->tick();

      if (child_status == NodeState::Failure) {
        halt();
        return NodeState::Failure;
      } else if (child_status == NodeState::Running) {
        result = NodeState::Running;
      } else if (child_status == NodeState::Success) {
        m_done.insert(index);
      }
    }

    if (result == NodeState::Success) {
      halt();
    }
    return result;
  }

private:
  void halt() override {
    ControlFlowNode::halt();
    m_done.clear();
  }

  std::unordered_set<unsigned int> m_done;
};

class ReactiveSequenceNode : public ControlFlowNode {
public:
  ReactiveSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2)
      : ControlFlowNode(std::move(child1), std::move(child2)) {}

  ReactiveSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2,
                       std::unique_ptr<Node> child3)
      : ControlFlowNode(std::move(child1), std::move(child2),
                        std::move(child3)) {}

  NodeState tick() override {
    for (int index = 0; index < m_children.size(); ++index) {
      auto child_status = m_children[index]->tick();

      if (child_status == NodeState::Running) {
        // at the next tick index will be the same
        return NodeState::Running;
      } else if (child_status == NodeState::Failure) {
        // reset all children because this one failed
        halt();
        return NodeState::Failure;
      }
    }

    // all the children returned success
    halt();
    return NodeState::Success;
  }
};

class FallbackNode : public ControlFlowNode {
public:
  FallbackNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2)
      : ControlFlowNode(std::move(child1), std::move(child2)) {}

  FallbackNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
               std::unique_ptr<Node> child3)
      : ControlFlowNode(std::move(child1), std::move(child2),
                        std::move(child3)) {}

  NodeState tick() override {
    while (m_index < m_children.size()) {
      auto child_status = m_children[m_index]->tick();

      if (child_status == NodeState::Success) {
        halt();
        return NodeState::Success;
      } else if (child_status == NodeState::Running) {
        // at the next tick index will be the same
        return NodeState::Running;
      } else {
        // continue to the next child
        ++m_index;
      }
    }

    // all the children returned failure
    halt();
    return NodeState::Failure;
  }
};

class ReactiveFallbackNode : public ControlFlowNode {
public:
  ReactiveFallbackNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2)
      : ControlFlowNode(std::move(child1), std::move(child2)) {}

  ReactiveFallbackNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2,
                       std::unique_ptr<Node> child3)
      : ControlFlowNode(std::move(child1), std::move(child2),
                        std::move(child3)) {}

  NodeState tick() override {
    for (int index = 0; index < m_children.size(); ++index) {
      auto child_status = m_children[index]->tick();

      if (child_status == NodeState::Success) {
        // halt all the children
        ControlFlowNode::halt();
        return NodeState::Success;
      } else if (child_status == NodeState::Running) {
        // halt subsequent siblings
        halt(index + 1);
        return NodeState::Running;
      }
    }

    // all the children returned failure
    // halt all the children
    ControlFlowNode::halt();
    return NodeState::Failure;
  }

private:
  void halt(int index) {
    for (int i = index; i < m_children.size(); ++i) {
      m_children[i]->halt();
    }
    m_index = 0;
  }
};

class ForceFailure : public DecoratorNode {
public:
  ForceFailure(std::unique_ptr<Node> child) : DecoratorNode(std::move(child)) {}

  NodeState tick() override {
    auto child_status = m_child->tick();

    if (child_status == NodeState::Running) {
      return NodeState::Running;
    }

    halt();
    return NodeState::Failure;
  }
};

class ForceSuccess : public DecoratorNode {
public:
  ForceSuccess(std::unique_ptr<Node> child) : DecoratorNode(std::move(child)) {}

  NodeState tick() override {
    auto child_status = m_child->tick();

    if (child_status == NodeState::Running) {
      return NodeState::Running;
    }

    halt();
    return NodeState::Success;
  }
};

#endif /* _NODE_H_ */
