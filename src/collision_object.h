#ifndef _COLLISION_OBJECT_H_
#define _COLLISION_OBJECT_H_

#include "aabb.h"
#include "bounding_box.h"
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

template <typename T> struct BVHNode {
  BVHNode(T &&vol = {}) : volume(std::forward<T>(vol)) {}
  BVHNode(const T &vol) : volume(vol) {}

  T volume;
  std::vector<std::unique_ptr<BVHNode<T>>> children;
};

template <typename T> class CollisionObject {
public:
  virtual ~CollisionObject() {}

  virtual std::unique_ptr<BVHNode<T>> get_bvh() const = 0;

  const BVHNode<T> &bvh() const {
    if (m_needs_update) {
      m_bvh_root = get_bvh();
      m_needs_update = false;
    }
    return *m_bvh_root;
  }

  void clear_bounding_volumes() { m_needs_update = true; }

  std::optional<glm::vec3>
  collision_vector(const CollisionObject<T> &other) const {
    // return a collision vector in which direction and intensity to move this
    // object to resolve a collision
    const auto &this_bvh = bvh();
    const auto &other_bvh = other.bvh();

    assert((this_bvh.children.empty() || other_bvh.children.empty()) &&
           "at least one bvh is a volume itself");

    return this_bvh.children.empty()
               ? collision_vector(other_bvh, this_bvh.volume)
               : collision_vector(this_bvh, other_bvh.volume);
  }

private:
  std::optional<glm::vec3> collision_vector(const BVHNode<T> &node,
                                            const T &volume) const {
    // order is important when doing intersects because of vector direction!
    auto result_vector = volume.intersects(node.volume);
    if (result_vector == glm::vec3(0, 0, 0) || node.children.empty()) {
      // there is no intersection - prune, don't go further
      return result_vector;
    }

    auto update_result = [&](const glm::vec3 &other) {
      for (int i = 0; i < 3; ++i) {
        if (result_vector[i] > 0 && other[i] < 0 ||
            result_vector[i] < 0 && other[i] > 0) {
          // there is no such vector
          return false;
        } else {
          if (result_vector[i] == 0) {
            result_vector[i] = other[i];
          } else if (result_vector[i] > 0) {
            result_vector[i] = std::max(result_vector[i], other[i]);
          } else {
            result_vector[i] = std::min(result_vector[i], other[i]);
          }
        }
      }

      return true;
    };

    // since there are children don't use this result vector in result
    result_vector = glm::vec3(0, 0, 0);

    for (const auto &child : node.children) {
      auto child_vector = collision_vector(*child, volume);
      if (!child_vector || !update_result(*child_vector)) {
        // unsolvable collision
        return std::nullopt;
      }
    }

    return result_vector;
  }

private:
  mutable std::unique_ptr<BVHNode<T>> m_bvh_root;
  mutable bool m_needs_update = true;
};

#endif /* _COLLISION_OBJECT_H_ */
