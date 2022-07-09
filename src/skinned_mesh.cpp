#include "skinned_mesh.h"
#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "channel.h"
#include "light.h"
#include "shader.h"
#include "texture.h"
#include "utility.h"
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <cstddef>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/matrix.hpp>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>

SkinnedMesh::SkinnedMesh(const std::string &filename) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                            aiProcess_FlipUVs |
                            aiProcess_JoinIdenticalVertices);

  if (scene) {
    m_transformation_tree = scene->mRootNode;
    init_from_scene(scene, filename);
  } else {
    printf("Error parsing '%s': '%s'\n", filename.c_str(),
           importer.GetErrorString());
  }
}

void SkinnedMesh::init_from_scene(const aiScene *scene,
                                  const std::string &filename) {
  init_mesh_entries(scene);
  // set bones aabb after initializing all the mesh entries
  set_bones_bounding_boxes();
  init_materials(scene, filename);
  init_animations(scene);
  init_transformations(m_transformation_tree.root_node);
}

void SkinnedMesh::init_mesh_entries(const aiScene *scene) {
  m_entries.reserve(scene->mNumMeshes);
  for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
    init_mesh_entry(scene->mMeshes[i]);
  }
}

void SkinnedMesh::init_mesh_entry(const aiMesh *mesh) {
  std::vector<MeshVertex> vertices;
  vertices.reserve(mesh->mNumVertices);
  std::vector<GLuint> indices;
  indices.reserve(3 * mesh->mNumFaces);

  const aiVector3D zero(0.0f, 0.0f, 0.0f);

  for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
    const aiVector3D *position = &(mesh->mVertices[i]);
    const aiVector3D *normal = &(mesh->mNormals[i]);
    // use only the first coodinates, in general vertex can have multiple
    // texture coodinates
    const aiVector3D *texture_coord =
        mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][i]) : &zero;

    vertices.emplace_back(glm::vec3(position->x, position->y, position->z),
                          glm::vec3(normal->x, normal->y, normal->z),
                          glm::vec3(1.0f, 1.0f, 1.0f),
                          glm::vec2(texture_coord->x, texture_coord->y));
  }

  if (mesh->mNumBones > 0) {
    for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
      const auto bone = mesh->mBones[i];
      unsigned int bone_index = add_bone(bone);

      for (int j = 0; j < bone->mNumWeights; ++j) {
        const auto &vertex_weight = bone->mWeights[j];
        assert(vertex_weight.mVertexId < vertices.size() &&
               "vertex id in the range");
        vertices[vertex_weight.mVertexId].add_weight(bone_index,
                                                     vertex_weight.mWeight);
      }
    }

    update_bones_aabb(vertices);
  } else {
    // this mesh entry has no bones, so get its bounding box
    add_mesh_aabb(vertices);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
    const aiFace &face = mesh->mFaces[i];
    // we requested only triangles with aiProcess_Triangulate
    assert(face.mNumIndices == 3);

    indices.push_back(face.mIndices[0]);
    indices.push_back(face.mIndices[1]);
    indices.push_back(face.mIndices[2]);
  }

  m_entries.emplace_back(*this, vertices, indices, mesh->mNumBones > 0,
                         mesh->mMaterialIndex);
}

void SkinnedMesh::update_bones_aabb(const std::vector<MeshVertex> &vertices) {
  // got through all the vertices and update bones aabb
  for (const auto &vertex : vertices) {
    float max_weight = 0;
    for (int i = 0; i < NUM_BONES_PER_VERTEX; ++i) {
      max_weight = std::max(max_weight, vertex.weights[i]);
    }

    if (max_weight > 0) {
      for (int i = 0; i < NUM_BONES_PER_VERTEX; ++i) {
        if (vertex.weights[i] == max_weight) {
          m_bones[vertex.bone_ids[i]].aabb.update(vertex.position);
        }
      }
    }
  }
}

void SkinnedMesh::add_mesh_aabb(const std::vector<MeshVertex> &vertices) {
  // calculate mesh bounding box
  AABB aabb;
  for (const auto &vertex : vertices) {
    aabb.update(vertex.position);
  }

  unsigned int mesh_index = m_entries.size();
  m_mesh_bounding_boxes.emplace(mesh_index, aabb);
}

void SkinnedMesh::init_materials(const aiScene *scene,
                                 const std::string &filename) {
  // extract the directory part from the file name
  auto slash_index = filename.find_last_of("/");
  std::string dir;

  if (slash_index == std::string::npos) {
    dir = ".";
  } else if (slash_index == 0) {
    dir = "/";
  } else {
    dir = filename.substr(0, slash_index);
  }

  m_materials.resize(scene->mNumMaterials);

  // initialize the materials
  for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
    const aiMaterial *material = scene->mMaterials[i];

    // get diffuse textures
    unsigned int diffuse_count =
        material->GetTextureCount(aiTextureType_DIFFUSE);
    for (unsigned int j = 0; j < diffuse_count; ++j) {
      aiString path;

      if (material->GetTexture(aiTextureType_DIFFUSE, j, &path, NULL, NULL,
                               NULL, NULL, NULL) == AI_SUCCESS) {
        std::string full_path = dir + "/" + path.data;
        auto texture_it = m_textures_map.find(full_path);
        if (texture_it == m_textures_map.end()) {
          texture_it = m_textures_map
                           .emplace(full_path, Texture(full_path.c_str(),
                                                       TextureType::DIFFUSE,
                                                       m_textures_map.size()))
                           .first;
        }
        m_materials[i].add(&texture_it->second);
      }
    }
  }
}

void SkinnedMesh::init_animations(const aiScene *scene) {
  m_animations.reserve(scene->mNumAnimations);
  for (int i = 0; i < scene->mNumAnimations; ++i) {
    std::cout << std::string(scene->mAnimations[i]->mName.C_Str()) << std::endl;
    if (scene->mAnimations[i]->mDuration == 0) {
      // if duration is 0 it is position
      m_positions.emplace(scene->mAnimations[i]->mName.C_Str(),
                          scene->mAnimations[i]);
    } else {
      // if duration is greater than 0 it is animation
      m_animations.emplace(scene->mAnimations[i]->mName.C_Str(),
                           scene->mAnimations[i]);
    }
  }
}

void SkinnedMesh::init_transformations(
    const std::unique_ptr<TransformationNode> &node,
    const glm::mat4 &parent_transform) {
  assert(node && "node is not null");
  // glm::mat4 global_transform = parent_transform * node->transformation;

  // if (!node->meshes.empty()) {
  //   // add transformation matrix for node meshes
  //   add_transformation(global_transform, node->meshes);
  // }

  // for (const auto &child : node->children) {
  //   init_transformations(child, global_transform);
  // }

  glm::mat4 &node_transform = node->transformation;
  glm::mat4 root_global_transform = node_transform;

  glm::mat4 global_transform = parent_transform * node_transform;

  // cache
  node->global_transformation = global_transform;

  auto bone_info = get_bone_info(node->name);

  if (bone_info) {
    bone_info->set_final_transformation(global_transform);
  }

  if (!node->meshes.empty()) {
    // add transformation matrix for node meshes
    add_transformation(global_transform, node->meshes);
  }

  for (const auto &child : node->children) {
    init_transformations(child, global_transform);
  }
}

glm::mat4
SkinnedMesh::get_bones_for_position(const std::string &position_name) {
  auto position_it = m_positions.find(position_name);
  assert(position_it != m_positions.end() && "position name is valid");
  auto &position = position_it->second;

  // no need to clear bones, all bones will be reset during tree traversal
  m_node_transformations.clear();

  auto root_global_transform = calculate_bones_transformations(
      position, 0, *m_transformation_tree.root_node, glm::mat4(1.0));

  return root_global_transform;
}

glm::mat4 SkinnedMesh::get_final_global_transformation_for_animation(
    const std::string &animation_name) {
  auto animation_it = m_animations.find(animation_name);
  assert(animation_it != m_animations.end() && "animation name is valid");
  auto &animation = animation_it->second;

  float animation_time = animation.m_duration;

  Channel *channel =
      animation.get_channel(m_transformation_tree.root_node->name);
  if (channel) {
    channel->update(animation_time);
    return channel->get_local_transform();
  }

  return m_transformation_tree.root_node->transformation;
}

std::pair<bool, glm::mat4>
SkinnedMesh::get_bones_for_animation(const std::string &animation_name,
                                     float time, float speed_factor) {
  auto animation_it = m_animations.find(animation_name);
  assert(animation_it != m_animations.end() && "animation name is valid");
  auto &animation = animation_it->second;

  float animation_time = animation.get_animation_time(time, speed_factor);

  // no need to clear bones, all bones will be reset during tree traversal
  m_node_transformations.clear();

  auto root_global_transform = calculate_bones_transformations(
      animation, animation_time, *m_transformation_tree.root_node,
      glm::mat4(1.0f));

  bool animation_finished = time < 0 ? animation_time == 0.0f
                                     : animation_time == animation.m_duration;

  return {animation_finished, std::move(root_global_transform)};
}

glm::mat4 SkinnedMesh::calculate_bones_transformations(
    Animation &animation, float animation_time, TransformationNode &node,
    const glm::mat4 &parent_transform) {

  // take a reference to node transform in order to save new transformation if
  // needed
  glm::mat4 &node_transform = node.transformation;
  glm::mat4 root_global_transform = node_transform;

  Channel *channel = animation.get_channel(node.name);
  if (channel) {
    channel->update(animation_time);
    if (node.name == m_transformation_tree.root_node->name) {
      root_global_transform = channel->get_local_transform();
    } else {
      node_transform = channel->get_local_transform();
    }
  }

  glm::mat4 global_transform =
      (node.name == m_transformation_tree.root_node->name)
          ? parent_transform
          : parent_transform * node_transform;

  // cache
  node.global_transformation = global_transform;

  auto bone_info = get_bone_info(node.name);

  if (bone_info) {
    bone_info->set_final_transformation(global_transform);
  }

  if (!node.meshes.empty()) {
    // add transformation matrix for node meshes
    add_transformation(global_transform, node.meshes);
  }

  for (const auto &child : node.children) {
    calculate_bones_transformations(animation, animation_time, *child,
                                    global_transform);
  }

  return root_global_transform;
}

void SkinnedMesh::set_node_transformation(const std::string &bone_name,
                                          const glm::mat4 &transformation) {
  auto node_ptr_it = m_transformation_tree.nodes_index.find(bone_name);
  assert(node_ptr_it != m_transformation_tree.nodes_index.end() &&
         "node name valid");
  node_ptr_it->second->transformation *= transformation;
  init_transformations(m_transformation_tree.root_node);
}

const glm::mat4 &
SkinnedMesh::node_global_transformation(const std::string &node_name) const {
  auto node_ptr_it = m_transformation_tree.nodes_index.find(node_name);
  assert(node_ptr_it != m_transformation_tree.nodes_index.end() &&
         "node name valid");
  return node_ptr_it->second->global_transformation;
}

void SkinnedMesh::render(Shader &shader, const Camera &camera,
                         const Light &light) const {
  // basic rendering
  shader.activate();
  // set camera position and matrix
  shader.set_uniform("camPos", camera.position());
  shader.set_uniform("camMatrix", camera.matrix());
  // set light position and color
  shader.set_uniform("lightPos", light.position());
  shader.set_uniform("lightColor", light.color());
  // set bones transformations
  set_bones_transformation_uniforms(shader);

  for (auto &mesh_entry : m_entries) {
    mesh_entry.render(shader);
  }
}

void SkinnedMesh::render_to_texture(Shader &shader,
                                    const Camera &camera) const {
  // render to texture method used to render information about
  // each mesh entry and its triangles
  shader.activate();
  // set camera matrix
  shader.set_uniform("camMatrix", camera.matrix());
  // set bones transformations
  set_bones_transformation_uniforms(shader);

  for (unsigned int i = 0; i < m_entries.size(); ++i) {
    // each triangle that will be rendered for this entry will have gDrawIndex
    // set to mesh entry index, triangles indices will be set automatically in
    // shader
    shader.set_uniform("gDrawIndex", i);
    m_entries[i].render(shader);
  }
}

void SkinnedMesh::render_primitive(Shader &shader, const Camera &camera,
                                   unsigned int entry_index,
                                   unsigned int primitive_index) const {
  // render only specific primitive (triangle) of given mesh entry
  // this method is used for testing to make sure the right primitive is
  // selected on click
  assert(entry_index < m_entries.size() && "valid mesh entry index");

  shader.activate();
  // set camera matrix
  shader.set_uniform("camMatrix", camera.matrix());
  // set bones transformations
  set_bones_transformation_uniforms(shader);

  m_entries[entry_index].render_primitive(shader, primitive_index);
}

void SkinnedMesh::set_bones_transformation_uniforms(Shader &shader) const {
  // set all bones transformations
  for (int i = 0; i < m_bones.size(); ++i) {
    shader.set_uniform("gBones[" + std::to_string(i) + "]",
                       m_bones[i].final_transformation);
  }
}

BoneInfo *SkinnedMesh::get_bone_info(const std::string &name) {
  auto it = m_bone_map.find(name);
  if (it != m_bone_map.end()) {
    return &m_bones[it->second];
  }
  return nullptr;
}

unsigned int SkinnedMesh::get_bone_index(const std::string &name) {
  auto it = m_bone_map.find(name);
  if (it != m_bone_map.end()) {
    return it->second;
  }
  return INT_MAX;
}

unsigned int SkinnedMesh::add_bone(const aiBone *bone) {
  // add bone if not exists and return bone index
  assert(bone);
  auto bone_index = get_bone_index(bone->mName.data);

  if (bone_index == INT_MAX) {
    // new bone in a mesh
    bone_index = m_bones.size();
    m_bone_map.emplace(bone->mName.data, bone_index);
    m_bones.push_back({bone->mName.data,
                       utility::convert_to_glm_mat4(bone->mOffsetMatrix),
                       glm::mat4(1.0)});
  }

  return bone_index;
}

void SkinnedMesh::add_transformation(
    glm::mat4 transformation,
    const std::vector<unsigned int> &mesh_entry_indices) {
  m_node_transformations.push_back(std::move(transformation));

  for (unsigned int mesh_index : mesh_entry_indices) {
    assert(mesh_index < m_entries.size());
    m_entries[mesh_index].set_transformation_index(
        m_node_transformations.size() - 1);
  }
}

void SkinnedMesh::set_bones_bounding_boxes() {
  // fill m_bones_bounding_boxes with aabb for each bone that has the most
  // influence to at least one vertex
  for (int i = 0; i < m_bones.size(); ++i) {
    auto bounding_box = m_bones[i].get_bounding_box();
    if (bounding_box) {
      m_bones_bounding_boxes.emplace(i, std::move(*bounding_box));
    }
  }
}

std::vector<BoundingBox> SkinnedMesh::get_transformed_bounding_boxes() const {
  std::vector<BoundingBox> transformed_bounding_boxes;
  transformed_bounding_boxes.reserve(m_bones_bounding_boxes.size() +
                                     m_mesh_bounding_boxes.size());
  for (const auto &bounding_box_pair : m_bones_bounding_boxes) {
    transformed_bounding_boxes.emplace_back(bounding_box_pair.second.transform(
        m_bones[bounding_box_pair.first].final_transformation));
  }

  for (const auto &bounding_box_pair : m_mesh_bounding_boxes) {
    transformed_bounding_boxes.emplace_back(bounding_box_pair.second.transform(
        m_node_transformations[m_entries[bounding_box_pair.first]
                                   .m_transformation_index]));
  }

  return transformed_bounding_boxes;
}

// MeshEntry
SkinnedMesh::MeshEntry::MeshEntry(SkinnedMesh &mesh,
                                  const std::vector<MeshVertex> &vertices,
                                  const std::vector<GLuint> &indices,
                                  bool has_bones, unsigned int material_index)
    : m_mesh(mesh), m_material_index(material_index), m_has_bones(has_bones),
      m_indices_count(indices.size()) {
  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(MeshVertex) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (const GLvoid *)offsetof(MeshVertex, position));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (const GLvoid *)offsetof(MeshVertex, normal));
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (const GLvoid *)offsetof(MeshVertex, color));
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (const GLvoid *)offsetof(MeshVertex, texture));
  glVertexAttribIPointer(4, 4, GL_INT, sizeof(MeshVertex),
                         (const GLvoid *)offsetof(MeshVertex, bone_ids));
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (const GLvoid *)offsetof(MeshVertex, weights));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);

  glGenBuffers(1, &m_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_indices_count,
               indices.data(), GL_STATIC_DRAW);

  // unbind buffers
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

SkinnedMesh::MeshEntry::~MeshEntry() {
  glDeleteVertexArrays(1, &m_vao);
  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(1, &m_ebo);
}

void SkinnedMesh::MeshEntry::render(Shader &shader) const {
  // basic rendering
  bind(shader);
  set_model_transformation_uniform(shader);
  glDrawElements(GL_TRIANGLES, m_indices_count, GL_UNSIGNED_INT, 0);
  unbind();
}

void SkinnedMesh::MeshEntry::render_primitive(
    Shader &shader, unsigned int primitive_index) const {
  // render only specific primitive (traingle)
  // there is no need to use textures here, since triangle will have predefined
  // color in shader
  bind(shader, false /* don't bind textures */);
  set_model_transformation_uniform(shader);
  glDrawElements(
      GL_TRIANGLES, 3, GL_UNSIGNED_INT,
      (const void *)(sizeof(unsigned int) * primitive_index * 3) /* offset */);
  unbind(false);
}

void SkinnedMesh::MeshEntry::bind(Shader &shader, bool textures) const {
  // bind vertex array object
  glBindVertexArray(m_vao);
  if (textures) {
    bind_textures(shader);
  }
}

void SkinnedMesh::MeshEntry::unbind(bool unbind_textures) const {
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  if (unbind_textures) {
    assert(m_material_index < m_mesh.m_materials.size());
    m_mesh.m_materials[m_material_index].unbind();
  }
}

void SkinnedMesh::MeshEntry::bind_textures(Shader &shader) const {
  assert(m_material_index < m_mesh.m_materials.size());
  m_mesh.m_materials[m_material_index].set_slots(shader, "diffuse");
  m_mesh.m_materials[m_material_index].bind();
}

void SkinnedMesh::MeshEntry::set_model_transformation_uniform(
    Shader &shader) const {
  if (m_has_bones) {
    // model transformation is already included in bones transformations,
    // so there is no need to include it twice - set identity matrix for model
    glUniformMatrix4fv(glGetUniformLocation(shader.id(), "model"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(1.0f)));
  } else {
    // there are no bones, use model transformation
    glUniformMatrix4fv(
        glGetUniformLocation(shader.id(), "model"), 1, GL_FALSE,
        glm::value_ptr(m_mesh.m_node_transformations[m_transformation_index]));
  }
}

void SkinnedMesh::MeshEntry::set_transformation_index(unsigned int index) {
  m_transformation_index = index;
}
