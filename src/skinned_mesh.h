#ifndef _SKINNED_MESH_H_
#define _SKINNED_MESH_H_

#include <GL/glew.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "aabb.h"
#include "animation.h"
#include "bounding_box.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "shader.h"
#include "texture.h"
#include "utility.h"

const unsigned int NUM_BONES_PER_VERTEX = 4;

struct MeshVertex {
  MeshVertex(glm::vec3 position, glm::vec3 normal, glm::vec3 color,
             glm::vec2 texture)
      : position(std::move(position)), normal(std::move(normal)),
        color(std::move(color)), texture(std::move(texture)) {
    bone_ids.fill(0);
    weights.fill(0);
  }

  void add_weight(uint bone_id, float weight) {
    for (int i = 0; i < NUM_BONES_PER_VERTEX; ++i) {
      if (weights[i] == 0.0f) {
        bone_ids[i] = bone_id;
        weights[i] = weight;
        return;
      }
    }

    throw "no space for bone";
  }

  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 texture;

  std::array<uint, NUM_BONES_PER_VERTEX> bone_ids;
  std::array<float, NUM_BONES_PER_VERTEX> weights;
};

struct BoneInfo {
  void set_final_transformation(const glm::mat4 &transformation) {
    final_transformation = transformation * offset;
  }

  std::optional<BoundingBox> get_bounding_box() {
    return (aabb.valid()) ? std::make_optional<BoundingBox>(aabb)
                          : std::nullopt;
  }

  std::string name;
  // offset matrix converts local mesh coordinates to local bone coordinates
  glm::mat4 offset;
  // final transformation transforms init local mesh coordinates to local
  // coordinates of transformed mesh
  glm::mat4 final_transformation = glm::mat4(1.0f);
  // axis-aligned bounding box of vertices which max weight is assigned to this
  // bone
  AABB aabb;
};

struct TransformationNode {
  std::string name;
  // local transformation
  glm::mat4 transformation;
  // cache
  glm::mat4 global_transformation;
  std::vector<unsigned int> meshes;
  std::vector<std::unique_ptr<TransformationNode>> children;
};

struct TransformationTree {
  TransformationTree() : root_node(nullptr) {}
  TransformationTree(const aiNode *node) : root_node(init_nodes(node)) {}

  std::unique_ptr<TransformationNode> init_nodes(const aiNode *node) {
    assert(node && "node not null");

    std::vector<unsigned int> meshes;
    for (int i = 0; i < node->mNumMeshes; ++i) {
      meshes.push_back(node->mMeshes[i]);
    }

    std::vector<std::unique_ptr<TransformationNode>> children;
    for (int i = 0; i < node->mNumChildren; i++) {
      children.emplace_back(init_nodes(node->mChildren[i]));
    }

    auto node_uptr = std::make_unique<TransformationNode>(TransformationNode{
        node->mName.data, utility::convert_to_glm_mat4(node->mTransformation),
        glm::mat4(1.0f) /* global transformation cache not set yet */,
        std::move(meshes), std::move(children)});

    nodes_index[node_uptr->name] = node_uptr.get();
    return node_uptr;
  }

  // node name -> node's pointer
  std::unordered_map<std::string, TransformationNode *> nodes_index;
  std::unique_ptr<TransformationNode> root_node;
};

class SkinnedMesh {
public:
  SkinnedMesh(const std::string &filename);

  // basic rendering
  void render(Shader &shader, const Camera &camera, const Light &light) const;
  // rendering to texture for mouse picking
  void render_to_texture(Shader &shader, const Camera &camera) const;
  // rendering specific primitive (triangle) of given mesh entry for testing
  void render_primitive(Shader &shader, const Camera &camera,
                        unsigned int entry_index,
                        unsigned int primitive_index) const;

  // get transformed bounding boxes for bones and mesh entries
  std::vector<BoundingBox> get_transformed_bounding_boxes() const;

  // return true if animation is finished and return global transformation
  std::pair<bool, glm::mat4>
  get_bones_for_animation(const std::string &animation_name, float time,
                          bool reversed = false);

  // return global transformation
  glm::mat4 get_bones_for_position(const std::string &position_name);

  glm::mat4 get_final_global_transformation_for_animation(
      const std::string &animation_name);

  const glm::mat4 &
  node_global_transformation(const std::string &node_name) const;

private:
  void init_from_scene(const aiScene *scene, const std::string &filename);
  void init_mesh_entries(const aiScene *scene);
  void init_mesh_entry(const aiMesh *mesh);
  void init_materials(const aiScene *scene, const std::string &filename);
  void init_animations(const aiScene *scene);

  void
  init_transformations(const std::unique_ptr<TransformationNode> &node,
                       const glm::mat4 &parent_transform = glm::mat4(1.0f));

  // fill m_bones_bounding_boxes with bones aabb
  void set_bones_bounding_boxes();
  void update_bones_aabb(const std::vector<MeshVertex> &vertices);
  // fill m_mesh_bounding_boxes with mesh aabb
  void add_mesh_aabb(const std::vector<MeshVertex> &vertices);

  // add bone if not exists and return its index in m_bones vector
  unsigned int add_bone(const aiBone *bone);
  // return bone info of the bone with the given name, or null if no such bone
  // exists
  BoneInfo *get_bone_info(const std::string &name);
  unsigned int get_bone_index(const std::string &name);

  void set_bones_transformation_uniforms(Shader &shader) const;

  // add new transformation and set transformation index for given mesh entries
  void add_transformation(glm::mat4 transformation,
                          const std::vector<unsigned int> &mesh_entry_indices);

  // return root global transform
  glm::mat4 calculate_bones_transformations(Animation &animation,
                                            float animation_time,
                                            TransformationNode &node,
                                            const glm::mat4 &parent_transform);

private:
  struct MeshEntry {
  public:
    MeshEntry(SkinnedMesh &mesh, const std::vector<MeshVertex> &vertices,
              const std::vector<GLuint> &indices, bool has_bones,
              unsigned int material_index);

    ~MeshEntry();

    // basic rendering
    void render(Shader &shader) const;
    // rendering specific primitive (triangle) for testing
    void render_primitive(Shader &shader, unsigned int primitive_index) const;

    void set_transformation_index(unsigned int index);

  private:
    void bind_textures(Shader &shader) const;
    void bind(Shader &shader, bool bind_textures = true) const;
    void unbind(bool unbind_textures = true) const;
    void set_model_transformation_uniform(Shader &shader) const;

  public:
    // vertex array object
    GLuint m_vao;
    // vertex buffer object
    GLuint m_vbo;
    // element buffer object (index buffer)
    GLuint m_ebo;

    // number of indices for this mesh entry
    unsigned int m_indices_count;
    // index of texture in m_textures vector
    unsigned int m_material_index;
    // index of node transformation in m_node_transformations vector
    unsigned int m_transformation_index;

    bool m_has_bones;
    SkinnedMesh &m_mesh;
  };

private:
  // mesh entries
  std::vector<MeshEntry> m_entries;

  // all materials
  std::vector<Material> m_materials;
  // texture name -> texture object
  std::unordered_map<std::string, Texture> m_textures_map;

  // all nodes transformations
  std::vector<glm::mat4> m_node_transformations;

  // all bones information
  std::vector<BoneInfo> m_bones;
  // bone name -> bone index in m_bones
  std::unordered_map<std::string, unsigned int> m_bone_map;

  // bone index -> bone bounding box
  std::unordered_map<unsigned int, BoundingBox> m_bones_bounding_boxes;
  // mesh index -> mesh bounding box
  std::unordered_map<unsigned int, BoundingBox> m_mesh_bounding_boxes;

  std::unordered_map<std::string, Animation> m_animations;
  // positions are represented as animations with duration 0
  // which means there is only one keyframe
  std::unordered_map<std::string, Animation> m_positions;

  TransformationTree m_transformation_tree;
};

#endif /*_SKINNED_MESH_H_ */
