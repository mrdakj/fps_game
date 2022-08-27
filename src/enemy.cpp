#include "enemy.h"

#include "animated_mesh.h"
#include "bounding_box.h"
#include "level_manager.h"
#include "player.h"

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <iostream>

// -------------- static vars ----------------------------
const std::string Enemy::LEFT_EYE_BONE = "swat:LeftEye_010";
const std::string Enemy::SPINE_BONE = "swat:Spine_02";
const std::string Enemy::GUN = "gun";
const std::string Enemy::FLASH = "flash";
// enemy is originally facing negative x-axis
const glm::vec3 Enemy::FRONT_DIRECTION = glm::vec3(-1, 0, 0);
// enemy's scale
const float Enemy::SCALING_FACTOR = 0.01;
// ------------------------------------------------------

// after how many ticks to update behavior tree
#define AI_REFRESH_INTERVAL (5)

#define SPINE_ANGLE_MIN (-60)
#define SPINE_ANGLE_MAX (40)

#define UNDER_AIM_THRESHOLD (5)

#define PLAYER_CLOSE_THRESHOLD (15)
#define PLAYER_VERY_CLOSE_THRESHOLD (3)

AnimatedMesh &Enemy::get_animated_mesh_instance() {
  // instantiated on first use
  static AnimatedMesh s_animated_mesh{"../res/models/enemy/enemy.gltf"};
  return s_animated_mesh;
}

unsigned int Enemy::get_id() {
  // instantiated on first use
  static unsigned int s_id = 0;
  unsigned int current_id = s_id++;
  return current_id;
}

Enemy::Enemy(const LevelManager &level_manger)
    : AnimatedMesh(Enemy::get_animated_mesh_instance()), m_id(Enemy::get_id()),
      m_level_manager(level_manger), m_state_machine(*this), m_bt(*this),
      m_tick_count(0),
      m_effects_to_render(m_skinned_mesh.get_render_object_ids(Enemy::FLASH)) {
  init_cache();

  auto scaling = glm::scale(glm::mat4(1.0f), glm::vec3(Enemy::SCALING_FACTOR));
  auto rotation = glm::mat4(1.0f);
  auto translation = glm::mat4(1.0f);
  AnimatedMesh::set_user_transformation(translation * rotation * scaling);
}

Enemy::Enemy(const Enemy &other)
    : AnimatedMesh(other), m_id(other.m_id),
      m_level_manager(other.m_level_manager),
      // it is important to create a new state
      m_state_machine(*this), m_bt(*this), m_tick_count(other.m_tick_count),
      m_effects_to_render(other.m_effects_to_render) {
  init_cache();
  AnimatedMesh::set_user_transformation(other.user_transformation());
}

void Enemy::set_transformation(const glm::vec3 &position, float degreesXZ) {
  // set enemy to the given position and rotate it for degreesXZ degrees in XZ
  // plane
  // keep the same scaling
  auto scaling = glm::scale(glm::mat4(1.0f), glm::vec3(Enemy::SCALING_FACTOR));
  // rotate in XZ plane for degreesXZ degrees
  auto rotation =
      glm::rotate(glm::mat4(1.0), glm::radians(degreesXZ), glm::vec3(0, 1, 0));
  // translate to the position
  auto translation = glm::translate(glm::mat4(1.0), position);
  AnimatedMesh::set_user_transformation(translation * rotation * scaling);
}

void Enemy::rotate_transformation(float delta_degrees_XZ) {
  // rotate current transformation for delta degrees around its origin
  // (position)

  //   |
  //   |            |
  //   |            |
  //   |            |
  //   |   position ___________   enemy's transformation
  //   |
  //   ____________________ world's system

  // 1. translate enemy's transformation to the origin
  // 2. rotate enemy's transformation for delta_degrees in XZ plane
  // 3. translate enemy's transformation back to the original position
  auto position = get_position();
  // translation matrix to move enemy's transformation to world's origin (0,0,0)
  auto translation_to_world_origin = glm::translate(glm::mat4(1.0), -position);
  // translation matrix to move enemy's transformation from world's origin
  // (0,0,0) to its original position
  // translation_to_world_origin inverse
  auto translation_back = glm::translate(glm::mat4(1.0), position);
  // rotation matrix to rotate system in XZ plane for delta_degrees_XZ degrees
  auto delta_rotation = glm::rotate(
      glm::mat4(1.0), glm::radians(delta_degrees_XZ), glm::vec3(0, 1, 0));

  AnimatedMesh::set_user_transformation(translation_back * delta_rotation *
                                        translation_to_world_origin *
                                        user_transformation());
}

glm::vec3 Enemy::get_position() const {
  // get enemy's current position in the world space
  return AnimatedMesh::final_transformation() * glm::vec4(0, 0, 0, 1);
}

std::pair<glm::vec3, glm::vec3> Enemy::get_gun_direction() const {
  // gun is initially positioned at the world's origin and oriented to point
  // in the positive x-axis direction

  // get gun's model transformation
  const auto &gun_node_global_transformation =
      m_skinned_mesh.node_global_transformation(Enemy::GUN);

  // find gun's position in the world space after its model transformation is
  // applied
  glm::vec3 gun_O = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(0, 0, 0, 1);
  // find transformation of one point on gun's pipe - (1, 0, 0)
  glm::vec3 gun_X = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(1, 0, 0, 1);

  // return gun's position and pipe direction
  return {gun_O, gun_X - gun_O};
}

std::pair<glm::vec3, glm::vec3> Enemy::get_eye_direction() const {
  //            h
  // z <--- eye e ------> -(eye_Z - eye_O) = looking direction
  //            a
  //            d

  // eye is initially positioned at the world origin and looks along negative
  // z-axis

  // get eye's model transformation
  const auto &left_eye_node_global_transformation =
      m_skinned_mesh.node_global_transformation(Enemy::LEFT_EYE_BONE);

  glm::vec3 eye_O = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(0, 0, 0, 1);
  glm::vec3 eye_Z = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(0, 0, 1, 1);

  // return eye's position and looking direction
  return {eye_O, -(eye_Z - eye_O)};
}

std::pair<glm::vec3, glm::vec3> Enemy::get_front_direction() const {
  // get enemy's position in world space
  glm::vec3 front_O = get_position();
  // get one point on enemy's front side (-1,0,0) in world space
  glm::vec3 front_X = AnimatedMesh::final_transformation() *
                      glm::vec4(Enemy::FRONT_DIRECTION, 1);

  // return enemy's position and it front direction (orientation vector)
  return {front_O, front_X - front_O};
}

std::pair<glm::vec3, glm::vec3> Enemy::get_eye_player_direction() const {
  // get eye-player direction
  // eye ---------------------> player
  //           ^ direction vector
  auto eye_O = get_eye_direction().first;
  const auto &player_position = m_level_manager.player_position();
  auto eye_player_direction = player_position - eye_O;
  // reduce size of vector for constant value 0.2
  // eye ------------------>    player
  //           ^ reduced direction vector
  auto eye_player_direction_reduced =
      eye_player_direction - 0.2f * glm::normalize(eye_player_direction);

  return {std::move(eye_O), std::move(eye_player_direction_reduced)};
}

void Enemy::set_bones_position(StateMachine::Position position) {
  m_skinned_mesh.get_bones_for_position(StateMachine::position_name(position));
}

void Enemy::create_transition_animation(const std::string &animation_name,
                                        float animation_duration,
                                        const std::string &bone_to_ignore) {
  // create "transition" animation in animated mesh that transforms it from
  // a current position to the first frame of the given animation/position
  // ignore spine bone if needed to control it manually (during attacking state)
  m_skinned_mesh.create_transition_animation(animation_name, animation_duration,
                                             bone_to_ignore);
}

void Enemy::render(Shader &shader, Shader &effects_shader,
                   Shader &bounding_box_shader, const Camera &camera,
                   const Light &light) const {
  // render everything but effect objects
  AnimatedMesh::render(shader, camera, light, m_effects_to_render,
                       true /* exclude */);

  if (is_shooting()) {
    // render only effect objects
    AnimatedMesh::render(effects_shader, camera, light, m_effects_to_render,
                         false /* exclude */);
  }

#ifdef FPS_DEBUG
  // for testing only
  AnimatedMesh::render_boxes(bounding_box_shader, camera);

  render_gun_direction(bounding_box_shader, camera);
  render_eye_direction(bounding_box_shader, camera);
  render_eye_player_direction(bounding_box_shader, camera);
#endif
}

void Enemy::render_gun_direction(Shader &bounding_box_shader,
                                 const Camera &camera) const {
  // for testing to visualize a gun direction
  const auto &gun_node_global_transformation =
      m_skinned_mesh.node_global_transformation(Enemy::GUN);

  auto [gun_O, gun_OX] = get_gun_direction();

  glm::vec3 gun_Y = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(0, 1, 0, 1);

  glm::vec3 gun_Z = AnimatedMesh::final_transformation() *
                    gun_node_global_transformation * glm::vec4(0, 0, 1, 1);

  BoundingBox({100.0f * gun_OX, (gun_Y - gun_O), (gun_Z - gun_O)}, gun_O)
      .render(bounding_box_shader, camera, glm::vec3(1.0, 0.0, 0.0));
}

void Enemy::render_eye_direction(Shader &bounding_box_shader,
                                 const Camera &camera) const {
  // for testing to visualize a left eye looking direction
  const auto &left_eye_node_global_transformation =
      m_skinned_mesh.node_global_transformation(Enemy::LEFT_EYE_BONE);

  auto [eye_O, eye_looking_direction] = get_eye_direction();

  glm::vec3 eye_X = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(1, 0, 0, 1);
  glm::vec3 eye_Y = AnimatedMesh::final_transformation() *
                    left_eye_node_global_transformation * glm::vec4(0, 1, 0, 1);

  BoundingBox(
      {(eye_X - eye_O), (eye_Y - eye_O), 100.0f * eye_looking_direction}, eye_O)
      .render(bounding_box_shader, camera, glm::vec3(0.0, 1.0, 0.0));
}

void Enemy::render_eye_player_direction(Shader &bounding_box_shader,
                                        const Camera &camera) const {
  // for testing to visualize direction between enemy's left eye and player's
  // position
  auto [eye_O, eye_player_direction] = get_eye_player_direction();

  auto eye_player_direction_axis_1 =
      glm::vec3(-eye_player_direction[2], 0, eye_player_direction[0]);

  auto eye_player_direction_axis_2 = glm::rotate(
      eye_player_direction, glm::radians(90.0f), eye_player_direction_axis_1);

  BoundingBox({eye_player_direction,
               0.2f * glm::normalize(eye_player_direction_axis_1),
               0.2f * glm::normalize(eye_player_direction_axis_2)},
              eye_O)
      .render(bounding_box_shader, camera, glm::vec3(0.0, 1.0, 0.0));
}

bool Enemy::is_player_visible() const {
  // player is visible if the following is true:
  // - player is not too far
  // - [eye, player] segment doesn't intersect any mesh box
  // - player is very close or angle between enemy's looking direction and
  // eye-player direction is less than 90

  if (!is_player_close(PLAYER_CLOSE_THRESHOLD)) {
    // player is too far
    return false;
  }

  // - player is not too far

  auto [eye_O, eye_player_direction] = get_eye_player_direction();

  if (is_player_close(PLAYER_VERY_CLOSE_THRESHOLD)) {
    // - player is very close

    // - [eye, player] segment doesn't intersect any mesh box
    return !m_level_manager.raycasting(eye_O, eye_O + eye_player_direction);
  }

  // - player is not very close

  auto eye_looking_direction = get_eye_direction().second;

  float eye_player_angle =
      // angle always returns positive value
      glm::degrees(glm::angle(glm::normalize(eye_looking_direction),
                              glm::normalize(eye_player_direction)));

  return
      // - angle between enemy's looking direction and eye-player direction is
      // less than 90
      eye_player_angle < 90 &&
      // - [eye, player] segment doesn't intersect any mesh box
      !m_level_manager.raycasting(eye_O, eye_O + eye_player_direction);
}

bool Enemy::is_player_close(unsigned int threshold) const {
  return glm::length2(get_position() - m_level_manager.player_position()) <
         threshold * threshold;
}

float Enemy::get_spine_angle() const {
  if (m_spine_angle_cache.second) {
    // if cache is valid return it
    return m_spine_angle_cache.first;
  }

  // bone points along y-axis
  // spine orientation is got by first rotating spine around y-axis (of parent's
  // system) then rotated it to point in some direction, after that spine is
  // translated (not important for finding the spine angle)
  // we want to find spine rotation angle around y-axis (XZ palne)

  //   y           y
  //   |           |
  //   |           |
  //   |           |
  //   |   spine_O -------- spine bone system
  //   |
  //   |
  // O ------------- spine's parent bone system

  // spine transformation in its parent bone space
  const auto &spine_local_transformation =
      m_skinned_mesh.node_local_transformation(Enemy::SPINE_BONE);

  // ----- spine's parent space ----
  glm::vec3 O = glm::vec3(0, 0, 0);
  glm::vec3 Y = glm::vec3(0, 1, 0);
  glm::vec3 X = glm::vec3(1, 0, 0);
  // -------------------------------

  // spine position in its parent system
  glm::vec3 spine_O = spine_local_transformation * glm::vec4(O, 1);
  // spine point on y-axis in its parent system
  glm::vec3 spine_Y = spine_local_transformation * glm::vec4(Y, 1);
  glm::vec3 spine_X = spine_local_transformation * glm::vec4(X, 1);
  auto spine_OY = spine_Y - spine_O;
  auto spine_OX = spine_X - spine_O;

  // find angle between parent's OY axis vector and spoine's OY vector in
  // parent's space
  float OY_angle = glm::angle(Y /* OY */, spine_OY);

  // find how spine's system would be oriented (translation is not important) if
  // it was not rotated around y-axis before its y-axis got transformed to point
  // in spine_OY direction (we are interested only in x-axis of such system
  // because we want to find rotation angle in XZ plane)
  // rotate x-axis for OY_angle angle around line trough origin that is
  // perpendicular to OY and spine_OY
  auto fixed_spine_X =
      glm::rotate(X, OY_angle, glm::cross(Y /* OY */, spine_OY));
  // fixed_spine_O = O

  // find oriented angle between non rotated spine's x-axis and spine's x-axis
  float angle = glm::degrees(glm::orientedAngle(
      // already normalized fixed_spine_OX
      fixed_spine_X,
      // already normalized
      spine_OX,
      // reference in order to determine sign for oriented angle
      glm::vec3(0, 1, 0)));

  // refresh a cache
  m_spine_angle_cache.first = angle;
  m_spine_angle_cache.second = true;

  return angle;
}

float Enemy::get_delta_spine_angle(float delta_time) const {
  Enemy::Aiming aim = get_aim();
  if (aim == Enemy::Aiming::UnderAim) {
    return 0;
  }

  float spine_angle = get_spine_angle();

  float delta_angle =
      (aim == Enemy::Aiming::Left) ? 100 * delta_time : -100 * delta_time;

  if (aim == Enemy::Aiming::Left) {
    if (spine_angle + delta_angle > SPINE_ANGLE_MAX) {
      delta_angle = SPINE_ANGLE_MAX - spine_angle;
    }
  } else {
    if (spine_angle + delta_angle < SPINE_ANGLE_MIN) {
      delta_angle = SPINE_ANGLE_MIN - spine_angle;
    }
  }

  return delta_angle;
}

bool Enemy::can_rotate_spine(bool left) const {
  float angle = get_spine_angle();
  // rotating left increases spine angle, while rotating right decreases it
  return left ? angle < SPINE_ANGLE_MAX : angle > SPINE_ANGLE_MIN;
}

bool Enemy::attacking() const {
  // return true if in attacking state or transitioning to attacking state
  return m_state_machine.in_state(StateMachine::StateName::Attacking) ||
         m_state_machine.transitioning_to_state(
             StateMachine::StateName::Attacking);
}

float Enemy::get_aiming_angle() const {
  // cannot get angle if player's position is unknown
  assert(is_player_seen() && "player was seen at some point");
  // get angle between gun pipe and [gun_O, player] in XZ plane
  auto [gun_O, gun_direction] = get_gun_direction();
  gun_direction[1] = 0;

  // get current player's position if in attacking state, otherwise get the last
  // seen player's position
  const auto &player_pos = attacking() ? m_level_manager.player_position()
                                       : m_state_machine.m_player_seen_position;

  auto gun_player_direction = player_pos - gun_O;
  gun_player_direction[1] = 0;

  return glm::degrees(glm::orientedAngle(
      glm::normalize(gun_direction), glm::normalize(gun_player_direction),
      // reference in order to determine sign for oriented angle
      glm::vec3(0, 1, 0)));
}

Enemy::Aiming Enemy::get_aim() const {
  // check cache
  if (m_under_aim_during_chasing) {
    return Enemy::Aiming::UnderAim;
  }

  float aiming_angle = get_aiming_angle();

  if (fabs(aiming_angle) < UNDER_AIM_THRESHOLD) {
    m_under_aim_during_chasing =
        m_state_machine.in_state(StateMachine::StateName::Chasing);
    return Enemy::Aiming::UnderAim;
  }

  return aiming_angle > 0 ? Enemy::Aiming::Left : Enemy::Aiming::Right;
}

void Enemy::rotate_spine(float delta_time) {
  float delta_angle = get_delta_spine_angle(delta_time);

  if (delta_angle != 0) {
    // update the cache
    m_spine_angle_cache.first += delta_angle;
    // rotate the spine bone
    m_skinned_mesh.rotate_bone(
        Enemy::SPINE_BONE,
        glm::rotate(glm::radians(delta_angle), glm::vec3(0, 1, 0)));
  }
}

std::optional<StateMachine::ActionStatus>
Enemy::get_action_status(StateMachine::Action action) const {
  return m_state_machine.get_action_status(action);
}

void Enemy::register_todo_action(StateMachine::Action action) {
  m_state_machine.register_todo_action(action);
}

void Enemy::remove_todo_action(StateMachine::Action action) {
  m_state_machine.remove_todo_action(action);
}

bool Enemy::find_path() {
  m_state_machine.set_path(m_level_manager.find_enemy_path(m_id));
  return true;
}

void Enemy::set_shot() { m_state_machine.m_is_shot = true; }
bool Enemy::is_shot() const { return m_state_machine.m_is_shot; }

void Enemy::set_player_seen() {
  m_state_machine.m_player_seen_time = std::time(nullptr);
  m_state_machine.m_player_seen_position = m_level_manager.player_position();
}

bool Enemy::is_player_seen() const {
  return m_state_machine.m_player_seen_time != 0;
}

unsigned int Enemy::player_seen_seconds_passed() const {
  assert(m_state_machine.m_player_seen_time != 0 &&
         "player was seen at some point");
  return std::time(nullptr) - m_state_machine.m_player_seen_time;
}

bool Enemy::change_state(StateMachine::StateName state_name) {
  // invalidate the cache
  init_cache();
  return m_state_machine.change_state(state_name);
}

void Enemy::update(float current_time) {
  // update enemy's state - change state or perform some todo action if exists
  if (++m_tick_count % AI_REFRESH_INTERVAL == 0) {
    m_bt.update();
  }
  m_state_machine.update(m_timer.tick(current_time));
}

void Enemy::init_cache() {
  m_spine_angle_cache = {0, false};
  m_under_aim_during_chasing = false;
}

bool Enemy::is_shooting() const { return m_state_machine.m_is_shooting; }
void Enemy::stop_shooting() { m_state_machine.m_is_shooting = false; }
void Enemy::start_shooting() { m_state_machine.m_is_shooting = true; }

unsigned int Enemy::id() const { return m_id; }
