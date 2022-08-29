#ifndef _SCENE_H_
#define _SCENE_H_

#include "cursor.h"
#include "enemy.h"
#include "input_controller.h"
#include "level_manager.h"
#include "menu.h"
#include "picking_texture.h"
#include <GLFW/glfw3.h>

class Game {
public:
  Game(GLFWwindow *window, unsigned int window_width,
       unsigned int window_height);

  void update(float current_time);
  void render();

  bool exit() const { return m_exit; }

private:
  void play();
  void reset();

  PickingTexture::PixelInfo process_mouse_click();

  void render_game(const PickingTexture::PixelInfo &pixel);
  void render_to_texture();

  bool is_game_over() const;

  void update_frame_rate(float current_time);

private:
  unsigned int m_window_width;
  unsigned int m_window_height;
  InputController m_input_controller;
  PickingTexture m_picking_texture;
  LevelManager m_level_manager;

  Menu::GameState m_game_state;
  Menu m_menu;

  bool m_exit;

  short m_frame_rate;
  short m_frame_count;
  float m_previous_frame_time;
};

#endif /* _SCENE_H_ */
