#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "shader.h"
#include "texture.h"
#include <unordered_map>
#include <vector>

struct UVTransform {
  glm::mat3 translation = glm::mat4(1.0f);
  glm::mat3 rotation = glm::mat4(1.0f);
  glm::mat3 scaling = glm::mat4(1.0f);

  glm::mat3 get_transformation() const {
    return translation * rotation * scaling;
  }
};

class Material {
public:
  void add(const Texture *texture);
  void add(const Texture *texture, UVTransform uv_transform);

  void set_slots(Shader &shader, const std::string &uniform) const;
  void set_uv_transformations(Shader &shader, const std::string &uniform) const;

  void bind() const;
  void unbind() const;

private:
  glm::mat3 get_uv_transformation(const Texture *texture) const;

  std::vector<const Texture *> m_diffuse_tex;
  std::unordered_map<const Texture *, UVTransform> m_uv_transform_map;
};

#endif /* _MATERIAL_H_ */
