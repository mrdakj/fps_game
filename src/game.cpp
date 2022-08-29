#include "game.h"
#include "menu.h"

Game::Game(GLFWwindow *window, unsigned int window_width,
           unsigned int window_height)
    : m_window_width(window_width),
      m_window_height(window_height), m_input_controller{window},
      m_level_manager(window, window_width, window_height),
      m_picking_texture(window_width, window_height),
      m_game_state(Menu::GameState::NotStarted),
      m_menu(window, window_width, window_height), m_exit(false),
      m_previous_frame_time(-1), m_frame_rate(0), m_frame_count(0) {}

PickingTexture::PixelInfo Game::process_mouse_click() {
  PickingTexture::PixelInfo pixel;
  auto [mouse_x, mouse_y] = m_input_controller.get_mouse_position();
  pixel = m_picking_texture.read_pixel(mouse_x, m_window_height - mouse_y - 1);
  if (m_level_manager.is_enemy_shot(pixel.object_id)) {
    m_level_manager.set_enemy_shot(pixel.object_id);
  }
  return pixel;
}

void Game::play() {
  if (m_game_state != Menu::GameState::NotStarted) {
    // reset game if game was played before
    reset();
  }
  m_game_state = Menu::GameState::Running;
}

void Game::reset() {
  m_level_manager.reset();
  m_previous_frame_time = -1;
  m_frame_rate = 0;
  m_frame_count = 0;
}

bool Game::is_game_over() const { return m_level_manager.is_player_dead(); }

void Game::update_frame_rate(float current_time) {
  m_frame_count++;
  if (m_previous_frame_time == -1) {
    m_previous_frame_time = current_time;
  }
  // if a second has passed.
  if (current_time - m_previous_frame_time >= 1.0) {
    m_frame_rate = m_frame_count;
    m_frame_count = 0;
    m_previous_frame_time = current_time;
  }
}

void Game::update(float current_time) {
  update_frame_rate(current_time);

  if (m_game_state != Menu::GameState::NotStarted) {
    m_level_manager.update(current_time);
  }
  if (is_game_over()) {
    m_game_state = Menu::GameState::Over;
  }

  switch (m_menu.update({m_game_state, m_level_manager.player_lives(),
                         m_level_manager.player_bullets(), m_frame_rate})) {
  case Menu::Result::Exit:
    m_exit = true;
    break;
  case Menu::Result::Play: {
    play();
    break;
  }
  case Menu::Result::None:
    break;
  }
}

void Game::render() {
  PickingTexture::PixelInfo pixel;
  if (!is_game_over() && m_level_manager.player_shoot_started() &&
      m_input_controller.is_mouse_button_pressed(MouseButton::Left)) {
    render_to_texture();
    pixel = process_mouse_click();
  }
  render_game(pixel);
  m_menu.render();
}

void Game::render_game(const PickingTexture::PixelInfo &pixel) {
  // clear the back buffer and assign the new color to it
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef FPS_DEBUG
  if (pixel.is_set()) {
    m_level_manager.render_primitive(pixel.object_id, pixel.draw_id,
                                     pixel.primitive_id);
  }
#endif

  m_level_manager.render();
}

void Game::render_to_texture() {
  m_picking_texture.enable_writing();
  // clear the back buffer and assign the new color to it
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  m_level_manager.render_to_texture();
  m_picking_texture.disable_writing();
}
