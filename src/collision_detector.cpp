#include "collision_detector.h"
#include "aabb.h"
#include "bounding_box.h"
#include "player.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <optional>
#include <vector>

#define EPS 0.0001

bool update_result(glm::vec3 &result_vector, const glm::vec3 &other) {
  for (int i = 0; i < 3; ++i) {
    if (fabs(other[i]) < EPS) {
      continue;
    }

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
}

std::optional<glm::vec3> CollistionDetector::collision_vector(
    const CollisionObject<BoundingBox> *dynamic_object) const {
  auto result_vector = glm::vec3(0, 0, 0);

  // collision with static objects
  for (auto static_object_ptr : m_static_objects) {
    auto current_vector = collision_vector(dynamic_object->bvh().volume,
                                           static_object_ptr->bvh());

    if (!current_vector || !update_result(result_vector, *current_vector)) {
      // unsolvable collision
      return std::nullopt;
    }
  }

  // collision with other dynamic objects
  bool ok = false;
  for (auto dynamic_object_ptr : m_dynamic_objects) {
    if (dynamic_object_ptr == dynamic_object) {
      ok = true;
      continue;
    }

    auto current_vector = dynamic_object->bvh().volume.intersects(
        dynamic_object_ptr->bvh().volume);

    if (!update_result(result_vector, current_vector)) {
      // unsolvable collision
      return std::nullopt;
    }
  }

  assert(ok);

  return result_vector;
}

std::optional<glm::vec3>
CollistionDetector::collision_vector(const BoundingBox &box,
                                     const BVHNode<BoundingBox> &node) const {
  // order is important when doing intersects because of vector direction!
  auto result_vector = box.intersects(node.volume);
  if (result_vector == glm::vec3(0, 0, 0) || node.children.empty()) {
    // there is no intersection - prune, don't go further
    return result_vector;
  }

  // since there are children don't use this result vector in result
  result_vector = glm::vec3(0, 0, 0);

  for (const auto &child : node.children) {
    auto child_vector = collision_vector(box, *child);
    if (!child_vector || !update_result(result_vector, *child_vector)) {
      // unsolvable collision
      return std::nullopt;
    }
  }

  return result_vector;
}
