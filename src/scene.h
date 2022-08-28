#ifndef _SCENE_H_
#define _SCENE_H_

#include "cursor.h"
#include "enemy.h"
#include "input_controller.h"
#include "level_manager.h"
#include "picking_texture.h"
#include <GLFW/glfw3.h>

class Scene {
public:
  Scene(GLFWwindow *window, unsigned int window_width,
        unsigned int window_height);

  void reset();
  bool is_game_over() const;
  void update(float current_time);
  void render();

private:
  PickingTexture::PixelInfo process_mouse_click();

  void render_scene(const PickingTexture::PixelInfo &pixel);
  void render_to_texture();

private:
  unsigned int m_window_width;
  unsigned int m_window_height;
  InputController m_input_controller;
  PickingTexture m_picking_texture;
  LevelManager m_level_manager;
};

#endif /* _SCENE_H_ */
