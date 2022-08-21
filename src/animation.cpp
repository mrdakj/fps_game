#include "animation.h"
#include "channel.h"
#include <iostream>

Animation::Animation(std::string name, std::vector<Channel> channels,
                     std::unordered_map<std::string, int> channels_map,
                     float duration, int ticks_per_second)
    : m_name(std::move(name)), m_channels(std::move(channels)),
      m_channels_map(std::move(channels_map)), m_duration(duration),
      m_ticks_per_second(ticks_per_second) {}

Animation::Animation(const aiAnimation *animation)
    : m_name(animation->mName.C_Str()), m_duration(animation->mDuration),
      m_ticks_per_second(animation->mTicksPerSecond != 0
                             ? animation->mTicksPerSecond
                             : 25.0f) {
  m_channels.reserve(animation->mNumChannels);
  for (int i = 0; i < animation->mNumChannels; ++i) {
    const auto *channel = animation->mChannels[i];
    m_channels_map.emplace(channel->mNodeName.data, m_channels.size());
    m_channels.emplace_back(channel);
  }
}

float Animation::get_animation_time(float time_in_seconds,
                                    float speed_factor) const {
  //  0   1   2
  // -3  -2  -1
  float time_in_ticks =
      (time_in_seconds >= 0 ? time_in_seconds : -time_in_seconds - 1) *
      m_ticks_per_second * speed_factor;

  if (time_in_seconds < 0) {
    time_in_ticks = m_duration - time_in_ticks;
    return std::max(time_in_ticks, 0.0f);
  }
  // return fmod(time_in_ticks, m_duration);
  return std::min(time_in_ticks, m_duration);
}

Channel *Animation::get_channel(const std::string &name) {
  auto it = m_channels_map.find(name);
  if (it != m_channels_map.end()) {
    return &m_channels[it->second];
  }

  return nullptr;
}
