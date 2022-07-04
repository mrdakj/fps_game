#include "animated_mesh.h"
#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <vector>

#include <iostream>

AnimatedMesh::AnimatedMesh(const std::string &file_name)
    : m_skinned_mesh(file_name) {}

std::pair<bool, glm::mat4>
AnimatedMesh::update(const std::string &animation_name, float time_in_seconds, bool reversed) {
  auto [animation_finished, global_transformation] =
      m_skinned_mesh.get_bones_for_animation(animation_name, time_in_seconds, reversed);
  clear_bounding_volumes();
  return {animation_finished, std::move(global_transformation)};
}

glm::mat4 AnimatedMesh::get_final_global_transformation_for_animation(const std::string &animation_name)
{
    return m_skinned_mesh.get_final_global_transformation_for_animation(animation_name);
}

void AnimatedMesh::render_to_texture(Shader &shader, const Camera &camera) {
  shader.activate();
  shader.set_uniform("transformation",
                     m_user_transformation * m_global_transformation);
  m_skinned_mesh.render_to_texture(shader, camera);
}

void AnimatedMesh::render_primitive(Shader &shader, const Camera &camera,
                                    unsigned int entry,
                                    unsigned int primitive) {
  shader.activate();
  shader.set_uniform("transformation",
                     m_user_transformation * m_global_transformation);
  m_skinned_mesh.render_primitive(shader, camera, entry, primitive);
}

void AnimatedMesh::render(Shader &shader, Shader &bounding_box_shader,
                          const Camera &camera, const Light &light) {
  shader.activate();
  shader.set_uniform("transformation",
                     m_user_transformation * m_global_transformation);
  m_skinned_mesh.render(shader, camera, light);

  // auto bvh = get_bounding_volumes();
  // bvh.m_parent.render(bounding_box_shader, camera, glm::vec3(0.0, 0.0, 1.0));
  // for (const auto &bounding_box : bvh.m_children) {
  //   bounding_box.render(bounding_box_shader, camera, glm::vec3(1.0, 0.0, 0.0));
  // }
}

void AnimatedMesh::set_user_transformation(glm::mat4 transformation) {
  m_user_transformation = std::move(transformation);
  clear_bounding_volumes();
}

void AnimatedMesh::set_global_transformation(glm::mat4 transformation) {
  m_global_transformation = std::move(transformation);
  clear_bounding_volumes();
}

void AnimatedMesh::merge_user_and_global_transformations() {
  m_user_transformation *= m_global_transformation;
  m_global_transformation = glm::mat4(1.0f);
}

BoundingVolumeHierarchy<BoundingBox>
AnimatedMesh::get_bounding_volumes() const {
  auto mesh_bounding_boxes = m_skinned_mesh.get_transformed_bounding_boxes();
  std::vector<BoundingBox> transformed_mesh_bounding_boxes;
  transformed_mesh_bounding_boxes.reserve(mesh_bounding_boxes.size());
  std::transform(mesh_bounding_boxes.cbegin(), mesh_bounding_boxes.cend(),
                 std::back_inserter(transformed_mesh_bounding_boxes),
                 [&](const auto &box) {
                   return box.transform(m_user_transformation *
                                        m_global_transformation);
                 });

  return {std::move(transformed_mesh_bounding_boxes),
          false /* don't always check children */};
}
