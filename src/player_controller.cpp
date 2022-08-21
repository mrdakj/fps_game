#include "player_controller.h"
#include "animation_controller.h"

#include <cmath>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/trigonometric.hpp>
#include <math.h>

#define EPS 0.0001

void PlayerController::update(float current_time) {
  process_inputs();
  animation_update(m_timer.tick(current_time));
}

void PlayerController::process_inputs() {
  process_inputs_keyboard();
  process_inputs_mouse();
}

void PlayerController::process_inputs_keyboard() {
  process_keyboard_for_animation();
  process_keyboard_for_move();
}

void PlayerController::process_inputs_mouse() { process_mouse_for_rotation(); }

void PlayerController::process_keyboard_for_move() const {
  bool key_pressed = false;

  glm::vec3 direction_w(0, 0, 0);
  glm::vec3 direction_a(0, 0, 0);
  glm::vec3 direction_s(0, 0, 0);
  glm::vec3 direction_d(0, 0, 0);

  if (is_key_pressed(GLFW_KEY_W)) {
    key_pressed = true;
    direction_w = m_player.camera().orientation();
    // don't change the height of the camera
    direction_w.y = 0;
    direction_w = glm::normalize(direction_w);
  }

  if (is_key_pressed(GLFW_KEY_A)) {
    key_pressed = true;
    direction_a =
        -glm::cross(m_player.camera().orientation(), m_player.camera().up());
    // don't change the height of the camera
    direction_a.y = 0;
    direction_a = glm::normalize(direction_a);
  }

  if (is_key_pressed(GLFW_KEY_S)) {
    key_pressed = true;
    direction_s = -m_player.camera().orientation();
    // don't change the height of the camera
    direction_s.y = 0;
    direction_s = glm::normalize(direction_s);
  }

  if (is_key_pressed(GLFW_KEY_D)) {
    key_pressed = true;
    direction_d =
        glm::cross(m_player.camera().orientation(), m_player.camera().up());
    direction_d.y = 0;
    direction_d = glm::normalize(direction_d);
  }

  if (!key_pressed) {
    return;
  }

  auto resolve_collision = [&]() {
    auto collision_vector = get_collision_vector();
    if (!collision_vector) {
      // collision cannot be solved
      return false;
    } else if (collision_vector == glm::vec3(0, 0, 0)) {
      // no collision
      return true;
    } else {
      // collision exists and maybe can be solved
      auto direction =
          glm::vec3((*collision_vector)[0], 0, (*collision_vector)[2]);
      float collision_vector_length_sq = glm::length2(*collision_vector);
      float direction_length_sq = glm::length2(direction);

      if (direction != glm::vec3(0, 0, 0)) {
        // how much to move in direction in order to pass collision_vector when
        // projected - distance*direction is hypotenuse and collision_vector is
        // cathetus
        float distance = collision_vector_length_sq / direction_length_sq;
        if (distance * distance * direction_length_sq > 0.5f) {
          // to big distance
          std::cout << "too big distance" << std::endl;
          return false;
        } else {
          m_player.update_position(distance * direction);

          auto new_collision_vector = get_collision_vector();
          if (!new_collision_vector || fabs((*new_collision_vector)[0]) > EPS ||
              fabs((*new_collision_vector)[1]) > EPS ||
              fabs((*new_collision_vector)[2]) > EPS) {
            // collision cannot be solved because of new collision
            std::cout << "new collision" << std::endl;
            return false;
          }

          // successfully resolved collision
          return true;
        }
      } else {
        // perpendicular collision vector
        std::cout << "prependicular" << std::endl;
        return false;
      }
    }

    return false;
  };

  auto saved_position = m_player.camera().position();

  // handle the keyboard x direction
  m_player.update_position(
      m_player.camera().speed() *
      glm::vec3(direction_w.x + direction_a.x + direction_s.x + direction_d.x,
                0, 0));

  if (!resolve_collision()) {
    m_player.set_position(saved_position);
  }

  saved_position = m_player.camera().position();

  // handle the keyboard z direction
  m_player.update_position(
      m_player.camera().speed() *
      glm::vec3(0, 0,
                direction_w.z + direction_a.z + direction_s.z + direction_d.z));

  if (!resolve_collision()) {
    m_player.set_position(saved_position);
  }
}

void PlayerController::process_mouse_for_rotation() const {
  // handle the mouse
  auto [mouse_x, mouse_y] = get_mouse_position();

  float rotX = m_player.camera().sensitivity() *
               (float)(mouse_y - (m_player.camera().height() / 2)) /
               m_player.camera().height();
  float rotY = m_player.camera().sensitivity() *
               (float)(mouse_x - (m_player.camera().width() / 2)) /
               m_player.camera().width();

  // restring horizontal rotation to avoid moving outside the wall wehn
  // resolving collision
  rotY = std::min(std::max(-20.0f, rotY), 20.0f);

  auto resolve_collision = [&]() {
    auto collision_vector = get_collision_vector();
    if (!collision_vector) {
      // cannot resolve collision
      std::cout << "reset orientation" << std::endl;
      return false;
    } else if (collision_vector == glm::vec3(0, 0, 0)) {
      // no collision
      return true;
    } else {
      // collision exists and maybe can be solved
      auto direction =
          glm::vec3((*collision_vector)[0], 0, (*collision_vector)[2]);
      float collision_vector_length_sq = glm::length2(*collision_vector);
      float direction_length_sq = glm::length2(direction);

      if (direction != glm::vec3(0, 0, 0)) {
        // how much to move in direction in order to pass collision_vector when
        // projected - distance*direction is hypotenuse and collision_vector is
        // cathetus
        float distance = collision_vector_length_sq / direction_length_sq;
        if (distance * distance * direction_length_sq > 0.5f) {
          // cannot resolve collision
          std::cout << "too big distance" << std::endl;
          return false;
        } else {
          m_player.update_position(distance * direction);

          auto new_collision_vector = get_collision_vector();
          if (!new_collision_vector || fabs((*new_collision_vector)[0]) > EPS ||
              fabs((*new_collision_vector)[1]) > EPS ||
              fabs((*new_collision_vector)[2]) > EPS) {
            // collision cannot be solved because of new collision
            std::cout << "new collision" << std::endl;
            return false;
          }

          // collision successfully resolved
          return true;
        }
      } else {
        // cannot resolve collision
        std::cout << "prependicular" << std::endl;
        return false;
      }
    }

    return false;
  };

  auto new_orientation = glm::rotate(
      m_player.camera().orientation(), glm::radians(-rotX),
      glm::cross(m_player.camera().orientation(), m_player.camera().up()));

  if (!(glm::angle(new_orientation, m_player.camera().up()) <=
            glm::radians(5.0f) ||
        glm::angle(new_orientation, -m_player.camera().up()) <=
            glm::radians(5.0f))) {

    auto saved_orientation = m_player.camera().orientation();
    auto saved_position = m_player.camera().position();
    m_player.set_orientation(new_orientation);
    if (!resolve_collision()) {
      m_player.set_orientation(saved_orientation);
      m_player.set_position(saved_position);
    }
  }

  auto saved_orientation = m_player.camera().orientation();
  auto saved_position = m_player.camera().position();
  m_player.set_orientation(glm::rotate(m_player.camera().orientation(),
                                       glm::radians(-rotY),
                                       m_player.camera().up()));
  if (!resolve_collision()) {
    m_player.set_orientation(saved_orientation);
    m_player.set_position(saved_position);
  }

  set_mouse_position(m_player.camera().width() / 2,
                     m_player.camera().height() / 2);
}

void PlayerController::process_keyboard_for_animation() {
  if (is_key_pressed(GLFW_KEY_P)) {
    if (m_todo_action == Player::Action::None) {
      m_todo_action = Player::Action::Recharge;
    }
  } else if (is_mouse_button_pressed(MouseButton::Left)) {
    if (m_todo_action == Player::Action::None) {
      m_todo_action = Player::Action::Shoot;
    }
  }
}

void PlayerController::animation_update(float delta_time) {
  if (m_todo_action != Player::Action::None) {
    auto animation_it = m_action_to_animation.find(m_todo_action);
    assert(animation_it != m_action_to_animation.end() && "animation found");
    if (animation_it->second.update(delta_time).first) {
      m_todo_action = Player::Action::None;
    }
  }
}
