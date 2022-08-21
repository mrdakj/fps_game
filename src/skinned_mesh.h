#ifndef _SKINNED_MESH_H_
#define _SKINNED_MESH_H_

#include <GL/glew.h>

#include <assimp/quaternion.h>
#include <assimp/vector3.h>
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

template <typename T> class CollisionObject;
template <typename T> struct BVHNode {
  BVHNode(T &&vol = {}) : volume(std::forward<T>(vol)) {}
  BVHNode(const T &vol) : volume(vol) {}

  std::string name;
  T volume;
  std::vector<std::unique_ptr<BVHNode<T>>> children;
  std::optional<unsigned int> render_object_id = std::nullopt;
};

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
  std::optional<BoundingBox> get_bounding_box() {
    return (aabb.valid()) ? std::make_optional<BoundingBox>(aabb)
                          : std::nullopt;
  }

  std::string name;
  // offset matrix converts local mesh coordinates to local bone coordinates
  glm::mat4 offset;
  // TODO: move this aabb outside bone since it is used only to construct
  // m_bones_bounding_boxes axis-aligned bounding box of vertices which max
  // weight is assigned to this bone
  AABB aabb;
};

struct TransformationNode {
  std::string name;
  std::vector<unsigned int> meshes;
  std::vector<std::unique_ptr<TransformationNode>> children;
};

struct TransformationNodeMutable {
  glm::vec3 local_scaling;
  glm::quat local_rotation;
  glm::vec3 local_translation;

  glm::mat4 local_transformation = glm::mat4(1.0f);
  glm::mat4 global_transformation = glm::mat4(1.0f);
};

struct TransformationTree {
  TransformationTree() : root_node(nullptr) {}

  TransformationTree(
      const aiNode *node,
      std::unordered_map<const TransformationNode *, TransformationNodeMutable>
          &transformation_map)
      : root_node(init_nodes(node, transformation_map)) {}

  std::unique_ptr<TransformationNode> init_nodes(
      const aiNode *node,
      std::unordered_map<const TransformationNode *, TransformationNodeMutable>
          &transformation_map) {
    assert(node && "node not null");

    std::vector<unsigned int> meshes;
    for (int i = 0; i < node->mNumMeshes; ++i) {
      meshes.push_back(node->mMeshes[i]);
    }

    std::vector<std::unique_ptr<TransformationNode>> children;
    for (int i = 0; i < node->mNumChildren; i++) {
      children.emplace_back(init_nodes(node->mChildren[i], transformation_map));
    }

    auto node_uptr = std::make_unique<TransformationNode>(TransformationNode{
        node->mName.data, std::move(meshes), std::move(children)});

    aiVector3t<float> ai_scaling;
    aiQuaterniont<float> ai_rotation;
    aiVector3t<float> ai_position;
    node->mTransformation.Decompose(ai_scaling, ai_rotation, ai_position);
    glm::vec3 scaling(ai_scaling.x, ai_scaling.y, ai_scaling.z);
    glm::vec3 position(ai_position.x, ai_position.y, ai_position.z);
    glm::quat rotation(ai_rotation.w, ai_rotation.x, ai_rotation.y,
                       ai_rotation.z);

    // auto a = glm::translate(glm::mat4(1.0f), position);
    // auto b = glm::scale(glm::mat4(1.0f), scaling);
    // auto c = glm::mat4(rotation);
    // auto x = a * c * b;
    // std::cout << "--------------" << std::endl;
    // utility::print_glm_mat4(x);
    // std::cout << std::endl;
    // utility::print_glm_mat4(
    //     utility::convert_to_glm_mat4(node->mTransformation));
    // std::cout << "--------------" << std::endl;

    transformation_map.emplace(
        node_uptr.get(), TransformationNodeMutable{scaling, rotation, position,
                                                   utility::convert_to_glm_mat4(
                                                       node->mTransformation),
                                                   glm::mat4(1.0f)});

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

  // render specific objects
  void render(Shader &shader, const Camera &camera, const Light &light,
              const std::vector<unsigned int> &ids_to_render) const;

  // rendering to texture for mouse picking
  void render_to_texture(Shader &shader, const Camera &camera) const;

  // rendering to texture  specific objects for mouse picking
  void render_to_texture(Shader &shader, const Camera &camera,
                         const std::vector<unsigned int> &ids_to_render) const;

  // rendering specific primitive (triangle) of given mesh entry for testing
  void render_primitive(Shader &shader, const Camera &camera,
                        unsigned int entry_index,
                        unsigned int primitive_index) const;

  // construct bounding volume hierarchy
  // packed means all boxes are in one aabb
  std::unique_ptr<BVHNode<BoundingBox>>
  get_bvh(const glm::mat4 &user_transformation = glm::mat4(1.0f),
          bool packed = false) const;

  // return true if animation is finished and return global transformation
  std::pair<bool, glm::mat4>
  get_bones_for_animation(const std::string &animation_name, float time,
                          float speed_factor = 1.0f);

  // return global transformation
  glm::mat4 get_bones_for_position(const std::string &position_name);

  glm::mat4 get_final_global_transformation_for_animation(
      const std::string &animation_name);

  const glm::mat4 &
  node_global_transformation(const std::string &node_name) const;

  const glm::mat4 &
  node_local_transformation(const std::string &node_name) const;

  void rotate_bone(const std::string &bone_name, const glm::quat &q);

  void create_transition_animation(const std::string &position_name,
                                   float duration,
                                   const std::string &bone_to_ignore = "");

private:
  void init_from_scene(const aiScene *scene, const std::string &filename);
  void init_mesh_entries(const aiScene *scene);
  void init_mesh_entry(const aiMesh *mesh);
  void init_materials(const aiScene *scene, const std::string &filename);
  void init_animations(const aiScene *scene);
  // init m_render_objects and m_nodes_to_render_object_index
  void init_render_objects(const std::unique_ptr<TransformationNode> &node);

  // update bones and nodes global transformations
  void update_global_transformations(
      const std::unique_ptr<TransformationNode> &node,
      const glm::mat4 &parent_transform = glm::mat4(1.0f));

  // fill m_bones_bounding_boxes with bones aabb
  void set_bones_bounding_boxes();
  void update_bones_aabb(const std::vector<MeshVertex> &vertices);
  // fill m_mesh_bounding_boxes with mesh aabb
  void add_mesh_aabb(const std::vector<MeshVertex> &vertices);

  // add bone if not exists and return its index in m_bones vector
  unsigned int add_bone(const aiBone *bone);
  // return bone index in m_bones vector of the bone with the given name if it
  // exists
  std::optional<unsigned int> get_bone_index(const std::string &name);

  void set_bones_transformation_uniforms(Shader &shader) const;

  // return root global transform
  glm::mat4 calculate_bones_transformations(Animation &animation,
                                            float animation_time,
                                            TransformationNode &node,
                                            const glm::mat4 &parent_transform);

  // render one mesh entry
  void render_mesh(Shader &shader, unsigned int mesh_id,
                   const glm::mat4 &transformation) const;

  // construct bounding volume hierarchy
  std::unique_ptr<BVHNode<BoundingBox>>
  get_bvh(const TransformationNode &node,
          const glm::mat4 &user_transformation) const;

  // return transformatio from m_node_transformations for the given node
  TransformationNodeMutable &
  get_node_transformation(const TransformationNode *node);
  // return transformatio from m_node_transformations for the given node
  const TransformationNodeMutable &
  get_node_transformation(const TransformationNode *node) const;

private:
  struct MeshEntry {
  public:
    MeshEntry(const std::vector<MeshVertex> &vertices,
              const std::vector<GLuint> &indices, bool has_bones,
              unsigned int material_index);

    ~MeshEntry();

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

    bool m_has_bones;
  };

private:
  // ---------- shared objects -----------------
  // shared objects are used by multiple
  // copies of the skinned mesh class

  // mesh entries
  std::shared_ptr<std::vector<MeshEntry>> m_entries;

  // all materials
  std::shared_ptr<std::vector<Material>> m_materials;
  // texture name -> texture object
  std::shared_ptr<std::unordered_map<std::string, Texture>> m_textures;

  // bones information
  std::shared_ptr<std::vector<BoneInfo>> m_bones;
  // bone name -> bone index in m_bones
  std::shared_ptr<std::unordered_map<std::string, unsigned int>> m_bone_index;

  // bone index -> bone bounding box
  std::shared_ptr<std::unordered_map<unsigned int, BoundingBox>>
      m_bones_bounding_boxes;
  // mesh index -> mesh bounding box
  std::shared_ptr<std::unordered_map<unsigned int, BoundingBox>>
      m_mesh_bounding_boxes;

  // animation name -> animation object
  std::shared_ptr<std::unordered_map<std::string, Animation>> m_animations;
  // positions are represented as animations with duration 0
  // which means there is only one keyframe
  // position name -> position object
  std::shared_ptr<std::unordered_map<std::string, Animation>> m_positions;

  std::shared_ptr<TransformationTree> m_transformation_tree;

  std::shared_ptr<std::vector<const TransformationNode *>> m_render_objects;

  // transformation node -> render object index in m_render_objects
  std::shared_ptr<std::unordered_map<const TransformationNode *, unsigned int>>
      m_nodes_to_render_object_index;

  // --------- non shadered objects ------------------------
  // non shadered objects are unique for each
  // copy of the skinned mesh class

  // nodes global and local transformations
  // node ptr -> node's transformations
  std::unordered_map<const TransformationNode *, TransformationNodeMutable>
      m_node_transformations;

  // bones final transformations
  // indices correspond to m_bones indices
  // final transformation transforms init local mesh coordinates to local
  // coordinates of transformed mesh
  std::vector<glm::mat4> m_bone_transformations;
  std::vector<Animation> m_transitions_animations;
};

#endif /*_SKINNED_MESH_H_ */
