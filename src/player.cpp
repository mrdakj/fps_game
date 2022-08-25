#include "player.h"
#include "aabb.h"
#include "animated_mesh.h"
#include "bounding_box.h"
#include "camera.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <vector>

#include <iostream>

Player::Player(Camera &camera)
    : AnimatedMesh("../res/models/fps_pistol/fps_pistol.gltf"),
      m_camera(camera) {
  auto [animation_finised, global_transormation] =
      m_skinned_mesh.get_bones_for_animation("shoot", 0);
  set_global_transformation(std::move(global_transormation));
  set_user_scaling();
  set_user_rotation();
  set_user_translation();
}

void Player::render(Shader &shader, Shader &bounding_box_shader,
                    const Light &light) const {
  AnimatedMesh::render(shader, m_camera, light);

#ifdef FPS_DEBUG
  // for testing only
  AnimatedMesh::render_boxes(bounding_box_shader, m_camera);
#endif
}

void Player::set_orientation(glm::vec3 orientation) {
  m_camera.orientation() = std::move(orientation);
  // when updating camera orientation both player rotation and translation
  // matrices need to be updated because position depends on camera down vector
  // that gets changed during camera rotation
  set_user_rotation();
  set_user_translation();
}

void Player::update_position(glm::vec3 delta_position) {
  m_camera.position() += std::move(delta_position);
  // only update player position, there is no need to update players rotation
  set_user_translation();
}

void Player::set_position(glm::vec3 position) {
  m_camera.position() = std::move(position);
  // only update player position, there is no need to update players rotation
  set_user_translation();
}

void Player::set_user_scaling() {
  m_scaling = glm::scale(glm::mat4(1.0f), glm::vec3(0.01, 0.01, 0.01));
  // m_scaling = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, 1));
  AnimatedMesh::set_user_transformation(m_translation * m_rotation * m_scaling);
}

void Player::set_user_rotation() {
  auto angleXZ =
      glm::orientedAngle(glm::vec3(0, 0, 1),
                         glm::normalize(glm::vec3(m_camera.orientation().x, 0,
                                                  m_camera.orientation().z)),
                         glm::vec3(0, 1, 0));
  auto rotXZ = glm::rotate(glm::mat4(1.0), angleXZ, glm::vec3(0, 1, 0));

  auto angleUp =
      glm::orientedAngle(glm::normalize(glm::vec3(m_camera.orientation().x, 0,
                                                  m_camera.orientation().z)),
                         glm::normalize(m_camera.orientation()),
                         glm::cross(m_camera.orientation(), m_camera.up()));
  auto rotUp = glm::rotate(glm::mat4(1.0), angleUp,
                           glm::cross(m_camera.orientation(), m_camera.up()));

  m_rotation = rotUp * rotXZ;
  // m_rotation = glm::mat4(1.0f);
  AnimatedMesh::set_user_transformation(m_translation * m_rotation * m_scaling);
}

void Player::set_user_translation() {
  auto downVector =
      glm::rotate(m_camera.orientation(), glm::radians(-90.0f),
                  glm::cross(m_camera.orientation(), m_camera.up()));

  m_translation = glm::translate(
      glm::mat4(1.0),
      m_camera.position() + 0.75f * m_camera.orientation() + 0.3f * downVector);
  // m_translation = glm::translate(glm::mat4(1.0), glm::vec3(0, 1.4, -3));

  AnimatedMesh::set_user_transformation(m_translation * m_rotation * m_scaling);
}
