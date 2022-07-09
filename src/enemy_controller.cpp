#include "enemy_controller.h"
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

const float GUN_PLAYER_ANGLE_THRESHOLD = 10;
const float ENEMY_ROTATION_ANGLE_THRESHOLD = 50;
const std::string ROTATE_ANIMATION = "rotate";
const float ROTATE_ANIMATION_SPEED_FACTOR = 2.5f;
const std::string FALL_DEAD_ANIMATION = "fall_dead";
const std::string SHOOT_ANIMATION = "shoot";
const float SHOOT_ANIMATION_SPEED_FACTOR = 2.0f;
const float SHOOTING_DURATION_SECONDS = 2;

void EnemyController::update(float current_time) {
  if (m_last_update_time == -1) {
    // set last update time for the first time
    m_last_update_time = current_time;
  }

  if (!check_if_dead_and_animate()) {
    // rotate hips or start rotation animation
    rotate(current_time);
    shoot();
  }

  animation_update(current_time);

  m_last_update_time = current_time;
}

void EnemyController::rotate(float current_time) {
  float degrees_to_rotate =
      get_rotation_angle(current_time - m_last_update_time);

  if (m_rotation_angle + degrees_to_rotate > ENEMY_ROTATION_ANGLE_THRESHOLD) {
    degrees_to_rotate = ENEMY_ROTATION_ANGLE_THRESHOLD - m_rotation_angle;
    start_rotate_left();
  } else if (m_rotation_angle + degrees_to_rotate <
             -ENEMY_ROTATION_ANGLE_THRESHOLD) {
    degrees_to_rotate = -ENEMY_ROTATION_ANGLE_THRESHOLD - m_rotation_angle;
    start_rotate_right();
  }

  m_rotation_angle += degrees_to_rotate;
  m_enemy.rotate(degrees_to_rotate);
}

void EnemyController::shoot() {
  // should shooting start
  float gun_player_angle = m_enemy.get_gun_player_angle();
  if (fabs(gun_player_angle) < GUN_PLAYER_ANGLE_THRESHOLD) {
    if (!m_is_shooting) {
      m_is_shooting = on_animation_start(
          SHOOT_ANIMATION,
          AnimationData{0, SHOOT_ANIMATION_SPEED_FACTOR, false});
    }
  } else if (m_is_shooting) {
    // should shooting end
    m_is_shooting = !on_animation_start(
        SHOOT_ANIMATION,
        AnimationData{
            -1, SHOOT_ANIMATION_SPEED_FACTOR, false, {ROTATE_ANIMATION}});
  }
}

float EnemyController::get_rotation_angle(float delta_time) const {
  float gun_player_angle = m_enemy.get_gun_player_angle();
  return gun_player_angle > GUN_PLAYER_ANGLE_THRESHOLD
             ? m_rotation_speed * delta_time
         : gun_player_angle < -GUN_PLAYER_ANGLE_THRESHOLD
             ? -m_rotation_speed * delta_time
             : 0;
}

void EnemyController::start_rotate_left() {
  bool start = on_animation_start(
      ROTATE_ANIMATION,
      AnimationData{
          -1, ROTATE_ANIMATION_SPEED_FACTOR, true, {SHOOT_ANIMATION}});
  if (start) {
    m_enemy.set_user_transformation(
        m_enemy.user_transformation() *
        glm::inverse(m_enemy.get_final_global_transformation_for_animation(
            ROTATE_ANIMATION)));
  }
}

void EnemyController::start_rotate_right() {
  on_animation_start(
      ROTATE_ANIMATION,
      AnimationData{0, ROTATE_ANIMATION_SPEED_FACTOR, true, {SHOOT_ANIMATION}});
}

bool EnemyController::check_if_dead_and_animate() {
  bool is_shot = m_enemy.is_shot();

  if (is_shot) {
    on_animation_start(FALL_DEAD_ANIMATION, AnimationData{0},
                       true /* stop all active */);
    disable_animation_start();
  }

  return is_shot;
}
