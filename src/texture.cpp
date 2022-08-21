#include "texture.h"
#include <stb/stb_image.h>
#include <stdexcept>
#include <string>
#include "utility.h"

Texture::~Texture() {
  if (m_type != TextureType::INVALID) {
    del();
  }
}

Texture::Texture(const char *image, TextureType type, GLuint slot)
    : m_type(type), m_slot(slot) {

  // loat image using stb library
  int img_width, img_height, chanel_count;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *bytes =
      stbi_load(image, &img_width, &img_height, &chanel_count, 0);

  glGenTextures(1, &m_id);
  bind();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // check type of color channels the texture has and load it accordingly
  if (chanel_count == 4)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, bytes);
  else if (chanel_count == 3)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, bytes);
  else if (chanel_count == 1)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, bytes);
  else
    throw std::invalid_argument("Automatic Texture type recognition failed.");

  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(bytes);
  unbind();
}

Texture::Texture(Texture &&other) {
  m_id = std::move(other.m_id);
  m_slot = std::move(other.m_slot);
  m_type = std::move(other.m_type);
  other.m_type = TextureType::INVALID;
}

void Texture::bind() const {
  glActiveTexture(GL_TEXTURE0 + m_slot);
  glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

void Texture::del() { glDeleteTextures(1, &m_id); }
