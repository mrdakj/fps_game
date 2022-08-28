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
#include "menu.h"
#include "picking_texture.h"
#include "player.h"
#include "player_controller.h"
#include "scene.h"
#include "shader.h"
#include "sound.h"
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH (2080)
#define WINDOW_HEIGHT (1000)

int main(void) {
  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "FPS Game", NULL, NULL);

  if (window == NULL) {
    std::cout << "Failed to create a window." << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

  glfwSwapInterval(1);

  if (glewInit() != GLEW_OK) {
    std::cout << "error initializing glew" << std::endl;
    return -1;
  }

  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  // specify the color used during glclear
  glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  Scene scene(window, WINDOW_WIDTH, WINDOW_HEIGHT);
  Menu menu(window, WINDOW_WIDTH, WINDOW_HEIGHT);

  bool game_started = false;

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

    if (game_started) {
      scene.update(current_time);
      scene.render();
    }

    bool game_over = scene.is_game_over();
    if (!game_started || game_over) {
      Menu::Result menu_result = menu.update(game_over);
      if (menu_result == Menu::Result::Exit) {
        break;
      } else if (menu_result == Menu::Result::Play) {
        if (game_started) {
          scene.reset();
        } else {
          game_started = true;
        }
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
      }
      menu.render();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
