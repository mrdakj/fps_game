#ifndef _MAP_H_
#define _MAP_H_

#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "enemy.h"
#include "light.h"
#include "shader.h"
#include "skinned_mesh.h"
#include "texture.h"
#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <vector>

class Map : public CollisionObject<BoundingBox> {
public:
  Map(const std::string &file_name);

  void render(Shader &shader, Shader &bounding_box_shader, const Camera &camera,
              const Light &light,
              const std::vector<unsigned int> &mesh_ids) const;

  void render_to_texture(Shader &shader, const Camera &camera,
                         const std::vector<unsigned int> &mesh_ids) const;

  std::unique_ptr<BVHNode<BoundingBox>> get_bvh() const override;

  bool is_room(const BVHNode<BoundingBox> &node) const;

private:
  void render_boxes(const BVHNode<BoundingBox> &node,
                    Shader &bounding_box_shader, const Camera &camera) const;

  SkinnedMesh m_mesh;
  const std::vector<std::string> m_rooms = {"ROOT"};
};

#endif /* _MAP_H_ */
