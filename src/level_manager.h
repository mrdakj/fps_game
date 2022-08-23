#ifndef _LEVEL_MANAGER_H_
#define _LEVEL_MANAGER_H_

#include "bounding_box.h"
#include "collision_detector.h"
#include "enemy.h"
#include "enemy_behavior_tree.h"
#include "map.h"
#include "nav_mesh.h"
#include "player.h"
#include "player_controller.h"
#include "skinned_mesh.h"
#include <algorithm>
#include <iterator>
#include <queue>
#include <unordered_set>
#include <vector>

#define ENEMIES_COUNT (5)

class LevelManager {
public:
  LevelManager(GLFWwindow *window, unsigned int window_width,
               unsigned int window_height)
      : m_map(), m_collision_detector(),
        m_camera(window_width, window_height, glm::vec3(10, 1.6, -32)),
        m_player(m_camera),
        m_player_controller(m_player, m_collision_detector, window) {

    m_enemies.reserve(ENEMIES_COUNT);
    for (unsigned int i = 0; i < ENEMIES_COUNT; ++i) {
      // call constructor for each enemy in order to create valid enemy's ids
      // creating vector as m_enemies(ENEMIES_COUNT, *this) will call
      // constructor once and then it will call copy constructor ENEMIES_COUNT
      // times which will copy the first enemy id to all enemies
      m_enemies.emplace_back(*this);
    }

    m_enemies[0].set_transformation(glm::vec3(2, 0.1, -2));
    m_enemies[1].set_transformation(glm::vec3(-2, 0.1, -2));
    m_enemies[2].set_transformation(glm::vec3(-5, 0.1, -23));
    m_enemies[3].set_transformation(glm::vec3(13, 0.1, -32));
    m_enemies[4].set_transformation(glm::vec3(-20, 0.1, -33));

    for (int i = 0; i < m_enemies.size(); ++i) {
      assert(m_enemies[i].m_id == i && "id valid");
      add_enemy_to_room(i);
    }
  }

  void add_enemy_to_room(unsigned int enemy_index) {
    // add enemy to exactly one room
    std::queue<const BVHNode<BoundingBox> *> queue;
    queue.emplace(&m_map.bvh());
    while (!queue.empty()) {
      auto current_node = queue.front();
      queue.pop();
      if (m_enemies[enemy_index].bvh().volume.intersects(
              current_node->volume) != glm::vec3(0, 0, 0)) {
        auto room_ptr = m_map.get_room(current_node);
        if (room_ptr) {
          m_room_to_enemies[room_ptr].push_back(enemy_index);
          m_enemy_to_room[enemy_index] = room_ptr;
          std::cout << "enemy " << enemy_index
                    << " added to :" << current_node->name << std::endl;
          return;
        }

        for (const auto &child : current_node->children) {
          queue.emplace(child.get());
        }
      }
    }

    assert(false && "enemy added to a room");
  }

  void update_active_rooms() {
    // find all rooms that intersects with player
    m_active_rooms.clear();

    std::queue<const BVHNode<BoundingBox> *> queue;
    queue.emplace(&m_map.bvh());
    while (!queue.empty()) {
      auto current_node = queue.front();
      queue.pop();
      if (m_player.bvh().volume.intersects(current_node->volume) !=
          glm::vec3(0, 0, 0)) {
        auto room_ptr = m_map.get_room(current_node);
        if (room_ptr) {
          m_active_rooms.push_back(room_ptr);
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

    for (auto active_room_ptr : m_active_rooms) {
      const auto &enemies_in_room = m_room_to_enemies[active_room_ptr];
      std::transform(
          enemies_in_room.begin(), enemies_in_room.end(),
          std::back_inserter(dynamic_objects),
          [&](unsigned int enemy_index) { return &m_enemies[enemy_index]; });
    }

    std::vector<const CollisionObject<BoundingBox> *> static_objects;
    static_objects.insert(static_objects.end(),
                          std::make_move_iterator(m_active_rooms.begin()),
                          std::make_move_iterator(m_active_rooms.end()));

    m_collision_detector.set_objects(std::move(static_objects),
                                     std::move(dynamic_objects));
  }

  bool raycasting(const glm::vec3 &A, const glm::vec3 &B) const {
    // return true if segment AB intersects some leaf node

    std::queue<const BVHNode<BoundingBox> *> queue;
    queue.emplace(&m_map.bvh());
    while (!queue.empty()) {
      auto current_node = queue.front();
      queue.pop();
      if (current_node->volume.intersects(A, B)) {
        if (current_node->children.empty()) {
          return true;
        }

        for (const auto &child : current_node->children) {
          queue.emplace(child.get());
        }
      }
    }

    return false;
  }

  void culling() {
    m_map_render_objects.clear();
    m_enemies_to_render.clear();

    auto camera_aabb = m_player.camera().get_bounding_box();

    std::queue<const BVHNode<BoundingBox> *> queue;
    queue.emplace(&m_map.bvh());
    while (!queue.empty()) {
      auto current_node = queue.front();
      queue.pop();
      if (camera_aabb.intersects(current_node->volume) != glm::vec3(0, 0, 0)) {
        // add node meshes
        if (current_node->render_object_id) {
          m_map_render_objects.push_back(*current_node->render_object_id);
        }

        // check enemies in room
        auto room_ptr = m_map.get_room(current_node);
        if (room_ptr) {
          const auto &room_enemies = m_room_to_enemies[room_ptr];
          std::copy_if(room_enemies.begin(), room_enemies.end(),
                       std::back_inserter(m_enemies_to_render),
                       [&](unsigned int enemy_index) {
                         return camera_aabb.intersects(
                                    m_enemies[enemy_index].bvh().volume) !=
                                glm::vec3(0, 0, 0);
                       });
        }

        for (const auto &child : current_node->children) {
          queue.emplace(child.get());
        }
      }
    }
  }

  void render_player() {
    m_player.render(skinned_mesh_shader, bounding_box_shader, light);
  }

  void render_map() {
    m_map.render(skinned_mesh_shader, bounding_box_shader, m_camera, light,
                 m_map_render_objects);
  }

  void render_to_texture_map() {
    picking_shader.activate();
    picking_shader.set_uniform<unsigned int>("gObjectIndex", 0);
    m_map.render_to_texture(picking_shader, m_camera, m_map_render_objects);
  }

  void render_enemies() {
    for (unsigned int enemy_index : m_enemies_to_render) {
      m_enemies[enemy_index].render(skinned_mesh_shader, bounding_box_shader,
                                    m_camera, light);
    }
  }

  void render_to_texture_enemies() {
    for (unsigned int enemy_index : m_enemies_to_render) {
      picking_shader.activate();
      picking_shader.set_uniform<unsigned int>("gObjectIndex", enemy_index + 1);
      m_enemies[enemy_index].render_to_texture(picking_shader, m_camera);
    }
  }

  void render() {
    render_enemies();
    render_player();
    render_map();
  }

  void render_to_texture() {
    render_to_texture_enemies();
    render_to_texture_map();
  }

  void render_primitive(unsigned int id, unsigned int entry,
                        unsigned int primitive) {
    if (id == 0) {
      m_map.render_primitive(picking_primitive_shader, m_camera, entry,
                             primitive);
    } else {
      m_enemies[id - 1].render_primitive(picking_primitive_shader, m_camera,
                                         entry, primitive);
    }
  }

  bool is_enemy_shot(unsigned int id) const {
    return id != 0 && id - 1 < m_enemies.size();
  }

  void set_enemy_shot(unsigned int id) {
    assert(id != 0 && id - 1 < m_enemies.size() && "valid shot id");
    m_enemies[id - 1].set_shot();
  }

  const glm::vec3 &player_position() const {
    return m_player.camera().position();
  }

  void update(float current_time) {
    // std::cout << m_player.camera().position()[0] << ","
    //           << m_player.camera().position()[2] << std::endl;

    update_active_rooms();
    update_collision_detector();
    m_player_controller.update(current_time);

    for (unsigned int i = 0; i < m_enemies.size(); ++i) {
      m_enemies[i].update(current_time);
    }

    // always update cammera matrix before culling
    m_camera.update_matrix();
    culling();
  }

  Path find_enemy_path(unsigned int enemy_id) const {
    auto room_it = m_enemy_to_room.find(enemy_id);
    assert(room_it != m_enemy_to_room.end() && room_it->second &&
           "enemy's room found");
    const auto &room_nav_mesh = room_it->second->m_nav_mesh;
    return room_nav_mesh.get_path(m_enemies[enemy_id].get_position(),
                                  room_nav_mesh.get_random_point());
  }

public:
  Map m_map;
  // map objects set during culling
  std::vector<unsigned int> m_map_render_objects;

  // rooms where player is in
  std::vector<const Map::Room *> m_active_rooms;
  // room to enemies ids that are in that room
  std::unordered_map<const Map::Room *, std::vector<unsigned int>>
      m_room_to_enemies;
  // enemy id to room where enemy is in
  std::unordered_map<unsigned int, const Map::Room *> m_enemy_to_room;

  CollistionDetector m_collision_detector;

  Camera m_camera;
  Player m_player;
  PlayerController m_player_controller;

  std::vector<Enemy> m_enemies;
  // enemies that are in active rooms
  std::vector<unsigned int> m_enemies_to_render;

  // objects used for rendering
  Light light{glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.5f, 0.0f)};

  Shader basic_shader{"../res/shaders/default.vert",
                      "../res/shaders/default.frag"};
  Shader skinned_mesh_shader{"../res/shaders/skinned_mesh.vert",
                             "../res/shaders/skinned_mesh.frag"};
  Shader bounding_box_shader{"../res/shaders/bounding_box.vert",
                             "../res/shaders/bounding_box.frag"};
  Shader picking_shader{"../res/shaders/picking.vert",
                        "../res/shaders/picking.frag"};
  Shader picking_primitive_shader{"../res/shaders/picking_primitive.vert",
                                  "../res/shaders/picking_primitive.frag"};
};

#endif /* _LEVEL_MANAGER_H_ */
