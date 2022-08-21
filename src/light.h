#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "shader.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

class Light {
public:
  Light(glm::vec4 color, glm::vec3 position);

  const glm::vec4 &color() const { return m_color; }
  const glm::vec3 &position() const { return m_position; }

private:
  glm::vec4 m_color;
  glm::vec3 m_position;
};

#endif /* _LIGHT_H_ */
