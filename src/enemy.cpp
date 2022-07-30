#include "enemy.h"
#include "aabb.h"
#include "animated_mesh.h"
#include "bounding_box.h"
#include "camera.h"
#include "player.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

#include <iostream>

const std::string IDLE_POSITION = "idle";
const std::string LEFT_EYE_BONE = "swat:LeftEye_010";
const std::string SPINE_BONE = "swat:Spine_02";
const std::string GUN = "gun";

Enemy::Enemy(const Player &player)
    : AnimatedMesh("../res/models/enemy/enemy.gltf"), m_player(player) {
  auto scaling = glm::scale(glm::mat4(1.0f), glm::vec3(0.01, 0.01, 0.01));
  auto rotation = glm::mat4(1.0f);
  auto translation = glm::translate(glm::mat4(1.0), glm::vec3(2, 0.1, -2));
  AnimatedMesh::set_user_transformation(translation * rotation * scaling);

  set_idle();
  AnimatedMesh::merge_user_and_global_transformations();
}

void Enemy::set_idle() {
  auto global_transformation =
      m_skinned_mesh.get_bones_for_position(IDLE_POSITION);
  AnimatedMesh::set_global_transformation(std::move(global_transformation));
}

void Enemy::set_shot() { m_is_shot = true; }

bool Enemy::is_shot() const { return m_is_shot; }

glm::vec3 Enemy::get_gun_direction() const {
  const auto &gun_node_global_transformation =
      m_skinned_mesh.node_global_transformation("gun");
  glm::vec3 gun_O = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(0, 0, 0, 1);
  glm::vec3 gun_X = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(1, 0, 0, 1);
  return gun_X - gun_O;
}

float Enemy::get_eye_player_angle() const {
  const auto &left_eye_node_global_transformation =
      m_skinned_mesh.node_global_transformation("swat:LeftEye_010");
  glm::vec3 eye_O = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(0, 0, 0, 1);
  glm::vec3 eye_Z = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(0, 0, 1, 1);
  auto eye_direction = -(eye_Z - eye_O);
  eye_direction[1] = 0;
  auto player_position = m_player.camera().position();
  auto player_direction = player_position - eye_O;
  player_direction[1] = 0;
  eye_direction = glm::normalize(eye_direction);
  player_direction = glm::normalize(player_direction);
  auto cross_product = glm::cross(eye_direction, player_direction);
  float angle = (cross_product[1] > 0 ? 1 : -1) *
                glm::degrees(glm::angle(eye_direction, player_direction));
  return angle;
}

float Enemy::get_gun_player_angle() const {
  const auto &gun_node_global_transformation =
      m_skinned_mesh.node_global_transformation(GUN);
  glm::vec3 gun_O = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(0, 0, 0, 1);
  glm::vec3 gun_X = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(1, 0, 0, 1);
  auto gun_direction = gun_X - gun_O;
  gun_direction[1] = 0;
  auto player_position = m_player.camera().position();
  auto player_direction = player_position - gun_O;
  player_direction[1] = 0;
  gun_direction = glm::normalize(gun_direction);
  player_direction = glm::normalize(player_direction);
  auto cross_product = glm::cross(gun_direction, player_direction);
  float angle = (cross_product[1] > 0 ? 1 : -1) *
                glm::degrees(glm::angle(gun_direction, player_direction));
  return angle;
}

void Enemy::render(Shader &shader, Shader &bounding_box_shader,
                   const Camera &camera, const Light &light) const {
  AnimatedMesh::render(shader, bounding_box_shader, camera, light);

  // for testing to visualize a gun direction
  // const auto &gun_node_global_transformation =
  //     m_skinned_mesh.node_global_transformation(GUN);

  // glm::vec3 gun_O = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(0, 0, 0, 1);

  // glm::vec3 gun_X = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(1, 0, 0, 1);

  // glm::vec3 gun_Y = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(0, 1, 0, 1);
  // glm::vec3 gun_Z = AnimatedMesh::final_transformation() *
  //                   gun_node_global_transformation * glm::vec4(0, 0, 1, 1);

  // BoundingBox(
  //     {100.0f * (get_gun_direction()), (gun_Y - gun_O), (gun_Z - gun_O)},
  //     gun_O) .render(bounding_box_shader, camera, glm::vec3(1.0, 0.0, 0.0));

  // const auto &left_eye_node_global_transformation =
  //     m_skinned_mesh.node_global_transformation(LEFT_EYE_BONE);
  // glm::vec3 eye_O = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(0, 0, 0,
  //                   1);
  // glm::vec3 eye_X = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(1, 0, 0,
  //                   1);
  // glm::vec3 eye_Y = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(0, 1, 0,
  //                   1);
  // glm::vec3 eye_Z = AnimatedMesh::final_transformation() *
  //                   left_eye_node_global_transformation * glm::vec4(0, 0, 1,
  //                   1);
  // BoundingBox(
  //     {(eye_X-eye_O), (eye_Y - eye_O), -100.0f * (eye_Z - eye_O)}, eye_O)
  //     .render(bounding_box_shader, camera, glm::vec3(0.0, 1.0, 0.0));
}

void Enemy::rotate(float degrees) {
  // rotate around spine's y axis which is up axis
  m_skinned_mesh.set_node_transformation(
      SPINE_BONE, glm::rotate(glm::radians(degrees), glm::vec3(0, 1, 0)));
}
