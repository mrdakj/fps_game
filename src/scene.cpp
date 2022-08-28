#include "scene.h"

Scene::Scene(GLFWwindow *window, unsigned int window_width,
             unsigned int window_height)
    : m_window_width(window_width),
      m_window_height(window_height), m_input_controller{window},
      m_level_manager(window, window_width, window_height),
      m_picking_texture(window_width, window_height) {}

PickingTexture::PixelInfo Scene::process_mouse_click() {
  PickingTexture::PixelInfo pixel;
  auto [mouse_x, mouse_y] = m_input_controller.get_mouse_position();
  pixel = m_picking_texture.read_pixel(mouse_x, m_window_height - mouse_y - 1);
  if (m_level_manager.is_enemy_shot(pixel.object_id)) {
    m_level_manager.set_enemy_shot(pixel.object_id);
  }
  return pixel;
}

void Scene::reset() { m_level_manager.reset(); }

bool Scene::is_game_over() const { return m_level_manager.is_player_dead(); }

void Scene::update(float current_time) { m_level_manager.update(current_time); }

void Scene::render() {
  PickingTexture::PixelInfo pixel;
  if (!is_game_over() &&
      m_input_controller.is_mouse_button_pressed(MouseButton::Left)) {
    render_to_texture();
    pixel = process_mouse_click();
  }
  render_scene(pixel);
}

void Scene::render_scene(const PickingTexture::PixelInfo &pixel) {
  // clear the back buffer and assign the new color to it
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef FPS_DEBUG
  if (pixel.is_set()) {
    m_level_manager.render_primitive(pixel.object_id, pixel.draw_id,
                                     pixel.primitive_id);
  }
#endif

  m_level_manager.render();
  if (!is_game_over()) {
    m_cursor.render();
  }
}

void Scene::render_to_texture() {
  m_picking_texture.enable_writing();
  // clear the back buffer and assign the new color to it
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  m_level_manager.render_to_texture();
  m_picking_texture.disable_writing();
}
