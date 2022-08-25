#ifndef _ANIMATED_MESH_H_
#define _ANIMATED_MESH_H_

#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "light.h"
#include "shader.h"
#include "skinned_mesh.h"
#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <string>

class AnimatedMesh : public CollisionObject<BoundingBox> {
public:
  AnimatedMesh(const std::string &file_name);

  // return true if animation is finished and return global transformation
  std::pair<bool, glm::mat4> update(const std::string &animation_name,
                                    float time_in_seconds,
                                    float speed_factor = 1.0f);

  glm::mat4 get_final_global_transformation_for_animation(
      const std::string &animation_name);

  virtual void render(Shader &shader, const Camera &camera, const Light &light,
                      const std::vector<unsigned int> &render_object_ids,
                      bool exclude) const;

  virtual void render(Shader &shader, const Camera &camera,
                      const Light &light) const;

  virtual void render_boxes(Shader &bounding_box_shader,
                            const Camera &camera) const;

  void render_to_texture(Shader &shader, const Camera &camera) const;
  void render_primitive(Shader &shader, const Camera &camera,
                        unsigned int entry, unsigned int primitive);

  void set_user_transformation(glm::mat4 transformation);
  void set_global_transformation(glm::mat4 transformation);
  void merge_user_and_global_transformations();

  const glm::mat4 &user_transformation() const { return m_user_transformation; }

  glm::mat4 final_transformation() const {
    return m_user_transformation * m_global_transformation;
  }

  std::unique_ptr<BVHNode<BoundingBox>> get_bvh() const override;

private:
  void render_boxes(const BVHNode<BoundingBox> &node,
                    Shader &bounding_box_shader, const Camera &camera) const;

protected:
  SkinnedMesh m_skinned_mesh;

  glm::mat4 m_user_transformation = glm::mat4(1.0f);
  glm::mat4 m_global_transformation = glm::mat4(1.0f);
};

#endif /* _ANIMATED_MESH_H_ */
