#ifndef _CURSOR_H_
#define _CURSOR_H_

#include "shader.h"
#include <GL/glew.h>

class Cursor {
public:
  Cursor();

  void render();

  // private:
  GLuint m_vao;
  GLuint m_vbo;
  GLuint m_ebo;
  Shader m_shader;
};

#endif /* _CURSOR_H_ */
