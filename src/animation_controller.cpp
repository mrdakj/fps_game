#include "animation_controller.h"

AnimationController::AnimationController(std::string name, bool reversed,
                                         float speed_factor, bool update_global)
    : m_name(std::move(name)), m_reversed(reversed),
      m_speed_factor(speed_factor), m_update_global(update_global),
      m_last_update_time(-1), m_current_animation_time(m_reversed ? -1 : 0) {}

bool AnimationController::update(AnimatedMesh &mesh, float current_time) {
  if (m_update_global && m_reversed && m_first_tick) {
    // set inverse of global transformation
    mesh.set_user_transformation(
        mesh.user_transformation() *
        glm::inverse(
            mesh.get_final_global_transformation_for_animation(m_name)));
  }

  update_animation_time(current_time);

  auto [animation_finished, global_transformation] =
      mesh.update(m_name, m_current_animation_time, m_speed_factor);

  if (m_update_global) {
    mesh.set_global_transformation(std::move(global_transformation));
  }

  m_first_tick = false;

  if (animation_finished) {
    on_animation_stop(mesh);
  }

  return animation_finished;
}

void AnimationController::reset_animation() {
  m_current_animation_time = m_reversed ? -1 : 0;
  m_first_tick = true;
}

void AnimationController::update_animation_time(float current_time) {
  float delta = m_first_tick ? 0 : current_time - m_last_update_time;
  m_last_update_time = current_time;

  m_current_animation_time = m_reversed ? m_current_animation_time - delta
                                        : m_current_animation_time + delta;
}

void AnimationController::on_animation_stop(AnimatedMesh &mesh) {
  if (m_update_global) {
    mesh.merge_user_and_global_transformations();
  }
  reset_animation();
}
