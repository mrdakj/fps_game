#ifndef _LEVEL_MANAGER_H_
#define _LEVEL_MANAGER_H_

#include "bounding_box.h"
#include "collision_detector.h"
#include "enemy.h"
#include "map.h"
#include "player.h"
#include "skinned_mesh.h"
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <vector>

class LevelManager {

public:
  LevelManager(const Map &map, const Player &player,
               const std::vector<const Enemy *> &enemies,
               CollistionDetector &collision_detector)
      : m_map(map), m_player(player), m_collision_detector(collision_detector) {
    init_rooms(m_map.bvh());
    for (auto enemy : enemies) {
      add_enemy(enemy);
    }
  }

  void init_rooms(const BVHNode<BoundingBox> &node) {
    if (m_map.is_room(node)) {
      m_rooms.emplace(&node, &node);
    }
    for (const auto &child : node.children) {
      init_rooms(*child);
    }
  }

  void add_enemy(const Enemy *enemy) {
    // add enemy to exactly one map
    std::queue<const BVHNode<BoundingBox> *> queue;
    queue.emplace(&m_map.bvh());
    while (!queue.empty()) {
      auto current_node = queue.front();
      queue.pop();
      if (enemy->bvh().volume.intersects(current_node->volume) !=
          glm::vec3(0, 0, 0)) {
        if (is_room(current_node)) {
          m_rooms[current_node].m_enemies.push_back(enemy);
          std::cout << "enemy added to :" << current_node->name << std::endl;
          return;
        }

        for (const auto &child : current_node->children) {
          queue.emplace(child.get());
        }
      }
    }

    assert(false && "enemy added to a map");
  }

  // return pointers to all rooms that intersects with player
  void update_active_rooms() {
    m_active_rooms.clear();

    std::queue<const BVHNode<BoundingBox> *> queue;
    queue.emplace(&m_map.bvh());
    while (!queue.empty()) {
      auto current_node = queue.front();
      queue.pop();
      if (m_player.bvh().volume.intersects(current_node->volume) !=
          glm::vec3(0, 0, 0)) {
        if (is_room(current_node)) {
          m_active_rooms.push_back(&m_rooms[current_node]);
        }

        for (const auto &child : current_node->children) {
          queue.emplace(child.get());
        }
      }
    }

    assert(!m_active_rooms.empty() && "player is in at least one room");
  }

  void update_collision_detector() {
    std::vector<const CollisionObject<BoundingBox> *> dynamic_objects;
    dynamic_objects.push_back(&m_player);

    for (auto const &active_room : m_active_rooms) {
      dynamic_objects.insert(dynamic_objects.end(),
                             active_room->m_enemies.begin(),
                             active_room->m_enemies.end());
    }

    std::vector<const CollisionObject<BoundingBox> *> static_objects;
    static_objects.insert(static_objects.end(),
                          std::make_move_iterator(m_active_rooms.begin()),
                          std::make_move_iterator(m_active_rooms.end()));

    m_collision_detector.set_objects(std::move(static_objects),
                                     std::move(dynamic_objects));
  }

  void culling() {
    m_map_meshes_to_render.clear();
    m_enemies_to_render.clear();

    auto camera_aabb = m_player.camera().get_bounding_box();

    std::queue<const BVHNode<BoundingBox> *> queue;
    queue.emplace(&m_map.bvh());
    while (!queue.empty()) {
      auto current_node = queue.front();
      queue.pop();
      if (camera_aabb.intersects(current_node->volume) != glm::vec3(0, 0, 0)) {
        // add node meshes
        m_map_meshes_to_render.insert(m_map_meshes_to_render.end(),
                                      current_node->mesh_ids.begin(),
                                      current_node->mesh_ids.end());

        // check enemies in room
        if (is_room(current_node)) {
          const auto &room_enemies = m_rooms[current_node].m_enemies;
          std::copy_if(room_enemies.begin(), room_enemies.end(),
                       std::back_inserter(m_enemies_to_render),
                       [&](const Enemy *enemy) {
                         return camera_aabb.intersects(enemy->bvh().volume) !=
                                glm::vec3(0, 0, 0);
                       });
        }

        for (const auto &child : current_node->children) {
          queue.emplace(child.get());
        }
      }
    }

    // std::cout << m_map_meshes_to_render.size() << ","
    //           << m_enemies_to_render.size() << std::endl;
  }

  void render_player(Shader &shader, Shader &bounding_box_shader, const Light &light) const {
    m_player.render(shader, bounding_box_shader, light);
  }

  void render_map(Shader &shader, Shader &bounding_box_shader,
                  const Camera &camera, const Light &light) const {
    m_map.render(shader, bounding_box_shader, camera, light,
                 m_map_meshes_to_render);
  }

  void render_to_texture_map(Shader &shader, const Camera &camera) const {
    shader.activate();
    shader.set_uniform<unsigned int>("gObjectIndex", 2);
    m_map.render_to_texture(shader, camera, m_map_meshes_to_render);
  }

  void render_enemies(Shader &shader, Shader &bounding_box_shader,
                      const Camera &camera, const Light &light) const {
    for (auto enemy_ptr : m_enemies_to_render) {
      enemy_ptr->render(shader, bounding_box_shader, camera, light);
    }
  }

  void render_to_texture_enemies(Shader &shader, const Camera &camera) const {
    shader.activate();
    shader.set_uniform<unsigned int>("gObjectIndex", 1);
    for (auto enemy_ptr : m_enemies_to_render) {
      enemy_ptr->render_to_texture(shader, camera);
    }
  }

private:
  class Room : public CollisionObject<BoundingBox> {
  public:
    Room(const BVHNode<BoundingBox> *bvh = nullptr) : m_bvh(bvh) {}

    const BVHNode<BoundingBox> &bvh() const override {
      assert(m_bvh);
      return *m_bvh;
    }

    std::vector<const Enemy *> m_enemies;
    const BVHNode<BoundingBox> *m_bvh;
  };

  bool is_room(const BVHNode<BoundingBox> *node) const {
    return m_rooms.find(node) != m_rooms.end();
  }

  const Map &m_map;
  const Player &m_player;

  std::unordered_map<const BVHNode<BoundingBox> *, Room> m_rooms;
  std::vector<const Room *> m_active_rooms;

  CollistionDetector &m_collision_detector;

  std::vector<unsigned int> m_map_meshes_to_render;
  std::vector<const Enemy *> m_enemies_to_render;
};

#endif /* _LEVEL_MANAGER_H_ */
