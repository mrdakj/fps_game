#ifndef _COLLISION_OBJECT_H_
#define _COLLISION_OBJECT_H_

#include "aabb.h"
#include "bounding_box.h"
#include <iostream>
#include <optional>
#include <vector>

template <typename T> struct BoundingVolumeHierarchy {};

template <> struct BoundingVolumeHierarchy<BoundingBox> {
  BoundingVolumeHierarchy() = default;
  BoundingVolumeHierarchy(std::vector<BoundingBox> children,
                          bool always_check_children = true)
      : m_parent(BoundingBox::bounding_aabb(children)),
        m_children(std::move(children)),
        m_always_check_children(always_check_children) {}

  bool collision_exists(const BoundingVolumeHierarchy<BoundingBox> &other) const {
    auto zero_vector = glm::vec3(0, 0, 0);
    auto parent_intersects = m_parent.intersects(other.m_parent);
    if (parent_intersects == zero_vector) {
      // no collision
      return false;
    }

    if (!m_always_check_children && !other.m_always_check_children) {
      return true;
    }

    if (m_always_check_children && !other.m_always_check_children) {
      for (const auto &child : m_children) {
        auto current_vector = child.intersects(other.m_parent);
        if (current_vector != zero_vector) {
            return true;
        }
      }

      return false;
    }

    if (!m_always_check_children && other.m_always_check_children) {
      for (const auto &other_child : other.m_children) {
        auto current_vector = m_parent.intersects(other_child);
        if (current_vector != zero_vector) {
            return true;
        }
      }

      return false;
    }

    if (other.m_children.size() < m_children.size()) {
      for (const auto &other_child : other.m_children) {
        if (m_parent.intersects(other_child) == zero_vector) {
          continue;
        }
        for (const auto &child : m_children) {
          auto current_vector = child.intersects(other_child);
          if (current_vector != zero_vector) {
              return true;
          }
        }
      }

      return false;
    }

    for (const auto &child : m_children) {
      if (child.intersects(other.m_parent) == zero_vector) {
        continue;
      }
      for (const auto &other_child : other.m_children) {
        auto current_vector = child.intersects(other_child);
        if (current_vector != zero_vector) {
            return true;
        }
      }
    }

    return false;
  }

  std::optional<glm::vec3>
  collision_vector(const BoundingVolumeHierarchy<BoundingBox> &other) const {
    auto zero_vector = glm::vec3(0, 0, 0);
    auto parent_intersects = m_parent.intersects(other.m_parent);
    if (parent_intersects == zero_vector) {
      // no collision
      return zero_vector;
    }

    if (!m_always_check_children && !other.m_always_check_children) {
      return parent_intersects;
    }

    auto result_vector = glm::vec3(0, 0, 0);

    auto update_result = [&](const glm::vec3 other) {
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

    if (m_always_check_children && !other.m_always_check_children) {
      for (const auto &child : m_children) {
        auto current_vector = child.intersects(other.m_parent);
        if (current_vector != zero_vector) {
          if (!update_result(current_vector)) {
            return std::nullopt;
          }
        }
      }

      return result_vector;
    }

    if (!m_always_check_children && other.m_always_check_children) {
      for (const auto &other_child : other.m_children) {
        auto current_vector = m_parent.intersects(other_child);
        if (current_vector != zero_vector) {
          if (!update_result(current_vector)) {
            return std::nullopt;
          }
        }
      }

      return result_vector;
    }

    if (other.m_children.size() < m_children.size()) {
      for (const auto &other_child : other.m_children) {
        if (m_parent.intersects(other_child) == zero_vector) {
          continue;
        }
        for (const auto &child : m_children) {
          auto current_vector = child.intersects(other_child);
          if (current_vector != zero_vector) {
            if (!update_result(current_vector)) {
              return std::nullopt;
            }
          }
        }
      }

      return result_vector;
    }

    for (const auto &child : m_children) {
      if (child.intersects(other.m_parent) == zero_vector) {
        continue;
      }
      for (const auto &other_child : other.m_children) {
        auto current_vector = child.intersects(other_child);
        if (current_vector != zero_vector) {
          if (!update_result(current_vector)) {
            return std::nullopt;
          }
        }
      }
    }

    return result_vector;
  }

public:
  BoundingBox m_parent;
  std::vector<BoundingBox> m_children;
  bool m_always_check_children;
};

template <typename T> class CollisionObject {
public:
  const BoundingVolumeHierarchy<T> &bounding_volumes() const {
    if (m_needs_update) {
      m_bounding_volumes = get_bounding_volumes();
      m_needs_update = false;
    }
    return m_bounding_volumes;
  }

  void clear_bounding_volumes() { m_needs_update = true; }

  virtual BoundingVolumeHierarchy<T> get_bounding_volumes() const = 0;

  bool collision_exists(const CollisionObject<T> &other) const {
    return bounding_volumes().collision_exists(other.bounding_volumes());
  }

  std::optional<glm::vec3> collision_vector(const CollisionObject<T> &other) const {
    return bounding_volumes().collision_vector(other.bounding_volumes());
  }

  virtual ~CollisionObject() {}

private:
  mutable BoundingVolumeHierarchy<T> m_bounding_volumes;
  mutable bool m_needs_update = true;
};

#endif /* _COLLISION_OBJECT_H_ */
