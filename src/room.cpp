#include "room.h"
#include "aabb.h"
#include "camera.h"
#include "light.h"
#include "texture.h"
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <utility>
#include <vector>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texture;
  unsigned int texture_slot;
};

Room::Room(int width, int depth, int height, glm::vec3 down_left)
    : m_width(width), m_depth(depth), m_height(height),
      m_down_left(std::move(down_left)),
      m_floor_texture(
          Texture("../res/textures/floor.bmp", TextureType::DIFFUSE, 0)),
      m_wall_texture(
          Texture("../res/textures/wall.bmp", TextureType::DIFFUSE, 1)) {
  std::vector<Vertex> vertices = {
      // floor
      Vertex{m_down_left, glm::vec3(0, 1, 0), glm::vec2(0, 0), 0},
      Vertex{m_down_left + glm::vec3(m_width, 0, 0), glm::vec3(0, 1, 0),
             glm::vec2(m_width, 0), 0},
      Vertex{m_down_left + glm::vec3(m_width, 0, -m_depth), glm::vec3(0, 1, 0),
             glm::vec2(m_width, m_depth), 0},
      Vertex{m_down_left + glm::vec3(0, 0, -m_depth), glm::vec3(0, 1, 0),
             glm::vec2(0, m_depth), 0},
      // front wall
      Vertex{m_down_left, glm::vec3(0, 0, -1), glm::vec2(0, 0), 1},
      Vertex{m_down_left + glm::vec3(m_width, 0, 0), glm::vec3(0, 0, -1),
             glm::vec2(m_width, 0), 1},
      Vertex{m_down_left + glm::vec3(m_width, m_height, 0), glm::vec3(0, 0, -1),
             glm::vec2(m_width, m_height), 1},
      Vertex{m_down_left + glm::vec3(0, m_height, 0), glm::vec3(0, 0, -1),
             glm::vec2(0, m_height), 1},
      // back wall
      Vertex{m_down_left + glm::vec3(0, 0, -m_depth), glm::vec3(0, 0, 1),
             glm::vec2(0, 0), 1},
      Vertex{m_down_left + glm::vec3(m_width, 0, -m_depth), glm::vec3(0, 0, 1),
             glm::vec2(m_width, 0), 1},
      Vertex{m_down_left + glm::vec3(m_width, m_height, -m_depth),
             glm::vec3(0, 0, 1), glm::vec2(m_width, m_height), 1},
      Vertex{m_down_left + glm::vec3(0, m_height, -m_depth), glm::vec3(0, 0, 1),
             glm::vec2(0, m_height), 1},
      // left wall
      Vertex{m_down_left, glm::vec3(1, 0, 0), glm::vec2(0, 0), 1},
      Vertex{m_down_left + glm::vec3(0, 0, -m_depth), glm::vec3(1, 0, 0),
             glm::vec2(m_depth, 0), 1},
      Vertex{m_down_left + glm::vec3(0, m_height, -m_depth), glm::vec3(1, 0, 0),
             glm::vec2(m_depth, m_height), 1},
      Vertex{m_down_left + glm::vec3(0, m_height, 0), glm::vec3(1, 0, 0),
             glm::vec2(0, m_height), 1},
      // right wall
      Vertex{m_down_left + glm::vec3(m_width, 0, 0), glm::vec3(-1, 0, 0),
             glm::vec2(0, 0), 1},
      Vertex{m_down_left + glm::vec3(m_width, 0, -m_depth), glm::vec3(-1, 0, 0),
             glm::vec2(m_depth, 0), 1},
      Vertex{m_down_left + glm::vec3(m_width, m_height, -m_depth),
             glm::vec3(-1, 0, 0), glm::vec2(m_depth, m_height), 1},
      Vertex{m_down_left + glm::vec3(m_width, m_height, 0), glm::vec3(-1, 0, 0),
             glm::vec2(0, m_height), 1},
  };

  std::vector<GLuint> indices = {// floor
                                 0, 1, 2, 2, 3, 0,
                                 // front wall
                                 4, 5, 6, 6, 7, 4,
                                 // back wall
                                 8, 9, 10, 10, 11, 8,
                                 // left wall
                                 12, 13, 14, 14, 15, 12,
                                 // right wall
                                 16, 17, 18, 18, 19, 16};

  m_indices_count = indices.size();

  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid *)offsetof(Vertex, position));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid *)offsetof(Vertex, normal));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid *)offsetof(Vertex, texture));
  glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(Vertex),
                         (const GLvoid *)offsetof(Vertex, texture_slot));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  glGenBuffers(1, &m_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_indices_count,
               indices.data(), GL_STATIC_DRAW);

  // unbind buffers
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Room::render(Shader &shader, const Camera &camera,
                  const Light &light) const {
  shader.activate();

  shader.set_uniform("camPos", camera.position());
  shader.set_uniform("camMatrix", camera.matrix());

  shader.set_uniform("lightPos", light.position());
  shader.set_uniform("lightColor", light.color());

  glBindVertexArray(m_vao);

  // sampler2D uniform needs to be set as integer, not unsigned int
  shader.set_uniform<int>("diffuse[0]", m_floor_texture.slot());
  m_floor_texture.bind();

  // sampler2D uniform needs to be set as integer, not unsigned int
  shader.set_uniform<int>("diffuse[1]", m_wall_texture.slot());
  m_wall_texture.bind();

  shader.set_uniform("model", glm::mat4(1.0));

  glDrawElements(GL_TRIANGLES, m_indices_count, GL_UNSIGNED_INT, 0);

  // unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_floor_texture.unbind();
  m_wall_texture.unbind();
}

Room::~Room() {
  glDeleteVertexArrays(1, &m_vao);
  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(1, &m_ebo);
}

BoundingVolumeHierarchy<BoundingBox> Room::get_bounding_volumes() const {
  AABB floor_aabb;
  floor_aabb.update(m_down_left);
  floor_aabb.update(m_down_left + glm::vec3(m_width, 0, 0));
  floor_aabb.update(m_down_left + glm::vec3(m_width, 0, -m_depth));
  floor_aabb.update(m_down_left + glm::vec3(0, 0, -m_depth));

  AABB front_wall_aabb;
  front_wall_aabb.update(m_down_left);
  front_wall_aabb.update(m_down_left + glm::vec3(m_width, 0, 0));
  front_wall_aabb.update(m_down_left + glm::vec3(m_width, m_height, 0));
  front_wall_aabb.update(m_down_left + glm::vec3(0, m_height, 0));

  AABB back_wall_aabb;
  back_wall_aabb.update(m_down_left + glm::vec3(0, 0, -m_depth));
  back_wall_aabb.update(m_down_left + glm::vec3(m_width, 0, -m_depth));
  back_wall_aabb.update(m_down_left + glm::vec3(m_width, m_height, -m_depth));
  back_wall_aabb.update(m_down_left + glm::vec3(0, m_height, -m_depth));

  AABB left_wall_aabb;
  left_wall_aabb.update(m_down_left);
  left_wall_aabb.update(m_down_left + glm::vec3(0, 0, -m_depth));
  left_wall_aabb.update(m_down_left + glm::vec3(0, m_height, -m_depth));
  left_wall_aabb.update(m_down_left + glm::vec3(0, m_height, 0));

  AABB right_wall_aabb;
  right_wall_aabb.update(m_down_left + glm::vec3(m_width, 0, 0));
  right_wall_aabb.update(m_down_left + glm::vec3(m_width, 0, -m_depth));
  right_wall_aabb.update(m_down_left + glm::vec3(m_width, m_height, -m_depth));
  right_wall_aabb.update(m_down_left + glm::vec3(m_width, m_height, 0));

  std::vector<BoundingBox> children;
  children.emplace_back(std::move(floor_aabb));
  children.emplace_back(std::move(front_wall_aabb));
  children.emplace_back(std::move(back_wall_aabb));
  children.emplace_back(std::move(left_wall_aabb));
  children.emplace_back(std::move(right_wall_aabb));

  return std::move(children);
}
