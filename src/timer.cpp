#include "timer.h"

Timer::Timer() : m_last_update_time(-1) {}

void Timer::reset() { m_last_update_time = -1; }

float Timer::tick(float current_time) {
  if (m_last_update_time == -1) {
    m_last_update_time = current_time;
  }
  float delta_time = current_time - m_last_update_time;
  m_last_update_time = current_time;
  return delta_time;
}
