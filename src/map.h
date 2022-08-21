#ifndef _MAP_H_
#define _MAP_H_

#include "bounding_box.h"
#include "camera.h"
#include "collision_object.h"
#include "nav_mesh.h"
#include "skinned_mesh.h"

#include <GL/glew.h>
#include <vector>

class Map : public CollisionObject<BoundingBox> {
public:
  class Room : public CollisionObject<BoundingBox> {
  public:
    Room(const BVHNode<BoundingBox> *bvh, NavMesh nav_mesh)
        : m_bvh(bvh), m_nav_mesh(std::move(nav_mesh)) {}

    std::unique_ptr<BVHNode<BoundingBox>> get_bvh() const override {
      return nullptr;
    }

    const BVHNode<BoundingBox> &bvh() const override {
      assert(m_bvh);
      return *m_bvh;
    }

    const BVHNode<BoundingBox> *m_bvh;
    NavMesh m_nav_mesh;
  };

  Map();

  const Room *get_room(const BVHNode<BoundingBox> *node) const;

  void render(Shader &shader, Shader &bounding_box_shader, const Camera &camera,
              const Light &light,
              const std::vector<unsigned int> &mesh_ids) const;
  void render_to_texture(Shader &shader, const Camera &camera,
                         const std::vector<unsigned int> &mesh_ids) const;
  void render_primitive(Shader &shader, const Camera &camera,
                        unsigned int entry, unsigned int primitive) const;

private:
  void init_rooms(const BVHNode<BoundingBox> &node);
  bool is_room(const BVHNode<BoundingBox> &node) const;

  std::unique_ptr<BVHNode<BoundingBox>> get_bvh() const override;

  void render_boxes(const BVHNode<BoundingBox> &node,
                    Shader &bounding_box_shader, const Camera &camera) const;
  void render_nav_meshes(Shader &bounding_box_shader,
                         const Camera &camera) const;

private:
  // used only during rooms creation
  // room name -> nav mesh name
  std::unordered_map<std::string, std::string> m_room_nav_mesh_names;

  std::vector<Room> m_rooms;
  // bvh node -> room id
  std::unordered_map<const BVHNode<BoundingBox> *, unsigned int> m_rooms_index;

  // level mesh
  SkinnedMesh m_mesh;
};

#endif /* _MAP_H_ */
