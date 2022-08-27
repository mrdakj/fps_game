#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

#include "aabb.h"
#include "animated_mesh.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_detector.h"
#include "cursor.h"
#include "enemy.h"
#include "enemy_behavior_tree.h"
#include "input_controller.h"
#include "level_manager.h"
#include "light.h"
#include "map.h"
#include "picking_texture.h"
#include "player.h"
#include "player_controller.h"
#include "shader.h"
#include "sound.h"
#include <GLFW/glfw3.h>

const int window_width = 2080;
const int window_height = 1000;

struct Scene {
  Scene(GLFWwindow *window)
      : input_controller{window},
        level_manager(window, window_width, window_height) {}

  PickingTexture::PixelInfo process_mouse_click() {
    PickingTexture::PixelInfo pixel;
    auto [mouse_x, mouse_y] = input_controller.get_mouse_position();
    pixel = picking_texture.read_pixel(mouse_x, window_height - mouse_y - 1);
    if (level_manager.is_enemy_shot(pixel.object_id)) {
      level_manager.set_enemy_shot(pixel.object_id);
    }
    return pixel;
  }

  void update(float current_time) { level_manager.update(current_time); }

  void render() {
    PickingTexture::PixelInfo pixel;
    if (input_controller.is_mouse_button_pressed(MouseButton::Left)) {
      render_to_texture();
      pixel = process_mouse_click();
    }
    render_scene(pixel);
  }

  void render_scene(const PickingTexture::PixelInfo &pixel) {
    // clear the back buffer and assign the new color to it
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef FPS_DEBUG
    if (pixel.is_set()) {
      level_manager.render_primitive(pixel.object_id, pixel.draw_id,
                                     pixel.primitive_id);
    }
#endif

    level_manager.render();
    cursor.render();
  }

  void render_to_texture() {
    picking_texture.enable_writing();
    // clear the back buffer and assign the new color to it
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    level_manager.render_to_texture();
    picking_texture.disable_writing();
  }

  Cursor cursor;
  InputController input_controller;
  PickingTexture picking_texture{window_width, window_height};
  LevelManager level_manager;
};

int main(void) {
  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(window_width, window_height, "My window", NULL, NULL);

  if (window == NULL) {
    std::cout << "Failed to create a window." << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetCursorPos(window, window_width / 2, window_height / 2);

  glfwSwapInterval(1);

  if (glewInit() != GLEW_OK) {
    std::cout << "error initializing glew" << std::endl;
    return -1;
  }

  glViewport(0, 0, window_width, window_height);

  // specify the color used during glclear
  glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  Scene scene(window);

  double previous_time = glfwGetTime();
  int frame_count = 0;

  while (!glfwWindowShouldClose(window)) {
    double current_time = glfwGetTime();
    frame_count++;
    // if a second has passed.
    if (current_time - previous_time >= 1.0) {
      std::cout << frame_count << std::endl;

      frame_count = 0;
      previous_time = current_time;
    }

    scene.update(current_time);
    scene.render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
