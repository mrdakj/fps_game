#include "enemy_controller.h"

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

void EnemyController::update(float current_time) {
  if (m_object.is_shot() && !m_last_animation_triggered) {
    // stop the current ongoing animation
    on_animation_stop();
    on_animation_start("fall_dead", current_time);
    m_last_animation_triggered = true;
  } else if (!m_object.is_shot() && animation_can_start("rotate")) {
    float angle = m_object.get_player_angle();
    if (fabs(angle) >= 80 && fabs(angle) <= 100) {
      on_animation_start("rotate", current_time);
      m_reversed = angle > 0;
      if (m_reversed) {
        m_object.set_user_transformation(
            m_object.user_transformation() *
            glm::inverse(m_object.get_final_global_transformation_for_animation(
                m_animation_name)));
      }
    }
  }

  animation_update(current_time);
}

void EnemyController::on_animation_stop() {
  AnimationController::on_animation_stop();
  m_object.merge_user_and_global_transformations();
}

void EnemyController::animation_update(float current_time) {
  if (animation_ongoing()) {
    auto [animation_finished, global_transformation] = m_object.update(
        m_animation_name, current_time - m_animation_start_time, m_reversed);

    m_object.set_global_transformation(std::move(global_transformation));

    if (animation_finished) {
      on_animation_stop();
    }
  }
}
