#ifndef _ENEMY_H_
#define _ENEMY_H_

#include "animated_mesh.h"
#include "light.h"
#include "skinned_mesh.h"
#include <glm/ext/vector_float3.hpp>
#include <string>

#include "enemy_behavior_tree.h"
#include "enemy_state_machine.h"
#include "timer.h"

class LevelManager;
class EnemyBT;

class Enemy : public AnimatedMesh {
public:
  enum class Aiming : int { Left = -1, Right = 1, UnderAim = 0 };

  // -------------------- constructors ------------------------------
  Enemy(LevelManager &level_manager,
        const glm::vec3 &position = glm::vec3(0.0f), float degreesXZ = 0);
  // copy constructor needed to create a new state machine and behavior tree
  Enemy(const Enemy &other);
  // ------------------------------------------------------------------

  void reset(const glm::vec3 &position = glm::vec3(0.0f), float degreesXZ = 0);

  // -------------------- transformation --------------------------------
  // set enemy's position and orientation
  void set_transformation(const glm::vec3 &position = glm::vec3(0.0f),
                          float degreesXZ = 0);
  // rotate current transformation for delta degrees in XZ plane around its
  // origin
  void rotate_transformation(float delta_degrees_XZ);
  // --------------------------------------------------------------------

  // --------------- enemy's parts in world system -----------------
  //            (after all transformations are applied)
  // get enemy's position in world space
  glm::vec3 get_position() const;
  // get gun's origin and direction in world space
  std::pair<glm::vec3, glm::vec3> get_gun_direction() const;
  // get enemy's font part position direction in world space
  std::pair<glm::vec3, glm::vec3> get_front_direction() const;
  // ---------------------------------------------------------------

  // ---------------------- animated mesh interface ---------------------------
  // set enemy's bones to be in some position
  void set_bones_position(StateMachine::Position position);

  // create "transition" animation in animated mesh that transforms it from
  // a current position to the first frame of the given animation/position
  // ignore spine bone if needed to control it manually (during attacking state)
  void create_transition_animation(const std::string &animation_name,
                                   float animation_duration,
                                   const std::string &bone_to_ignore = "");

  void render(Shader &shader, Shader &effects_shader,
              Shader &bounding_box_shader, const Camera &camera,
              const Light &light) const;
  // --------------------------------------------------------------------------

  // ------------------ spine control -----------------------------
  // control spine bone in order to follow a moving target (player)
  void rotate_spine(float delta_time);
  // check if spine can be rotated
  bool can_rotate_spine(bool left) const;
  // --------------------------------------------------------------

  // ------------------ state machine interface -----------------------
  // change state from current to the given state and return true when
  // transition is done
  bool change_state(StateMachine::StateName state_name);

  // return "success" if action is done, "running" if it exists but it is not
  // yet done, and null if it doesn't exit in todo actions
  std::optional<StateMachine::ActionStatus>
  get_action_status(StateMachine::Action action) const;
  // create new todo action (only allowed when not transitioning)
  void register_todo_action(StateMachine::Action action);
  // remove existing todo action
  void remove_todo_action(StateMachine::Action action);

  // set path in Patrolling state
  bool find_path();

  // make enemy as shot
  void set_shot();
  // check if enemy is shot
  bool is_shot() const;

  bool is_shooting() const;
  void stop_shooting();
  void start_shooting();

  void set_player_seen();
  bool is_player_seen() const;
  // how many seconds passed since the last time player was seen
  unsigned int player_seen_seconds_passed() const;
  // ------------------------------------------------------------------

  // return true if player is visible
  bool is_player_visible() const;

  // return UnderAim, Left or Right depending on gun-player angle
  Aiming get_aim() const;

  void shoot_player();

  bool is_player_dead() const;

  unsigned int id() const;

  // update enemy's state
  void update(float current_time);

  // --------------------- private methods --------------------------
private:
  // get enemy's left eye position and direction in world space
  std::pair<glm::vec3, glm::vec3> get_eye_direction() const;
  // get direction between enemy's left eye and player's position
  std::pair<glm::vec3, glm::vec3> get_eye_player_direction() const;

  // render gun direction for testing
  void render_gun_direction(Shader &bounding_box_shader,
                            const Camera &camera) const;

  // render left eye looking direction for testing
  void render_eye_direction(Shader &bounding_box_shader,
                            const Camera &camera) const;

  // render box from enemy's left eye to player's position for testing
  void render_eye_player_direction(Shader &bounding_box_shader,
                                   const Camera &camera) const;

  float get_spine_angle() const;
  float get_delta_spine_angle(float delta_time) const;

  // get angle between gun and player
  float get_aiming_angle() const;

  // return true if in attacking state or transitioning to attacking state
  bool attacking() const;

  // return true if player is close
  bool is_player_close(unsigned int threshold) const;

  float get_player_distance() const;

  bool is_player_shot() const;

  void init_cache();
  // ----------------------------------------------------------------

public:
  static bool is_target_shot(float distance, float max_distance);

  // ---------------- static vars -------------------
  static const std::string LEFT_EYE_BONE;
  static const std::string SPINE_BONE;
  static const std::string GUN;
  static const glm::vec3 FRONT_DIRECTION;
  static const float SCALING_FACTOR;
  static const std::string FLASH;
  // -----------------------------------------------

  // ---------------- member vars -----------------------
private:
  static AnimatedMesh &get_animated_mesh_instance();
  static unsigned int get_id();

  unsigned int m_id;
  // enemy's state
  StateMachine m_state_machine;
  // enemy's behavior tree
  EnemyBT m_bt;
  // reference to level manager
  LevelManager &m_level_manager;
  // timer to update the state
  Timer m_timer;
  // how many times update is called
  unsigned int m_tick_count;
  // id of effect objects to render (gun flash)
  std::vector<unsigned int> m_effects_to_render;
  // ---------------- cache -----------------------------
  mutable std::pair<float, bool> m_spine_angle_cache;
  mutable bool m_under_aim_during_chasing;
  // ----------------------------------------------------
};

#endif /* _ENEMY_H_ */
