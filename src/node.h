#ifndef _NODE_H_
#define _NODE_H_

#include <vector>
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
  ControlFlowNode(std::unique_ptr<Node> child1);

  ControlFlowNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2);

  ControlFlowNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
                  std::unique_ptr<Node> child3);

  ControlFlowNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
                  std::unique_ptr<Node> child3, std::unique_ptr<Node> child4);

  void halt() override;

  std::vector<std::unique_ptr<Node>> m_children;
  unsigned int m_index = 0;
};

class DecoratorNode : public Node {
public:
  DecoratorNode(std::unique_ptr<Node> child);

protected:
  void halt() override;

  std::unique_ptr<Node> m_child;
};

class SequenceNode : public ControlFlowNode {
public:
  SequenceNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2);

  SequenceNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
               std::unique_ptr<Node> child3);

  SequenceNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
               std::unique_ptr<Node> child3, std::unique_ptr<Node> child4);

  NodeState tick() override;
};

class ParallelSequenceNode : public ControlFlowNode {
public:
  ParallelSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2);

  ParallelSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2,
                       std::unique_ptr<Node> child3);

  NodeState tick() override;

private:
  void halt() override;

  std::unordered_set<unsigned int> m_done;
};

class ReactiveSequenceNode : public ControlFlowNode {
public:
  ReactiveSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2);

  ReactiveSequenceNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2,
                       std::unique_ptr<Node> child3);

  NodeState tick() override;
};

class FallbackNode : public ControlFlowNode {
public:
  FallbackNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2);

  FallbackNode(std::unique_ptr<Node> child1, std::unique_ptr<Node> child2,
               std::unique_ptr<Node> child3);

  NodeState tick() override;
};

class ReactiveFallbackNode : public ControlFlowNode {
public:
  ReactiveFallbackNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2);

  ReactiveFallbackNode(std::unique_ptr<Node> child1,
                       std::unique_ptr<Node> child2,
                       std::unique_ptr<Node> child3);

  NodeState tick() override;

private:
  void halt(int index);
};

class Invert : public DecoratorNode {
public:
  Invert(std::unique_ptr<Node> child);

  NodeState tick() override;
};

class ForceFailure : public DecoratorNode {
public:
  ForceFailure(std::unique_ptr<Node> child);

  NodeState tick() override;
};

class ForceSuccess : public DecoratorNode {
public:
  ForceSuccess(std::unique_ptr<Node> child);

  NodeState tick() override;
};

#endif /* _NODE_H_ */
