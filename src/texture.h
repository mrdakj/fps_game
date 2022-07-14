#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "shader.h"
#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <string>

enum class TextureType { DIFFUSE, SPECULAR, INVALID };

class Texture {
public:
  Texture();
  Texture(const char *image, TextureType type, GLuint slot);
  Texture(Texture &&other);

  ~Texture();

  TextureType type() const { return m_type; }

  GLuint slot() const { return m_slot; }

  void bind() const;
  void unbind() const;
  void del();

private:
  GLuint m_id;
  GLuint m_slot;
  TextureType m_type;
};

#endif /* _TEXTURE_H_ */
