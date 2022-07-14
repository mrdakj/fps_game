#ifndef _ROOM_H_
#define _ROOM_H_

#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "light.h"
#include "shader.h"
#include "skinned_mesh.h"
#include "texture.h"
#include <GL/glew.h>
#include <glm/ext/vector_float3.hpp>
#include <vector>

class Room : public CollisionObject<BoundingBox> {
public:
  Room(const std::string &file_name);

  void render(Shader &shader, Shader &bounding_box_shader, const Camera &camera,
              const Light &light) const;
  void render_to_texture(Shader &shader, const Camera &camera);

  std::unique_ptr<BVHNode<BoundingBox>> get_bvh() const override;

private:
  void render_boxes(const BVHNode<BoundingBox> &node, Shader &bounding_box_shader,
                    const Camera &camera) const;

  SkinnedMesh m_mesh;
};

#endif /* _ROOM_H_ */
