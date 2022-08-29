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

class LevelManager {
public:
  LevelManager(GLFWwindow *window, unsigned int window_width,
               unsigned int window_height);

  // basic rendering
  void render();
  // render to texture is used to check if enemy is shot (3d mouse picking)
  void render_to_texture();
  // render primitive (as red triangle) that is shot (picked by mouse)
  // this is used only for testing
  void render_primitive(unsigned int id, unsigned int entry,
                        unsigned int primitive);

  // return true if id belongs to enemy and not to the map
  bool is_enemy_shot(unsigned int id) const;
  // set enemy with the given id to be dead
  void set_enemy_shot(unsigned int id);

  // reduce one life from player
  void player_shot();
  // check if player is dead
  bool is_player_dead() const;

  // return player's position
  const glm::vec3 &player_position() const;

  // find a path for the enemy
  Path find_enemy_path(unsigned int enemy_id) const;

  // return true if a segment AB intersects some mesh bounding box
  bool raycasting(const glm::vec3 &A, const glm::vec3 &B) const;

  // notify enemies in active room about player's position if player is shooting
  void notify_enemies();

  // update the state
  void update(float current_time);

  void reset();

  bool player_shoot_started() const;

  short player_lives() const;
  short player_bullets() const;

private:
  // add enemy with the given id to exaclty one room in a map
  void add_enemy_to_room(unsigned int enemy_index);

  // get rooms where player is right now (at most 2)
  void update_active_rooms();
  // update collision detector with enemy and objects in active rooms
  void update_collision_detector();

  void render_map();
  void render_player();
  void render_enemies();
  // render to texture is used to check if enemy is shot (3d mouse picking)
  void render_to_texture_map();
  void render_to_texture_enemies();

  // culling is used to avoid sending objects to GPU for rendering if they
  // cannot be seen
  void culling();

public:
  const Map m_map;
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
  // enemies that shoud be rendered
  std::vector<unsigned int> m_enemies_to_render;

  // objects used for rendering
  const Light light{glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                    glm::vec3(0.0f, 0.5f, 0.0f)};

  // ------------ shaders --------------------
  Shader skinned_mesh_no_light_shader{
      "../res/shaders/skinned_mesh.vert",
      "../res/shaders/skinned_mesh_no_light.frag"};

  Shader skinned_mesh_shader{"../res/shaders/skinned_mesh.vert",
                             "../res/shaders/skinned_mesh.frag"};

  Shader bounding_box_shader{"../res/shaders/bounding_box.vert",
                             "../res/shaders/bounding_box.frag"};

  // Shader default_shader{"../res/shaders/default.vert",
  //                       "../res/shaders/default.frag"};

  Shader picking_shader{"../res/shaders/picking.vert",
                        "../res/shaders/picking.frag"};

  Shader picking_primitive_shader{"../res/shaders/picking_primitive.vert",
                                  "../res/shaders/picking_primitive.frag"};
};

#endif /* _LEVEL_MANAGER_H_ */
