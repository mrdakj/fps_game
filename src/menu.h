#ifndef _MENU_H_
#define _MENU_H_

#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"
#include <imgui.h>
#include <imgui_internal.h>

#include <iostream>

class Menu {
public:
  enum class Result { Play, Exit, None };
  enum class GameState { NotStarted, Running, Over };

  struct State {
    GameState game_state;
    short lives;
    short bullets;
    short frame_rate;
  };

  Menu(GLFWwindow *window, unsigned int window_width,
       unsigned int window_height);
  ~Menu();

  Result update(State state);
  void render() const;

private:
  Result draw_menu() const;
  void draw_text() const;

  void spacing(float space_x, float space_y) const;
  float label_size(const char *label) const;
  float button_size(const char *label) const;
  void align(const char *label, float element_size) const;
  void text_aligned(const char *label) const;
  bool button_aligned(const char *label) const;
  bool button_aligned(const char *label, float element_size) const;

private:
  unsigned int m_width;
  unsigned int m_height;

  State m_state;
};

#endif /* _MENU_H_ */
