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

  Menu(GLFWwindow *window, unsigned int window_width,
       unsigned int window_height);
  ~Menu();

  Result update(bool game_over) const;
  void render() const;

private:
  void spacing(float space_x, float space_y) const;
  float label_size(const char *label) const;
  void align(const char *label, float element_size) const;
  void text_aligned(const char *label) const;
  bool button_aligned(const char *label) const;
  bool button_aligned(const char *label, float element_size) const;

private:
  unsigned int m_width;
  unsigned int m_height;
};

#endif /* _MENU_H_ */
