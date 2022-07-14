#include "collision_detector.h"
#include "aabb.h"
#include "bounding_box.h"
#include "player.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <optional>
#include <vector>

const float eps = 0.001;

std::optional<glm::vec3> CollistionDetector::collision_vector(
    const CollisionObject<BoundingBox> &object) const {
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

  auto maybe_current_vector = object.collision_vector(*m_static_object);
  if (!maybe_current_vector || !update_result(*maybe_current_vector)) {
    return std::nullopt;
  }

  for (const auto &dynamic_object : m_dynamic_objects) {
    if (&object == dynamic_object) {
      // don't check intersection with itself
      continue;
    }

    auto maybe_current_vector = object.collision_vector(*dynamic_object);
    if (!maybe_current_vector || !update_result(*maybe_current_vector)) {
      return std::nullopt;
    }
  }

  return result_vector;
}
