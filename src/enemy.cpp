#include "enemy.h"
#include "aabb.h"
#include "animated_mesh.h"
#include "bounding_box.h"
#include "camera.h"
#include "player.h"
#include <functional>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

#include <iostream>

const std::string IDLE_POSITION = "idle";
const std::string LEFT_EYE_BONE = "swat:LeftEye_010";
const std::string SPINE_BONE = "swat:Spine_02";
const std::string GUN = "gun";

Enemy::Enemy()
    : AnimatedMesh("../res/models/enemy/enemy.gltf"),
      m_action_to_animation(
          {{Action::RotateLeft, {"rotate", true, 2.0f, true}},
           {Action::RotateRight, {"rotate", false, 2.0f, true}},
           {Action::FallDead, {"fall_dead", false}},
           {Action::GunUp, {"shoot", false, 2.0f}},
           {Action::GunDown, {"shoot", true, 2.0f}}}) {
  auto scaling = glm::scale(glm::mat4(1.0f), glm::vec3(0.01, 0.01, 0.01));
  auto rotation = glm::mat4(1.0f);
  auto translation = glm::translate(glm::mat4(1.0), glm::vec3(2, 0.1, -2));
  AnimatedMesh::set_user_transformation(translation * rotation * scaling);

  set_idle();
  AnimatedMesh::merge_user_and_global_transformations();
}

void Enemy::set_idle() {
  auto global_transformation =
      m_skinned_mesh.get_bones_for_position(IDLE_POSITION);
  AnimatedMesh::set_global_transformation(std::move(global_transformation));
}

void Enemy::set_shot() { m_is_shot = true; }

bool Enemy::is_shot() const { return m_is_shot; }

std::pair<glm::vec3, glm::vec3> Enemy::get_gun_direction() const {
  const auto &gun_node_global_transformation =
      m_skinned_mesh.node_global_transformation(GUN);
  glm::vec3 gun_O = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(0, 0, 0, 1);
  glm::vec3 gun_X = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(1, 0, 0, 1);
  return {gun_O, gun_X - gun_O};
}

std::pair<glm::vec3, glm::vec3> Enemy::get_eye_direction() const {
  const auto &left_eye_node_global_transformation =
      m_skinned_mesh.node_global_transformation("swat:LeftEye_010");
  glm::vec3 eye_O = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(0, 0, 0, 1);
  glm::vec3 eye_Z = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(0, 0, 1, 1);
  return {eye_O, -(eye_Z - eye_O)};
}

void Enemy::render(Shader &shader, Shader &bounding_box_shader,
                   const Camera &camera, const Light &light) const {
  AnimatedMesh::render(shader, bounding_box_shader, camera, light);

  // for testing to visualize a gun direction
  // const auto &gun_node_global_transformation =
  //     m_skinned_mesh.node_global_transformation(GUN);

  // glm::vec3 gun_O = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(0, 0, 0, 1);

  // glm::vec3 gun_X = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(1, 0, 0, 1);

  // glm::vec3 gun_Y = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(0, 1, 0, 1);
  // glm::vec3 gun_Z = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(0, 0, 1, 1);

  // BoundingBox(
  //     {100.0f * (get_gun_direction()), (gun_Y - gun_O), (gun_Z - gun_O)},
  //     gun_O) .render(bounding_box_shader, camera, glm::vec3(1.0, 0.0, 0.0));

  // const auto &left_eye_node_global_transformation =
  //     m_skinned_mesh.node_global_transformation(LEFT_EYE_BONE);
  // glm::vec3 eye_O = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(0, 0, 0,
  //                   1);
  // glm::vec3 eye_X = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(1, 0, 0,
  //                   1);
  // glm::vec3 eye_Y = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(0, 1, 0,
  //                   1);
  // glm::vec3 eye_Z = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(0, 0, 1,
  //                   1);
  // BoundingBox(
  //     {(eye_X-eye_O), (eye_Y - eye_O), -100.0f * (eye_Z - eye_O)}, eye_O)
  //     .render(bounding_box_shader, camera, glm::vec3(0.0, 1.0, 0.0));
}

void Enemy::update(float current_time) {
  auto animation_tick =
      [&](std::pair<const Action, ActionStatus> &action_status,
          std::function<void()> on_animation_success = {}) {
        if (m_action_to_animation[action_status.first].update(*this,
                                                              current_time)) {
          action_status.second = ActionStatus::Success;
          if (on_animation_success) {
            on_animation_success();
          }
        }
      };

  for (auto &action : m_todo_actions) {
    assert(action.first != Action::None && "valid todo action");

    if (action.second == ActionStatus::Success) {
      continue;
    }

    assert(m_enemy_state == EnemyState::Alive &&
           "cannot have running animation on dead enemy");

    if (action.first == Action::FallDead) {
      assert(m_todo_actions.size() == 1 &&
             "there is only one action when falling dead");
      animation_tick(action, [&]() { m_enemy_state = EnemyState::Dead; });
    } else if (action.first == Action::RotateLeft ||
               action.first == Action::RotateRight) {
      animation_tick(action);
    } else if (action.first == Action::GunUp) {
      assert(m_gun_state == GunState::Down && "gun is down");
      animation_tick(action, [&]() { m_gun_state = GunState::Up; });
    } else if (action.first == Action::GunDown) {
      assert(m_gun_state == GunState::Up && "gun is up");
      animation_tick(action, [&]() { m_gun_state = GunState::Down; });
    }
  }

  if (m_last_update_time == -1) {
    m_last_update_time = current_time;
  }
  float delta = current_time - m_last_update_time;
  m_last_update_time = current_time;

  assert(!is_rotatng_spine_left || !is_rotatng_spine_right);
  if (is_rotatng_spine_left) {
    rotate(true, delta);
  } else if (is_rotatng_spine_right) {
    rotate(false, delta);
  }
}

bool Enemy::rotate(bool left, float delta) {
  // TODO: fix magic numbers
  // rotate around spine's y axis which is up axis
  float delta_angle = left ? 100 * delta : -100 * delta;
  if (left) {
    if (m_spine_angle + delta_angle > 40.0) {
      delta_angle = 40.0 - m_spine_angle;
    }
  } else {
    if (m_spine_angle + delta_angle < -60.0) {
      delta_angle = -60 - m_spine_angle;
    }
  }
  m_spine_angle += delta_angle;
  if (delta_angle != 0) {
    m_skinned_mesh.set_node_transformation(
        SPINE_BONE, glm::rotate(glm::radians(delta_angle), glm::vec3(0, 1, 0)));
  }

  return delta_angle != 0;
}

bool Enemy::can_rotate_spine(bool left) const {
  return left && m_spine_angle < 40 || !left && m_spine_angle > -60;
}

std::optional<Enemy::ActionStatus>
Enemy::get_action_status_and_remove_successful(Action action) {
  auto todo_action_it = m_todo_actions.find(action);
  if (todo_action_it != m_todo_actions.end()) {
    if (todo_action_it->second == ActionStatus::Success) {
      m_todo_actions.erase(todo_action_it);
      return ActionStatus::Success;
    } else {
      return ActionStatus::Running;
    }
  }

  return std::nullopt;
}

void Enemy::register_new_todo_action(Action action) {
  m_todo_actions.emplace(action, ActionStatus::Running);
}

void Enemy::remove_todo_action(Action action) { m_todo_actions.erase(action); }

void Enemy::clear_todo_actions() { m_todo_actions.clear(); }
