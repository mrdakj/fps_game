#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <assimp/anim.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <set>
#include <vector>

struct KeyPosition {
  glm::vec3 position;
  float timeStamp;
};

struct KeyRotation {
  glm::quat orientation;
  float timeStamp;
};

struct KeyScale {
  glm::vec3 scale;
  float timeStamp;
};

class Channel {
private:
  std::string m_name;
  std::vector<KeyPosition> m_positions;
  std::vector<KeyRotation> m_rotations;
  std::vector<KeyScale> m_scales;

  glm::vec3 m_translation;
  glm::quat m_rotation;
  glm::vec3 m_scaling;
  glm::mat4 m_transformation;

public:
  Channel(const aiNodeAnim *channel);
  Channel(std::string name, std::vector<KeyPosition> positions,
          std::vector<KeyRotation> rotations, std::vector<KeyScale> scales);

  const glm::mat4 &get_local_transform() const { return m_transformation; }
  const glm::vec3 &get_local_translation() const { return m_translation; }
  const glm::vec3 &get_local_scaling() const { return m_scaling; }
  const glm::quat &get_local_rotation() const { return m_rotation; }

  const std::vector<KeyPosition> &positions_channel() const {
    return m_positions;
  };
  const std::vector<KeyRotation> &rotations_channel() const {
    return m_rotations;
  };
  const std::vector<KeyScale> &scales_channel() const { return m_scales; };

  void update(float animation_time);
  int get_position_index(float animation_time) const;
  int get_rotation_index(float animation_time) const;
  int get_scale_index(float animation_time) const;

  float get_factor(float last_time, float next_time,
                   float animation_time) const;

  glm::mat4 interpolate_position(float animation_time);
  glm::mat4 interpolate_rotation(float animation_time);
  glm::mat4 interpolate_scaling(float animation_time);
};

#endif /* _CHANNEL_H_ */
