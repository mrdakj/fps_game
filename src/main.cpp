#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_detector.h"
#include "cursor.h"
#include "enemy.h"
#include "enemy_controller.h"
#include "input_controller.h"
#include "light.h"
#include "picking_texture.h"
#include "player.h"
#include "player_controller.h"
#include "room.h"
#include "shader.h"
#include <GLFW/glfw3.h>

const int window_width = 2080;
const int window_height = 1000;

struct Scene {
  Scene(GLFWwindow *window)
      : enemy(player), collision_detector{{&player, &enemy}, {&room}},
        enemy_controller{enemy, collision_detector},
        player_controller{player, collision_detector, window}, input_controller{
                                                                   window} {}

  void update(float current_time) {
    player_controller.update(current_time);
    enemy_controller.update(current_time);
    camera.update_matrix(45.0f, 0.1f, 300.0f);
  }

  void process_mouse_click() {
    PickingTexture::PixelInfo pixel;
    auto [mouse_x, mouse_y] = input_controller.get_mouse_position();
    pixel = picking_texture.read_pixel(mouse_x, window_height - mouse_y - 1);
    // std::cout << pixel.object_id << "," << pixel.draw_id << ","
    //           << pixel.primitive_id << std::endl;
    if (pixel.object_id == 1) {
      enemy.set_shot();
    }
  }

  void render(float current_time) {
    update(current_time);

    PickingTexture::PixelInfo pixel;
    if (input_controller.is_mouse_button_pressed(MouseButton::Left)) {
      render_to_texture();
      process_mouse_click();
    }
    render_scene(current_time, pixel);
  }

  void render_scene(float current_time,
                    const PickingTexture::PixelInfo &pixel) {
    // clear the back buffer and assign the new color to it
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    enemy.render(skinned_mesh_shader, bounding_box_shader, camera, light);
    player.render(skinned_mesh_shader, bounding_box_shader, light);

    room.render(basic_shader, camera, light);
    cursor.render();

    render_pixel_primitive(pixel);
  }

  void render_to_texture() {
    picking_texture.enable_writing();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    picking_shader.activate();
    picking_shader.set_uniform<unsigned int>("gObjectIndex", 1);
    enemy.render_to_texture(picking_shader, camera);

    picking_texture.disable_writing();
  }

  void render_pixel_primitive(const PickingTexture::PixelInfo &pixel) {
    if (pixel.object_id == 1) {
      enemy.render_primitive(picking_primitive_shader, camera, pixel.draw_id,
                             pixel.primitive_id);
    }
  }

  Shader basic_shader{"../res/shaders/default.vert",
                      "../res/shaders/default.frag"};
  Shader skinned_mesh_shader{"../res/shaders/skinned_mesh.vert",
                             "../res/shaders/skinned_mesh.frag"};
  Shader bounding_box_shader{"../res/shaders/bounding_box.vert",
                             "../res/shaders/bounding_box.frag"};
  Shader picking_shader{"../res/shaders/picking.vert",
                        "../res/shaders/picking.frag"};
  Shader picking_primitive_shader{"../res/shaders/picking_primitive.vert",
                                  "../res/shaders/picking_primitive.frag"};

  Cursor cursor;

  Camera camera{window_width, window_height, glm::vec3(0.0f, 5.0f, -10.0f)};
  Light light{glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f)};

  Room room{50, 50, 10, glm::vec3(-25, 0, 0)};

  Player player{camera};
  Enemy enemy;
  EnemyController enemy_controller;

  CollistionDetector collision_detector;
  PlayerController player_controller;

  InputController input_controller;
  PickingTexture picking_texture{window_width, window_height};
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
      // std::cout << frame_count << std::endl;

      frame_count = 0;
      previous_time = current_time;
    }

    scene.render(current_time);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
