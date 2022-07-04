#include "light.h"
#include <utility>

Light::Light(glm::vec4 color, glm::vec3 position)
    : m_color(std::move(color)), m_position(std::move(position)) {}

