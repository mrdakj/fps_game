# 1. Standardni paketi
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

# 2. DEFINICIJE - Dodajemo BEFORE da nateramo kompajler da prvo gleda tvoj vendor folder
# Ovo rešava problem sa /usr/include/imgui.h
include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")
include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image")

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# 3. IMGUI BIBLIOTEKA - Moraš uključiti sve ove .cpp fajlove!
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_demo.cpp
)

# 4. BACKENDS
add_library(imgui_impl_glfw STATIC vendor/imgui/backends/imgui_impl_glfw.cpp)
add_library(imgui_impl_opengl3 STATIC vendor/imgui/backends/imgui_impl_opengl3.cpp)

# 5. OSTALE BIBLIOTEKE (Tvoj originalni kod)
set(ENABLE_BOOST_WORKAROUND ON)
set(BUILD_STATIC_LIB ON)
set(BUILD_ASSIMP_TOOLS ON)
set(ASSIMP_BUILD_STATIC_LIB ON)

add_library(menu menu.cpp menu.h)
add_library(game game.cpp game.h)
add_library(shader shader.cpp shader.h)
add_library(stb vendor/stb_image/stb.cpp)
add_library(texture texture.cpp texture.h)
add_library(camera camera.cpp camera.h)
add_library(animated_mesh animated_mesh.cpp animated_mesh.h)
add_library(player player.cpp player.h)
add_library(enemy enemy.cpp enemy.h)
add_library(enemy_state_machine enemy_state_machine.cpp enemy_state_machine.h)
add_library(light light.cpp light.h)
add_library(map map.cpp map.h)
add_library(cursor cursor.cpp cursor.h)
add_library(collision_object collision_object.cpp collision_object.h)
add_library(collision_detector collision_detector.cpp collision_detector.h)
add_library(object_controller object_controller.cpp object_controller.h)
add_library(input_controller input_controller.cpp input_controller.h)
add_library(animation_controller animation_controller.cpp animation_controller.h)
add_library(player_controller player_controller.cpp player_controller.h)
add_library(enemy_behavior_tree enemy_behavior_tree.cpp enemy_behavior_tree.h)
add_library(utility utility.cpp utility.h)
add_library(material material.cpp material.h)
add_library(channel channel.cpp channel.h)
add_library(animation animation.cpp animation.h)
add_library(skinned_mesh skinned_mesh.cpp skinned_mesh.h)
add_library(nav_mesh nav_mesh.cpp nav_mesh.h)
add_library(aabb aabb.cpp aabb.h)
add_library(bounding_box bounding_box.cpp bounding_box.h)
add_library(picking_texture picking_texture.cpp picking_texture.h)
add_library(level_manager level_manager.cpp level_manager.h)
add_library(node node.cpp node.h)
add_library(timer timer.cpp timer.h)
add_library(sound sound.cpp sound.h)

# 6. EXECUTABLE
add_executable(main main.cpp)

# 7. LINKOVANJE
target_link_libraries(main
    menu game level_manager shader camera map animated_mesh
    player enemy cursor timer collision_object player_controller
    enemy_behavior_tree enemy_state_machine collision_detector
    object_controller input_controller animation_controller
    skinned_mesh nav_mesh texture stb material assimp
    channel light animation node utility bounding_box aabb
    picking_texture sound
    imgui_impl_glfw imgui_impl_opengl3 imgui
    OpenGL::GL glfw GLEW::GLEW
)
