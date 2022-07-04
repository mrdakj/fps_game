#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "shader.h"
#include "texture.h"
#include <vector>

class Material {
public:
  void add(const Texture* texture);

  void set_slots(Shader &shader, const std::string &uniform) const;
  void bind() const;
  void unbind() const;

private:
  std::vector<const Texture*> m_diffuse_tex;
};

#endif /* _MATERIAL_H_ */
