#include "animation_controller.h"
#include "sound.h"
#include <future>

AnimationController::AnimationController(AnimatedMesh &mesh, std::string name,
                                         bool reversed, float speed_factor,
                                         bool update_global)
    : m_mesh(mesh), m_name(std::move(name)), m_reversed(reversed),
      m_speed_factor(speed_factor), m_update_global(update_global),
      m_current_animation_time(m_reversed ? -1 : 0),
      m_sound_track(std::nullopt) {}

AnimationController::AnimationController(AnimatedMesh &mesh, std::string name,
                                         Sound::Track sound_track,
                                         bool reversed, float speed_factor,
                                         bool update_global)
    : m_mesh(mesh), m_name(std::move(name)), m_reversed(reversed),
      m_speed_factor(speed_factor), m_update_global(update_global),
      m_current_animation_time(m_reversed ? -1 : 0),
      m_sound_track(sound_track) {}

std::pair<bool, glm::mat4> AnimationController::update(float delta_time) {
  if (m_update_global && m_reversed && m_first_tick) {
    // set inverse of global transformation
    m_mesh.set_user_transformation(
        m_mesh.user_transformation() *
        glm::inverse(
            m_mesh.get_final_global_transformation_for_animation(m_name)));
  }

  update_animation_time(delta_time);

  auto [animation_finished, global_transformation] =
      m_mesh.update(m_name, m_current_animation_time, m_speed_factor);

  if (m_update_global) {
    m_mesh.set_global_transformation(global_transformation);
  }

  if (m_first_tick && m_sound_track) {
    SoundPlayer::instance().play_track(*m_sound_track);
  }

  m_first_tick = false;

  if (animation_finished) {
    on_animation_stop();
  }

  return {animation_finished, std::move(global_transformation)};
}

void AnimationController::reset_animation() {
  m_current_animation_time = m_reversed ? -1 : 0;
  m_first_tick = true;
}

void AnimationController::update_animation_time(float delta_time) {
  if (!m_first_tick) {
    m_current_animation_time = m_reversed
                                   ? m_current_animation_time - delta_time
                                   : m_current_animation_time + delta_time;
  }
}

void AnimationController::on_animation_stop() {
  if (m_update_global) {
    m_mesh.merge_user_and_global_transformations();
  }
  reset_animation();
}
