#ifndef _INPUT_CONTROLLER_H_
#define _INPUT_CONTROLLER_H_

#include <GLFW/glfw3.h>
#include <iostream>

enum class MouseButton {
  Left = GLFW_MOUSE_BUTTON_LEFT,
  Right = GLFW_MOUSE_BUTTON_RIGHT,
  Middle = GLFW_MOUSE_BUTTON_MIDDLE
};

class InputController {
public:
  InputController(GLFWwindow *window) : m_window(window) {}

  virtual void process_inputs(float delta_time) {}
  virtual void process_inputs_keyboard(float delta_time) {}
  virtual void process_inputs_mouse(float delta_time) {}

  bool is_key_pressed(unsigned int key) const {
    return glfwGetKey(m_window, key) == GLFW_PRESS;
  }

  bool is_mouse_button_pressed(MouseButton button) const {
    return glfwGetMouseButton(
               m_window, static_cast<std::underlying_type<MouseButton>::type>(
                             button)) == GLFW_PRESS;
  }

  bool is_mouse_button_released(MouseButton button) const {
    return glfwGetMouseButton(
               m_window, static_cast<std::underlying_type<MouseButton>::type>(
                             button)) == GLFW_RELEASE;
  }

  std::pair<double, double> get_mouse_position() const {
    double mouse_x;
    double mouse_y;
    glfwGetCursorPos(m_window, &mouse_x, &mouse_y);
    return {mouse_x, mouse_y};
  }

  void set_mouse_position(double mouse_x, double mouse_y) const {
    glfwSetCursorPos(m_window, mouse_x, mouse_y);
  }

private:
  GLFWwindow *m_window;
};

#endif /* _INPUT_CONTROLLER_H_ */
