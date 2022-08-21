#ifndef _PICKING_TEXTURE_H
#define _PICKING_TEXTURE_H

#include <GL/glew.h>

#define INF 99999

class PickingTexture {
public:
  PickingTexture(unsigned int window_width, unsigned int window_height);
  ~PickingTexture();

  void enable_writing();

  void disable_writing();

  struct PixelInfo {
    unsigned int object_id = INF;
    unsigned int draw_id = 0;
    unsigned int primitive_id = 0;

    bool is_set() const { return object_id < INF; }
  };

  PixelInfo read_pixel(unsigned int x, unsigned int y);

private:
  GLuint m_fbo = 0;
  GLuint m_picking_texture = 0;
  GLuint m_depth_texture = 0;
};

#endif /* _PICKING_TEXTURE_H */
