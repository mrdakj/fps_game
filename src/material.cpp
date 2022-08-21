#include "material.h"
#include "texture.h"

#include "utility.h"

void Material::add(const Texture *texture) {
  if (texture && texture->type() == TextureType::DIFFUSE) {
    m_diffuse_tex.push_back(texture);
  } else {
    throw "Unsupported texture type.";
  }
}

void Material::add(const Texture *texture, UVTransform uv_transform) {
  if (texture && texture->type() == TextureType::DIFFUSE) {
    m_diffuse_tex.push_back(texture);
    m_uv_transform_map.emplace(texture, std::move(uv_transform));
  } else {
    throw "Unsupported texture type.";
  }
}

glm::mat3 Material::get_uv_transformation(const Texture *texture) const {
  auto uv_transform_it = m_uv_transform_map.find(texture);
  return uv_transform_it != m_uv_transform_map.end()
             ? uv_transform_it->second.get_transformation()
             : glm::mat3(1.0f);
}

void Material::set_slots(Shader &shader, const std::string &uniform) const {
  for (int i = 0; i < m_diffuse_tex.size(); ++i) {
    // sampler2D uniform needs to be set as integer, not unsigned int
    shader.set_uniform<int>(uniform + std::to_string(i),
                            m_diffuse_tex[i]->slot());
  }
}

void Material::set_uv_transformations(Shader &shader,
                                      const std::string &uniform) const {
  for (int i = 0; i < m_diffuse_tex.size(); ++i) {
    shader.set_uniform(uniform + std::to_string(i),
                       get_uv_transformation(m_diffuse_tex[i]));
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
