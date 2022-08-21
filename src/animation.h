#ifndef _ANIMATION_H_
#define _ANIMATION_H_

#include "aabb.h"
#include "channel.h"

#include <assimp/scene.h>

#include <string>
#include <unordered_map>
#include <vector>

class Animation {
public:
  Animation(const aiAnimation *animation);
  Animation(std::string name, std::vector<Channel> channels,
            std::unordered_map<std::string, int> channels_map, float duration,
            int ticks_per_second);

  float get_animation_time(float time_in_seconds,
                           float speed_factor = 1.0f) const;

  Channel *get_channel(const std::string &name);

  // TODO fix set private
public:
  std::string m_name;
  float m_duration;
  int m_ticks_per_second;

  std::vector<Channel> m_channels;
  // channel name -> channel index in m_channels
  std::unordered_map<std::string, int> m_channels_map;
};

#endif /* _ANIMATION_H_ */
