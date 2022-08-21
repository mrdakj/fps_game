#include "map.h"
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
#include <queue>
#include <utility>
#include <vector>

Map::Map()
    : m_mesh{"../res/models/level1/level1.gltf"},
      m_room_nav_mesh_names{
          {"room1", "../res/models/level1_nav_mesh/room1_nav_mesh.gltf"},
          {"room2", "../res/models/level1_nav_mesh/room2_nav_mesh.gltf"},
          {"room3", "../res/models/level1_nav_mesh/room3_nav_mesh.gltf"},
          {"room4", "../res/models/level1_nav_mesh/room4_nav_mesh.gltf"}} {
  init_rooms(bvh());
}

void Map::init_rooms(const BVHNode<BoundingBox> &node) {
  if (is_room(node)) {
    std::cout << "Map: found room " << node.name << std::endl;
    m_rooms.emplace_back(&node, m_room_nav_mesh_names[node.name]);
    m_rooms_index.emplace(&node, m_rooms.size() - 1);
  }
  for (const auto &child : node.children) {
    init_rooms(*child);
  }
}

const Map::Room *Map::get_room(const BVHNode<BoundingBox> *node) const {
  // it is ok to return a pointer of vector element because vector won't be
  // changed anymore and reallocation won't happen which can invalidate a
  // pointer
  auto room_it = m_rooms_index.find(node);
  return room_it != m_rooms_index.end() ? &m_rooms[room_it->second] : nullptr;
}

void Map::render(Shader &shader, Shader &bounding_box_shader,
                 const Camera &camera, const Light &light,
                 const std::vector<unsigned int> &mesh_ids) const {
  shader.activate();
  shader.set_uniform("transformation", glm::mat4(1.0f));
  m_mesh.render(shader, camera, light, mesh_ids);

#ifdef FPS_DEBUG
  render_nav_meshes(bounding_box_shader, camera);
  render_boxes(*get_bvh(), bounding_box_shader, camera);
#endif
}

void Map::render_nav_meshes(Shader &bounding_box_shader,
                            const Camera &camera) const {
  for (const auto &room : m_rooms) {
    room.m_nav_mesh.render(bounding_box_shader, camera);
  }
}

void Map::render_boxes(const BVHNode<BoundingBox> &node,
                       Shader &bounding_box_shader,
                       const Camera &camera) const {
  node.volume.render(bounding_box_shader, camera, glm::vec3(1.0, 0.0, 0.0));
  for (const auto &child : node.children) {
    render_boxes(*child, bounding_box_shader, camera);
  }
}

void Map::render_to_texture(Shader &shader, const Camera &camera,
                            const std::vector<unsigned int> &mesh_ids) const {
  shader.activate();
  shader.set_uniform("transformation", glm::mat4(1.0f));
  m_mesh.render_to_texture(shader, camera, mesh_ids);
}

void Map::render_primitive(Shader &shader, const Camera &camera,
                           unsigned int entry, unsigned int primitive) const {
  shader.activate();
  shader.set_uniform("transformation", glm::mat4(1.0f));
  m_mesh.render_primitive(shader, camera, entry, primitive);
}

std::unique_ptr<BVHNode<BoundingBox>> Map::get_bvh() const {
  return m_mesh.get_bvh();
}

bool Map::is_room(const BVHNode<BoundingBox> &node) const {
  return std::any_of(m_room_nav_mesh_names.begin(), m_room_nav_mesh_names.end(),
                     [&](const auto &room_nav_mesh_name) {
                       return node.name == room_nav_mesh_name.first;
                     });
}
