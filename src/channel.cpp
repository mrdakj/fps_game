#include "channel.h"
#include "utility.h"
#include <algorithm>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <set>

Channel::Channel(std::string name, std::vector<KeyPosition> positions,
                 std::vector<KeyRotation> rotations,
                 std::vector<KeyScale> scales)
    : m_name(std::move(name)), m_positions(std::move(positions)),
      m_rotations(std::move(rotations)), m_scales(std::move(scales)) {}

Channel::Channel(const aiNodeAnim *channel) : m_name(channel->mNodeName.data) {

  std::set<float> all_unique_time_stamps;
  for (int positionIndex = 0; positionIndex < channel->mNumPositionKeys;
       ++positionIndex) {
    aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
    float timeStamp = channel->mPositionKeys[positionIndex].mTime;
    KeyPosition data;
    data.position = utility::convert_to_glm_vec3(aiPosition);
    data.timeStamp = timeStamp;
    m_positions.push_back(data);
  }

  for (int rotationIndex = 0; rotationIndex < channel->mNumRotationKeys;
       ++rotationIndex) {
    aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
    float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
    KeyRotation data;
    data.orientation = utility::convert_to_glm_quat(aiOrientation);
    data.timeStamp = timeStamp;
    m_rotations.push_back(data);
  }

  for (int keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex) {
    aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
    float timeStamp = channel->mScalingKeys[keyIndex].mTime;
    KeyScale data;
    data.scale = utility::convert_to_glm_vec3(scale);
    data.timeStamp = timeStamp;
    m_scales.push_back(data);
  }
}

void Channel::update(float animation_time) {
  auto translation = interpolate_position(animation_time);
  auto rotation = interpolate_rotation(animation_time);
  auto scaling = interpolate_scaling(animation_time);
  m_transformation = translation * rotation * scaling;
}

int Channel::get_position_index(float animation_time) const {
  for (int index = 0; index < m_positions.size() - 1; ++index) {
    if (animation_time < m_positions[index + 1].timeStamp)
      return index;
  }
  return m_positions.size() - 1;
}

int Channel::get_rotation_index(float animation_time) const {
  for (int index = 0; index < m_rotations.size() - 1; ++index) {
    if (animation_time < m_rotations[index + 1].timeStamp)
      return index;
  }
  return m_rotations.size() - 1;
}

int Channel::get_scale_index(float animation_time) const {
  for (int index = 0; index < m_scales.size() - 1; ++index) {
    if (animation_time < m_scales[index + 1].timeStamp)
      return index;
  }
  return m_scales.size() - 1;
}

float Channel::get_factor(float last_time, float next_time,
                          float animation_time) const {
  float factor = 0.0f;
  float midWayLength = animation_time - last_time;
  float framesDiff = next_time - last_time;
  factor = midWayLength / framesDiff;
  return factor;
}

glm::mat4 Channel::interpolate_position(float animation_time) {
  if (1 == m_positions.size()) {
    m_translation = m_positions[0].position;
    return glm::translate(glm::mat4(1.0f), m_positions[0].position);
  }

  int p0Index = get_position_index(animation_time);
  if (p0Index == m_positions.size() - 1) {
    m_translation = m_positions.back().position;
    return glm::translate(glm::mat4(1.0f), m_positions.back().position);
  }
  int p1Index = p0Index + 1;
  float scaleFactor =
      get_factor(m_positions[p0Index].timeStamp, m_positions[p1Index].timeStamp,
                 animation_time);
  glm::vec3 finalPosition =
      glm::mix(m_positions[p0Index].position, m_positions[p1Index].position,
               scaleFactor);

  m_translation = finalPosition;
  return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Channel::interpolate_rotation(float animation_time) {
  if (1 == m_rotations.size()) {
    auto rotation = glm::normalize(m_rotations[0].orientation);
    m_rotation = rotation;
    return glm::toMat4(rotation);
  }

  int p0Index = get_rotation_index(animation_time);
  if (p0Index == m_rotations.size() - 1) {
    auto rotation = glm::normalize(m_rotations.back().orientation);
    m_rotation = rotation;
    return glm::toMat4(rotation);
  }
  int p1Index = p0Index + 1;
  float scaleFactor =
      get_factor(m_rotations[p0Index].timeStamp, m_rotations[p1Index].timeStamp,
                 animation_time);
  glm::quat finalRotation =
      glm::slerp(m_rotations[p0Index].orientation,
                 m_rotations[p1Index].orientation, scaleFactor);
  finalRotation = glm::normalize(finalRotation);
  m_rotation = finalRotation;
  return glm::toMat4(finalRotation);
}

glm::mat4 Channel::interpolate_scaling(float animation_time) {
  if (1 == m_scales.size()) {
    m_scaling = m_scales[0].scale;
    return glm::scale(glm::mat4(1.0f), m_scales[0].scale);
  }

  int p0Index = get_scale_index(animation_time);
  if (p0Index == m_scales.size() - 1) {
    m_scaling = m_scales.back().scale;
    return glm::scale(glm::mat4(1.0f), m_scales.back().scale);
  }
  int p1Index = p0Index + 1;
  float scaleFactor = get_factor(m_scales[p0Index].timeStamp,
                                 m_scales[p1Index].timeStamp, animation_time);
  glm::vec3 finalScale =
      glm::mix(m_scales[p0Index].scale, m_scales[p1Index].scale, scaleFactor);
  m_scaling = finalScale;
  return glm::scale(glm::mat4(1.0f), finalScale);
}
