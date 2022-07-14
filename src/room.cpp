#include "room.h"
#include "aabb.h"
#include "camera.h"
#include "collision_object.h"
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

Room::Room(const std::string &file_name) : m_mesh{file_name} {}

void Room::render(Shader &shader, Shader &bounding_box_shader,
                  const Camera &camera, const Light &light) const {
  shader.activate();
  shader.set_uniform("transformation", glm::mat4(1.0f));
  m_mesh.render(shader, camera, light);

  render_boxes(*get_bvh(), bounding_box_shader, camera);
}

void Room::render_boxes(const BVHNode<BoundingBox> &node,
                        Shader &bounding_box_shader,
                        const Camera &camera) const {
  node.volume.render(bounding_box_shader, camera, glm::vec3(1.0, 0.0, 0.0));
  for (const auto &child : node.children) {
    render_boxes(*child, bounding_box_shader, camera);
  }
}

void Room::render_to_texture(Shader &shader, const Camera &camera) {
  shader.activate();
  shader.set_uniform("transformation", glm::mat4(1.0f));
  m_mesh.render_to_texture(shader, camera);
}

std::unique_ptr<BVHNode<BoundingBox>> Room::get_bvh() const {
  return m_mesh.get_bvh();
}
