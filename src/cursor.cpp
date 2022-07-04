#include "cursor.h"
#include "shader.h"

Cursor::Cursor()
    : m_shader("../res/shaders/cursor.vert", "../res/shaders/cursor.frag") {
  m_shader.activate();

  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);
  static const GLfloat vertices[] = {
      -0.01, -0.02, 0,
      0.01, -0.02, 0,
      0.0, 0.0, 0.0,
  };
  // This will identify our vertex buffer
  // Generate 1 buffer, put the resulting identifier in vertexbuffer
  glGenBuffers(1, &m_vbo);
  // The following commands will talk about our 'vertexbuffer' buffer
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  // Give our vertices to OpenGL.
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, // attribute 0. No particular reason for 0, but must
                           // match the layout in the shader.
                        3, // size
                        GL_FLOAT, // type
                        GL_FALSE, // normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
  );
}

void Cursor::render() {
  m_shader.activate();
  glBindVertexArray(m_vao);

  glDrawArrays(GL_TRIANGLES, 0,
               3); // Starting from vertex 0; 3 vertices total -> 1 triangle

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
