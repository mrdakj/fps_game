#ifndef _ENEMY_H_
#define _ENEMY_H_

#include "animated_mesh.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "light.h"
#include "player.h"
#include "shader.h"
#include "skinned_mesh.h"
#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <string>

class Enemy : public AnimatedMesh {
public:
  Enemy(const Player &player);

  void set_idle();

  void set_shot();
  bool is_shot() const;

  glm::vec3 get_gun_direction() const;
  float get_player_angle() const;

  void render(Shader &shader, Shader &bounding_box_shader, const Camera &camera,
              const Light &light) override;

private:
  bool m_is_shot = false;
  const Player& m_player;
};

#endif /* _ENEMY_H_ */
