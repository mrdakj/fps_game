#include "bounding_box.h"
#include "aabb.h"
#include "camera.h"
#include "utility.h"
#include <array>
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <initializer_list>
#include <iostream>
#include <vector>

#define INVALID INT_MAX
#define FLOAT_MIN (-std::numeric_limits<float>::infinity())
#define FLOAT_MAX (std::numeric_limits<float>::infinity())
#define EPS 0.0001

BoundingBox::BoundingBox(const AABB &aabb)
    : BoundingBox({glm::vec3(aabb.max_x - aabb.min_x, 0, 0),
                   glm::vec3(0, aabb.max_y - aabb.min_y, 0),
                   glm::vec3(0, 0, aabb.min_z - aabb.max_z)},
                  glm::vec3(aabb.min_x, aabb.min_y, aabb.max_z)) {}

BoundingBox::BoundingBox(std::array<glm::vec3, 3> axes, glm::vec3 origin)
    : m_axes(std::move(axes)), m_origin(std::move(origin)) {}

BoundingBox BoundingBox::bounding_aabb(const std::vector<BoundingBox> &boxes) {
  AABB aabb;
  for (const auto &box : boxes) {
    aabb.update(box.aabb());
  }

  return aabb;
}

void BoundingBox::render(Shader &shader, const Camera &camera,
                         const glm::vec3 &color) const {
  std::vector<unsigned int> indices = {// bottom square
                                       0, 1, 1, 2, 2, 3, 3, 0,
                                       // top square
                                       4, 5, 5, 6, 6, 7, 7, 4,
                                       // in between
                                       0, 4, 1, 5, 2, 6, 3, 7};

  std::array<glm::vec3, 8> positions = {m_origin,
                                        m_origin + m_axes[0],
                                        m_origin + m_axes[0] + m_axes[1],
                                        m_origin + m_axes[1],
                                        m_origin + m_axes[2],
                                        m_origin + m_axes[2] + m_axes[0],
                                        m_origin + m_axes[2] + m_axes[0] +
                                            m_axes[1],
                                        m_origin + m_axes[2] + m_axes[1]};

  GLuint vao, vbo, ebo;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 8, positions.data(),
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                        (const GLvoid *)0);

  glEnableVertexAttribArray(0);

  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(),
               indices.data(), GL_STATIC_DRAW);

  shader.activate();

  glUniform3f(glGetUniformLocation(shader.id(), "Color"), color[0], color[1],
              color[2]);

  shader.set_uniform("camMatrix", camera.matrix());
  glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);

  // unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

BoundingBox BoundingBox::transform(const glm::mat4 &transformation) const {
  glm::vec3 transformed_origin = transformation * glm::vec4(m_origin, 1);

  std::array<glm::vec3, 3> transformed_axes;
  transformed_axes[0] =
      glm::vec3(transformation * glm::vec4(m_origin + m_axes[0], 1.0f)) -
      transformed_origin;
  transformed_axes[1] =
      glm::vec3(transformation * glm::vec4(m_origin + m_axes[1], 1.0f)) -
      transformed_origin;
  transformed_axes[2] =
      glm::vec3(transformation * glm::vec4(m_origin + m_axes[2], 1.0f)) -
      transformed_origin;

  return BoundingBox(std::move(transformed_axes),
                     std::move(transformed_origin));
}

std::array<glm::vec3, 3> BoundingBox::get_axes() const {
  auto zero = glm::vec3(0, 0, 0);

  // check if box is a rectangle and manually generate the third axis
  if (m_axes[0] == zero) {
    assert(m_axes[1] != zero && m_axes[2] != zero &&
           "at most one axes is zero vector");
    return {glm::normalize(glm::cross(m_axes[1], m_axes[2])),
            glm::normalize(m_axes[1]), glm::normalize(m_axes[2])};
  } else if (m_axes[1] == zero) {
    assert(m_axes[0] != zero && m_axes[2] != zero &&
           "at most one axes is zero vector");
    return {glm::normalize(m_axes[0]),
            glm::normalize(glm::cross(m_axes[0], m_axes[2])),
            glm::normalize(m_axes[2])};
  } else if (m_axes[2] == zero) {
    assert(m_axes[0] != zero && m_axes[1] != zero &&
           "at most one axes is zero vector");
    return {glm::normalize(m_axes[0]), glm::normalize(m_axes[1]),
            glm::normalize(glm::cross(m_axes[0], m_axes[1]))};
  }

  return {glm::normalize(m_axes[0]), glm::normalize(m_axes[1]),
          glm::normalize(m_axes[2])};
}

std::pair<float, float> BoundingBox::project(const glm::vec3 &v) const {
  // project all positions to normalized vector v and return min and max
  // projected value
  float min_projected = FLOAT_MAX;
  float max_projected = FLOAT_MIN;

  glm::vec3 positions[8] = {m_origin,
                            m_origin + m_axes[0],
                            m_origin + m_axes[0] + m_axes[1],
                            m_origin + m_axes[1],
                            m_origin + m_axes[2],
                            m_origin + m_axes[2] + m_axes[0],
                            m_origin + m_axes[2] + m_axes[0] + m_axes[1],
                            m_origin + m_axes[2] + m_axes[1]};

  for (const auto &position : positions) {
    // suppose v is normalized
    float projected_value = glm::dot(position, v);
    min_projected = std::min(min_projected, projected_value);
    max_projected = std::max(max_projected, projected_value);
  }

  return {min_projected, max_projected};
}

AABB BoundingBox::aabb() const {
  std::initializer_list<glm::vec3> positions = {
      m_origin,
      m_origin + m_axes[0],
      m_origin + m_axes[0] + m_axes[1],
      m_origin + m_axes[1],
      m_origin + m_axes[2],
      m_origin + m_axes[2] + m_axes[0],
      m_origin + m_axes[2] + m_axes[0] + m_axes[1],
      m_origin + m_axes[2] + m_axes[1]};

  float min_x = std::min(
      positions, [](const auto &a, const auto &b) { return a[0] < b[0]; })[0];
  float max_x = std::max(
      positions, [](const auto &a, const auto &b) { return a[0] < b[0]; })[0];

  float min_y = std::min(
      positions, [](const auto &a, const auto &b) { return a[1] < b[1]; })[1];
  float max_y = std::max(
      positions, [](const auto &a, const auto &b) { return a[1] < b[1]; })[1];

  float min_z = std::min(
      positions, [](const auto &a, const auto &b) { return a[2] < b[2]; })[2];
  float max_z = std::max(
      positions, [](const auto &a, const auto &b) { return a[2] < b[2]; })[2];

  return {min_x, max_x, min_y, max_y, min_z, max_z};
}

bool BoundingBox::is_aabb() const {
  return // x axis is parallel to (1,0,0)
      m_axes[0][1] == 0.0f && m_axes[0][2] == 0.0f &&
      // y axis is parallel to (0,1,0)
      m_axes[1][0] == 0.0f && m_axes[1][2] == 0.0f &&
      // z axis is parallel to (0,0,1)
      m_axes[2][0] == 0.0f && m_axes[2][1] == 0.0f;
}

bool BoundingBox::intersects(const glm::vec3 &A, const glm::vec3 &B) const {

  auto this_axes = get_axes();

  for (const auto &this_ax : this_axes) {
    auto [this_min, this_max] = project(this_ax);

    float A_projected = glm::dot(A, this_ax);
    float B_projected = glm::dot(B, this_ax);
    float other_min = std::min(A_projected, B_projected);
    float other_max = std::max(A_projected, B_projected);

    if (this_max < other_min || other_max < this_min) {
      // no collision
      return false;
    }
  }

  auto other_ax = B - A;
  auto [this_min, this_max] = project(other_ax);

  float A_projected = glm::dot(A, other_ax);
  float B_projected = glm::dot(B, other_ax);
  float other_min = std::min(A_projected, B_projected);
  float other_max = std::max(A_projected, B_projected);

  if (this_max < other_min || other_max < this_min) {
    // no collision
    return false;
  }

  for (const auto &this_ax : this_axes) {
    float dot_product = fabs(glm::dot(this_ax, other_ax));
    if (dot_product > 1 - EPS && dot_product < 1 + EPS) {
      // perpendicular axes
      continue;
    }
    auto c_ax = glm::cross(this_ax, other_ax);
    assert((glm::isnan(c_ax) == glm::vec<3, bool>(false, false, false)) &&
           "cross product valid");

    auto [this_min, this_max] = project(c_ax);

    float A_projected = glm::dot(A, c_ax);
    float B_projected = glm::dot(B, c_ax);
    float other_min = std::min(A_projected, B_projected);
    float other_max = std::max(A_projected, B_projected);

    if (this_max < other_min || other_max < this_min) {
      // no collision
      return false;
    }
  }

  return true;
}

glm::vec3 BoundingBox::intersects(const BoundingBox &other) const {
  // return vector into which direction and length we need to move to resolve a
  // collision
  auto this_aabb = aabb();
  auto other_aabb = other.aabb();

  auto zero_vector = glm::vec3(0, 0, 0);
  float min_overlap = FLOAT_MAX;
  auto min_axis = zero_vector;

  if (!this_aabb.intersects(other_aabb)) {
    // no intersection
    return zero_vector;
  }

  // check for collision using Separating Axis Theorem
  // we need to check at most 15 axes - 3 face normals for box a (other 3
  // normals are mirrored vectors), 3 face normals for box b, and 9 normals that
  // are cross product between 3 face normals for box a and 3 face normals for
  // box b
  // axes doesn't need to be normalized even when doing projection because it
  // doesn't change the order

  auto update_result_with_overlap = [&](const glm::vec3 &axis) {
    auto [this_min, this_max] = project(axis);
    auto [other_min, other_max] = other.project(axis);

    if (this_max < other_min || other_max < this_min) {
      // there is a separation plane, hence no collision
      return false;
    } else {
      if (this_max - other_min < min_overlap) {
        min_overlap = this_max - other_min;
        min_axis = -axis;
      }
      if (other_max - this_min < min_overlap) {
        min_overlap = other_max - this_min;
        min_axis = axis;
      }
    }

    return true;
  };

  auto this_axes = get_axes();

  for (const auto &this_ax : this_axes) {
    if (!update_result_with_overlap(this_ax)) {
      // no collision
      return zero_vector;
    }
  }

  auto other_axes = other.get_axes();

  for (const auto &other_ax : other_axes) {
    if (!update_result_with_overlap(other_ax)) {
      // no collision
      return zero_vector;
    }
  }

  if (is_aabb() && other.is_aabb()) {
    return min_overlap * min_axis;
  }

  for (const auto &this_ax : this_axes) {
    for (const auto &other_ax : other_axes) {
      float dot_product = fabs(glm::dot(this_ax, other_ax));
      if (dot_product > 1 - EPS && dot_product < 1 + EPS) {
        // perpendicular axes
        continue;
      }
      auto c_ax = glm::normalize(glm::cross(this_ax, other_ax));
      assert((glm::isnan(c_ax) == glm::vec<3, bool>(false, false, false)) &&
             "cross product valid");
      if (!update_result_with_overlap(c_ax)) {
        // no collision
        return zero_vector;
      }
    }
  }

  // cannot find a separation plane, hence collision exists
  return min_overlap * min_axis;
}
