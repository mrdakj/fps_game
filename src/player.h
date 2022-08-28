#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "animated_mesh.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "cursor.h"
#include "light.h"
#include "shader.h"
#include "skinned_mesh.h"
#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <string>

class Player : public AnimatedMesh {
  friend class PlayerController;

public:
  enum class Action { Shoot, Recharge, TestAll, None };

  Player(Camera &camera);

  void reset();

  void render(Shader &shader, Shader &bounding_box_shader, const Light &light);

  void set_user_scaling();
  void set_user_rotation();
  void set_user_translation();

  void set_orientation(glm::vec3 orientation);
  void set_position(glm::vec3 position);
  void update_position(glm::vec3 delta_position);

  void shot();
  bool is_dead() const;

  bool can_shoot() const;
  void take_bullet();
  void recharge_gun();

  const Camera &camera() const { return m_camera; }

private:
  Camera &m_camera;

  glm::mat4 m_scaling;
  glm::mat4 m_rotation;
  glm::mat4 m_translation;

  // how many times player can get shot
  short m_lives;
  // how many bullets player has
  short m_bullets;

  // ongoing action
  Player::Action m_todo_action;

  Cursor m_cursor;
};

#endif /* _PLAYER_H_ */
