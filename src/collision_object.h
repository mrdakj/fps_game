#ifndef _COLLISION_OBJECT_H_
#define _COLLISION_OBJECT_H_

#include "aabb.h"
#include "bounding_box.h"
#include "skinned_mesh.h"
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

template <typename T> class CollisionObject {
public:
  CollisionObject() = default;

  CollisionObject(const CollisionObject &other)
      : m_bvh_root(nullptr), m_needs_update(true) {}

  virtual ~CollisionObject() {}

  virtual std::unique_ptr<BVHNode<T>> get_bvh() const = 0;

  virtual const BVHNode<T> &bvh() const {
    if (m_needs_update) {
      m_bvh_root = get_bvh();
      assert(m_bvh_root && "bvh valid");
      m_needs_update = false;
    }
    return *m_bvh_root;
  }

  void clear_bounding_volumes() { m_needs_update = true; }

private:
  mutable std::unique_ptr<BVHNode<T>> m_bvh_root;
  mutable bool m_needs_update = true;
};

#endif /* _COLLISION_OBJECT_H_ */
