#ifndef _ROOM_H_
#define _ROOM_H_

#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "light.h"
#include "shader.h"
#include "texture.h"
#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <vector>

class Room : public CollisionObject<BoundingBox> {
public:
  Room(int width, int depth, int height, glm::vec3 down_left);
  ~Room();

  void render(Shader &shader, const Camera &camera, const Light &light) const;

  float floor_height() const { return m_down_left.y; }
  float front_wall_z() const { return m_down_left.z; }
  float back_wall_z() const { return m_down_left.z - m_depth; }
  float left_wall_x() const { return m_down_left.x; }
  float right_wall_x() const { return m_down_left.x + m_width; }

  BoundingVolumeHierarchy<BoundingBox> get_bounding_volumes() const override;

private:
  int m_width;
  int m_depth;
  int m_height;
  glm::vec3 m_down_left;

  unsigned int m_indices_count;

  Texture m_floor_texture;
  Texture m_wall_texture;

  GLuint m_vao;
  GLuint m_vbo;
  GLuint m_ebo;
};

#endif /* _ROOM_H_ */
