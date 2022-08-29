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
#include "game.h"
#include "input_controller.h"
#include "level_manager.h"
#include "light.h"
#include "map.h"
#include "menu.h"
#include "picking_texture.h"
#include "player.h"
#include "player_controller.h"
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

  Game game(window, WINDOW_WIDTH, WINDOW_HEIGHT);

  while (!glfwWindowShouldClose(window) && !game.exit()) {
    game.update(glfwGetTime());
    game.render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
