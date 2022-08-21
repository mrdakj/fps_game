#include "picking_texture.h"

PickingTexture::~PickingTexture() {
  if (m_fbo != 0) {
    glDeleteFramebuffers(1, &m_fbo);
  }

  if (m_picking_texture != 0) {
    glDeleteTextures(1, &m_picking_texture);
  }

  if (m_depth_texture != 0) {
    glDeleteTextures(1, &m_depth_texture);
  }
}

PickingTexture::PickingTexture(unsigned int window_width,
                               unsigned int window_height) {
  // create the FBO
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  // create the texture object for the primitive information buffer
  glGenTextures(1, &m_picking_texture);
  glBindTexture(GL_TEXTURE_2D, m_picking_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, window_width, window_height, 0,
               GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_picking_texture, 0);

  // create the texture object for the depth buffer
  glGenTextures(1, &m_depth_texture);
  glBindTexture(GL_TEXTURE_2D, m_depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, window_width,
               window_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         m_depth_texture, 0);

  // verify the FBO is correct
  GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (Status != GL_FRAMEBUFFER_COMPLETE) {
    throw "Frame buffer creation failed";
  }

  // restore the default framebuffer
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickingTexture::enable_writing() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
}

void PickingTexture::disable_writing() {
  // bind back the default framebuffer
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

PickingTexture::PixelInfo PickingTexture::read_pixel(unsigned int x,
                                                     unsigned int y) {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);

  glReadBuffer(GL_COLOR_ATTACHMENT0);

  PixelInfo pixel;
  // read rectangle with corner (x,y) and width and height of 1
  glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &pixel);

  glReadBuffer(GL_NONE);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  return pixel;
}
