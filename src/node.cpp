#include "node.h"

ControlFlowNode::ControlFlowNode(std::unique_ptr<Node> child1) {
  m_children.push_back(std::move(child1));
}

ControlFlowNode::ControlFlowNode(std::unique_ptr<Node> child1,
                                 std::unique_ptr<Node> child2) {
  m_children.push_back(std::move(child1));
  m_children.push_back(std::move(child2));
}

ControlFlowNode::ControlFlowNode(std::unique_ptr<Node> child1,
                                 std::unique_ptr<Node> child2,
                                 std::unique_ptr<Node> child3) {
  m_children.push_back(std::move(child1));
  m_children.push_back(std::move(child2));
  m_children.push_back(std::move(child3));
}

ControlFlowNode::ControlFlowNode(std::unique_ptr<Node> child1,
                                 std::unique_ptr<Node> child2,
                                 std::unique_ptr<Node> child3,
                                 std::unique_ptr<Node> child4) {
  m_children.push_back(std::move(child1));
  m_children.push_back(std::move(child2));
  m_children.push_back(std::move(child3));
  m_children.push_back(std::move(child4));
}

void ControlFlowNode::halt() {
  for (auto &m_child : m_children) {
    m_child->halt();
  }
  m_index = 0;
}

DecoratorNode::DecoratorNode(std::unique_ptr<Node> child)
    : m_child(std::move(child)) {}

void DecoratorNode::halt() { m_child->halt(); }

SequenceNode::SequenceNode(std::unique_ptr<Node> child1,
                           std::unique_ptr<Node> child2)
    : ControlFlowNode(std::move(child1), std::move(child2)) {}

SequenceNode::SequenceNode(std::unique_ptr<Node> child1,
                           std::unique_ptr<Node> child2,
                           std::unique_ptr<Node> child3)
    : ControlFlowNode(std::move(child1), std::move(child2), std::move(child3)) {
}

SequenceNode::SequenceNode(std::unique_ptr<Node> child1,
                           std::unique_ptr<Node> child2,
                           std::unique_ptr<Node> child3,
                           std::unique_ptr<Node> child4)
    : ControlFlowNode(std::move(child1), std::move(child2), std::move(child3),
                      std::move(child4)) {}

NodeState SequenceNode::tick() {
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

ParallelSequenceNode::ParallelSequenceNode(std::unique_ptr<Node> child1,
                                           std::unique_ptr<Node> child2)
    : ControlFlowNode(std::move(child1), std::move(child2)) {}

ParallelSequenceNode::ParallelSequenceNode(std::unique_ptr<Node> child1,
                                           std::unique_ptr<Node> child2,
                                           std::unique_ptr<Node> child3)
    : ControlFlowNode(std::move(child1), std::move(child2), std::move(child3)) {
}

NodeState ParallelSequenceNode::tick() {
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

void ParallelSequenceNode::halt() {
  ControlFlowNode::halt();
  m_done.clear();
}

ReactiveSequenceNode::ReactiveSequenceNode(std::unique_ptr<Node> child1,
                                           std::unique_ptr<Node> child2)
    : ControlFlowNode(std::move(child1), std::move(child2)) {}

ReactiveSequenceNode::ReactiveSequenceNode(std::unique_ptr<Node> child1,
                                           std::unique_ptr<Node> child2,
                                           std::unique_ptr<Node> child3)
    : ControlFlowNode(std::move(child1), std::move(child2), std::move(child3)) {
}

NodeState ReactiveSequenceNode::tick() {
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

FallbackNode::FallbackNode(std::unique_ptr<Node> child1,
                           std::unique_ptr<Node> child2)
    : ControlFlowNode(std::move(child1), std::move(child2)) {}

FallbackNode::FallbackNode(std::unique_ptr<Node> child1,
                           std::unique_ptr<Node> child2,
                           std::unique_ptr<Node> child3)
    : ControlFlowNode(std::move(child1), std::move(child2), std::move(child3)) {
}

NodeState FallbackNode::tick() {
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

ReactiveFallbackNode::ReactiveFallbackNode(std::unique_ptr<Node> child1,
                                           std::unique_ptr<Node> child2)
    : ControlFlowNode(std::move(child1), std::move(child2)) {}

ReactiveFallbackNode::ReactiveFallbackNode(std::unique_ptr<Node> child1,
                                           std::unique_ptr<Node> child2,
                                           std::unique_ptr<Node> child3)
    : ControlFlowNode(std::move(child1), std::move(child2), std::move(child3)) {
}

NodeState ReactiveFallbackNode::tick() {
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

void ReactiveFallbackNode::halt(int index) {
  for (int i = index; i < m_children.size(); ++i) {
    m_children[i]->halt();
  }
  m_index = 0;
}

Invert::Invert(std::unique_ptr<Node> child) : DecoratorNode(std::move(child)) {}

NodeState Invert::tick() {
  auto child_status = m_child->tick();

  if (child_status == NodeState::Running) {
    return NodeState::Running;
  }

  halt();
  return child_status == NodeState::Success ? NodeState::Failure
                                            : NodeState::Success;
}

ForceFailure::ForceFailure(std::unique_ptr<Node> child)
    : DecoratorNode(std::move(child)) {}

NodeState ForceFailure::tick() {
  auto child_status = m_child->tick();

  if (child_status == NodeState::Running) {
    return NodeState::Running;
  }

  halt();
  return NodeState::Failure;
}

ForceSuccess::ForceSuccess(std::unique_ptr<Node> child)
    : DecoratorNode(std::move(child)) {}

NodeState ForceSuccess::tick() {
  auto child_status = m_child->tick();

  if (child_status == NodeState::Running) {
    return NodeState::Running;
  }

  halt();
  return NodeState::Success;
}
