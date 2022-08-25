#include "animated_mesh.h"
#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
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
AnimatedMesh::update(const std::string &animation_name, float time_in_seconds,
                     float speed_factor) {
  auto [animation_finished, global_transformation] =
      m_skinned_mesh.get_bones_for_animation(animation_name, time_in_seconds,
                                             speed_factor);
  clear_bounding_volumes();
  return {animation_finished, std::move(global_transformation)};
}

glm::mat4 AnimatedMesh::get_final_global_transformation_for_animation(
    const std::string &animation_name) {
  return m_skinned_mesh.get_final_global_transformation_for_animation(
      animation_name);
}

void AnimatedMesh::render_to_texture(Shader &shader,
                                     const Camera &camera) const {
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

void AnimatedMesh::render(Shader &shader, const Camera &camera,
                          const Light &light) const {
  shader.activate();
  shader.set_uniform("transformation",
                     m_user_transformation * m_global_transformation);
  m_skinned_mesh.render(shader, camera, light);
}

void AnimatedMesh::render(Shader &shader, const Camera &camera,
                          const Light &light,
                          const std::vector<unsigned int> &render_object_ids,
                          bool exclude) const {
  shader.activate();
  shader.set_uniform("transformation",
                     m_user_transformation * m_global_transformation);
  m_skinned_mesh.render(shader, camera, light, render_object_ids, exclude);
}

void AnimatedMesh::render_boxes(Shader &bounding_box_shader,
                                const Camera &camera) const {
  render_boxes(*get_bvh(), bounding_box_shader, camera);
}

void AnimatedMesh::render_boxes(const BVHNode<BoundingBox> &node,
                                Shader &bounding_box_shader,
                                const Camera &camera) const {
  node.volume.render(bounding_box_shader, camera, glm::vec3(1.0, 0.0, 0.0));
  for (const auto &child : node.children) {
    render_boxes(*child, bounding_box_shader, camera);
  }
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

std::unique_ptr<BVHNode<BoundingBox>> AnimatedMesh::get_bvh() const {
  return m_skinned_mesh.get_bvh(m_user_transformation * m_global_transformation,
                                true);
}
