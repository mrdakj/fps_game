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
  process_inputs(current_time);
  animation_update(current_time);
}

void PlayerController::process_inputs(float current_time) {
  process_inputs_keyboard(current_time);
  process_inputs_mouse(current_time);
}

void PlayerController::process_inputs_keyboard(float current_time) {
  process_keyboard_for_animation(current_time);
  process_keyboard_for_move();
}

void PlayerController::process_inputs_mouse(float current_time) {
  process_mouse_for_rotation();
}

void PlayerController::process_keyboard_for_move() const {
  auto saved_position = m_player.camera().position();

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
      // reset position because collision cannot be solved
      m_player.set_position(saved_position);
    } else if (collision_vector == glm::vec3(0, 0, 0)) {
      // no collision save current orientation
      saved_position = m_player.camera().position();
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
          // reset position
          std::cout << "too big distance" << std::endl;
          m_player.set_position(saved_position);
        } else {
          auto saved_position = m_player.camera().position();
          m_player.update_position(distance * direction);

          auto new_collision_vector = get_collision_vector();
          if (!new_collision_vector || fabs((*new_collision_vector)[0]) > EPS ||
              fabs((*new_collision_vector)[1]) > EPS ||
              fabs((*new_collision_vector)[2]) > EPS) {
            // collision cannot be solved, reset both position and orientation
            std::cout << "new collision" << std::endl;
            m_player.set_position(saved_position);
          }
        }
      } else {
        // reset position
        std::cout << "prependicular" << std::endl;
        m_player.set_position(saved_position);
      }
    }
  };

  // handle the keyboard x direction
  m_player.update_position(
      m_player.camera().speed() *
      glm::vec3(direction_w.x + direction_a.x + direction_s.x + direction_d.x,
                0, 0));

  resolve_collision();

  // handle the keyboard z direction
  m_player.update_position(
      m_player.camera().speed() *
      glm::vec3(0, 0,
                direction_w.z + direction_a.z + direction_s.z + direction_d.z));

  resolve_collision();
}

void PlayerController::process_mouse_for_rotation() const {
  // handle the mouse
  auto saved_orientation = m_player.camera().orientation();
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
      // reset orientation because collision cannot be solved
      m_player.set_orientation(saved_orientation);
    } else if (collision_vector == glm::vec3(0, 0, 0)) {
      // no collision save current orientation
      saved_orientation = m_player.camera().orientation();
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
          // reset orientation
          std::cout << "too big distance" << std::endl;
          m_player.set_orientation(saved_orientation);
        } else {
          auto saved_position = m_player.camera().position();
          m_player.update_position(distance * direction);

          auto new_collision_vector = get_collision_vector();
          if (!new_collision_vector || fabs((*new_collision_vector)[0]) > EPS ||
              fabs((*new_collision_vector)[1]) > EPS ||
              fabs((*new_collision_vector)[2]) > EPS) {
            // collision cannot be solved, reset both position and orientation
            std::cout << "new collision" << std::endl;
            m_player.set_position(saved_position);
            m_player.set_orientation(saved_orientation);
          }
        }
      } else {
        // reset orientation
        std::cout << "prependicular" << std::endl;
        m_player.set_orientation(saved_orientation);
      }
    }
  };

  auto new_orientation = glm::rotate(
      m_player.camera().orientation(), glm::radians(-rotX),
      glm::cross(m_player.camera().orientation(), m_player.camera().up()));

  if (!(glm::angle(new_orientation, m_player.camera().up()) <=
            glm::radians(5.0f) ||
        glm::angle(new_orientation, -m_player.camera().up()) <=
            glm::radians(5.0f))) {
    m_player.set_orientation(new_orientation);
    resolve_collision();
  }

  m_player.set_orientation(glm::rotate(m_player.camera().orientation(),
                                       glm::radians(-rotY),
                                       m_player.camera().up()));
  resolve_collision();

  set_mouse_position(m_player.camera().width() / 2,
                     m_player.camera().height() / 2);
}

void PlayerController::process_keyboard_for_animation(float current_time) {
  if (is_key_pressed(GLFW_KEY_P)) {
    on_animation_start("CINEMA_4D_Main", AnimationData{0});
  } else if (is_mouse_button_pressed(MouseButton::Left)) {
    on_animation_start("shoot", AnimationData{0});
  }
}

