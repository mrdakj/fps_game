#include "material.h"
#include "texture.h"

void Material::add(const Texture *texture) {
  if (texture && texture->type() == TextureType::DIFFUSE) {
    m_diffuse_tex.push_back(texture);
  } else {
    throw "Unsupported texture type.";
  }
}

void Material::set_slots(Shader &shader, const std::string &uniform) const {
  for (int i = 0; i < m_diffuse_tex.size(); ++i) {
    // sampler2D uniform needs to be set as integer, not unsigned int
    shader.set_uniform<int>(uniform + std::to_string(i),
                            m_diffuse_tex[i]->slot());
  }
}

void Material::bind() const {
  for (const auto &texture : m_diffuse_tex) {
    texture->bind();
  }
}

void Material::unbind() const {
  for (const auto &texture : m_diffuse_tex) {
    texture->unbind();
  }
}
