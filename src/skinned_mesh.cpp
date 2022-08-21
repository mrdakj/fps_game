#include "skinned_mesh.h"
#include "aabb.h"
#include "bounding_box.h"
#include "camera.h"
#include "channel.h"
#include "collision_object.h"
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
#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

SkinnedMesh::SkinnedMesh(const std::string &filename)
    : m_entries(std::make_shared<std::vector<MeshEntry>>()),
      m_materials(std::make_shared<std::vector<Material>>()),
      m_textures(std::make_shared<std::unordered_map<std::string, Texture>>()),
      m_bones(std::make_shared<std::vector<BoneInfo>>()),
      m_bone_index(
          std::make_shared<std::unordered_map<std::string, unsigned int>>()),
      m_bones_bounding_boxes(
          std::make_shared<std::unordered_map<unsigned int, BoundingBox>>()),
      m_mesh_bounding_boxes(
          std::make_shared<std::unordered_map<unsigned int, BoundingBox>>()),
      m_animations(
          std::make_shared<std::unordered_map<std::string, Animation>>()),
      m_positions(
          std::make_shared<std::unordered_map<std::string, Animation>>()),
      m_transformation_tree(std::make_shared<TransformationTree>()),
      m_render_objects(
          std::make_shared<std::vector<const TransformationNode *>>()),
      m_nodes_to_render_object_index(
          std::make_shared<
              std::unordered_map<const TransformationNode *, unsigned int>>()),
      m_node_transformations(), m_bone_transformations() {

  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      filename.c_str(),
      aiProcess_Triangulate | aiProcess_GenSmoothNormals |
          // flip uvs does 1-y for texture coordinates,
          // because in blender origin (0,0) is bottom left and texture origin
          // is top left aiProcess_FlipUVs |
          aiProcess_JoinIdenticalVertices);

  if (scene) {
    *m_transformation_tree = {scene->mRootNode, m_node_transformations};
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
  init_render_objects(m_transformation_tree->root_node);
  update_global_transformations(m_transformation_tree->root_node);
}

void SkinnedMesh::init_mesh_entries(const aiScene *scene) {
  m_entries->reserve(scene->mNumMeshes);
  for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
    init_mesh_entry(scene->mMeshes[i]);
  }
}

void SkinnedMesh::init_mesh_entry(const aiMesh *mesh) {
  std::vector<MeshVertex> vertices;
  vertices.reserve(mesh->mNumVertices);
  std::vector<GLuint> indices;
  indices.reserve(3 * mesh->mNumFaces);

  // std::cout << mesh->mName.C_Str() << std::endl;
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

  m_entries->emplace_back(vertices, indices, mesh->mNumBones > 0,
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
          (*m_bones)[vertex.bone_ids[i]].aabb.update(vertex.position);
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

  unsigned int mesh_index = m_entries->size();
  m_mesh_bounding_boxes->emplace(mesh_index, aabb);
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

  m_materials->resize(scene->mNumMaterials);

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
        auto texture_it = m_textures->find(full_path);
        if (texture_it == m_textures->end()) {
          texture_it = m_textures
                           ->emplace(full_path, Texture(full_path.c_str(),
                                                        TextureType::DIFFUSE,
                                                        m_textures->size()))
                           .first;
        }

        // get uv transformation if exists
        aiUVTransform ai_uv_transform;
        if (aiGetMaterialUVTransform(
                material, AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, j),
                &ai_uv_transform) == AI_SUCCESS) {
          UVTransform uv_transform{
              utility::create_glm_mat3_translation(
                  ai_uv_transform.mTranslation.x,
                  ai_uv_transform.mTranslation.y),
              utility::create_glm_mat3_rotation(ai_uv_transform.mRotation),
              utility::create_glm_mat3_scaling(ai_uv_transform.mScaling.x,
                                               ai_uv_transform.mScaling.y)};

          (*m_materials)[i].add(&texture_it->second, std::move(uv_transform));
        } else {
          (*m_materials)[i].add(&texture_it->second);
        }
      }
    }
  }
}

void SkinnedMesh::init_animations(const aiScene *scene) {
  m_animations->reserve(scene->mNumAnimations);

  for (int i = 0; i < scene->mNumAnimations; ++i) {
    std::cout << std::string(scene->mAnimations[i]->mName.C_Str()) << std::endl;
    if (scene->mAnimations[i]->mDuration == 0) {
      // if duration is 0 it is position
      m_positions->emplace(scene->mAnimations[i]->mName.C_Str(),
                           scene->mAnimations[i]);
    } else {
      // if duration is greater than 0 it is animation
      m_animations->emplace(scene->mAnimations[i]->mName.C_Str(),
                            scene->mAnimations[i]);
    }
  }
}

TransformationNodeMutable &
SkinnedMesh::get_node_transformation(const TransformationNode *node) {
  auto node_transformations_it = m_node_transformations.find(node);
  assert(node_transformations_it != m_node_transformations.end() &&
         "node transformation exists");
  return node_transformations_it->second;
}

const TransformationNodeMutable &
SkinnedMesh::get_node_transformation(const TransformationNode *node) const {
  auto node_transformations_it = m_node_transformations.find(node);
  assert(node_transformations_it != m_node_transformations.end() &&
         "node transformation exists");
  return node_transformations_it->second;
}

void SkinnedMesh::init_render_objects(
    const std::unique_ptr<TransformationNode> &node) {
  assert(node && "node is not null");

  if (!node->meshes.empty()) {
    m_render_objects->push_back(node.get());
    m_nodes_to_render_object_index->emplace(node.get(),
                                            m_render_objects->size() - 1);
  }

  for (const auto &child : node->children) {
    init_render_objects(child);
  }
}

void SkinnedMesh::update_global_transformations(
    const std::unique_ptr<TransformationNode> &node,
    const glm::mat4 &parent_transform) {
  assert(node && "node is not null");

  auto &node_transformations = get_node_transformation(node.get());

  node_transformations.global_transformation =
      parent_transform * node_transformations.local_transformation;

  const auto &bone_index = get_bone_index(node->name);

  if (bone_index) {
    m_bone_transformations[*bone_index] =
        node_transformations.global_transformation *
        (*m_bones)[*bone_index].offset;
  }

  for (const auto &child : node->children) {
    update_global_transformations(child,
                                  node_transformations.global_transformation);
  }
}

glm::mat4
SkinnedMesh::get_bones_for_position(const std::string &position_name) {
  auto position_it = m_positions->find(position_name);
  assert(position_it != m_positions->end() && "position name is valid");
  auto &position = position_it->second;

  auto root_global_transform = calculate_bones_transformations(
      position, 0, *m_transformation_tree->root_node, glm::mat4(1.0));

  return root_global_transform;
}

glm::mat4 SkinnedMesh::get_final_global_transformation_for_animation(
    const std::string &animation_name) {
  auto animation_it = m_animations->find(animation_name);
  assert(animation_it != m_animations->end() && "animation name is valid");
  auto &animation = animation_it->second;

  float animation_time = animation.m_duration;

  Channel *channel =
      animation.get_channel(m_transformation_tree->root_node->name);
  if (channel) {
    channel->update(animation_time);
    return channel->get_local_transform();
  }

  return get_node_transformation(m_transformation_tree->root_node.get())
      .local_transformation;
}

std::pair<bool, glm::mat4>
SkinnedMesh::get_bones_for_animation(const std::string &animation_name,
                                     float time, float speed_factor) {
  Animation *animation = nullptr;
  auto animation_it = m_animations->find(animation_name);
  if (animation_it == m_animations->end()) {
    for (auto &transition_animation : m_transitions_animations) {
      if (transition_animation.m_name == animation_name) {
        animation = &transition_animation;
        break;
      }
    }
  } else {
    animation = &animation_it->second;
  }
  assert(animation && "animation name is valid");

  float animation_time = animation->get_animation_time(time, speed_factor);

  auto root_global_transform = calculate_bones_transformations(
      *animation, animation_time, *m_transformation_tree->root_node,
      glm::mat4(1.0f));

  bool animation_finished = time < 0 ? animation_time == 0.0f
                                     : animation_time == animation->m_duration;

  return {animation_finished, std::move(root_global_transform)};
}

glm::mat4 SkinnedMesh::calculate_bones_transformations(
    Animation &animation, float animation_time, TransformationNode &node,
    const glm::mat4 &parent_transform) {

  // take a reference to node transform in order to save new transformation
  // if needed
  auto &node_transform = get_node_transformation(&node);
  glm::mat4 root_global_transform = node_transform.local_transformation;

  Channel *channel = animation.get_channel(node.name);
  if (channel) {
    channel->update(animation_time);

    if (node.name == m_transformation_tree->root_node->name) {
      root_global_transform = channel->get_local_transform();
    } else {
      node_transform.local_transformation = channel->get_local_transform();
      node_transform.local_scaling = channel->get_local_scaling();
      node_transform.local_translation = channel->get_local_translation();
      node_transform.local_rotation = channel->get_local_rotation();
    }
  }

  node_transform.global_transformation =
      (node.name == m_transformation_tree->root_node->name)
          ? parent_transform
          : parent_transform * node_transform.local_transformation;

  const auto &bone_index = get_bone_index(node.name);

  if (bone_index) {
    m_bone_transformations[*bone_index] =
        node_transform.global_transformation * (*m_bones)[*bone_index].offset;
  }

  for (const auto &child : node.children) {
    calculate_bones_transformations(animation, animation_time, *child,
                                    node_transform.global_transformation);
  }

  return root_global_transform;
}

void SkinnedMesh::rotate_bone(const std::string &bone_name,
                              const glm::quat &q) {
  auto node_ptr_it = m_transformation_tree->nodes_index.find(bone_name);
  assert(node_ptr_it != m_transformation_tree->nodes_index.end() &&
         "node name valid");
  auto &node_transform = get_node_transformation(node_ptr_it->second);
  node_transform.local_rotation *= q;
  auto translation =
      glm::translate(glm::mat4(1.0f), node_transform.local_translation);
  auto scaling = glm::scale(glm::mat4(1.0f), node_transform.local_scaling);
  auto rotation = glm::mat4(node_transform.local_rotation);
  node_transform.local_transformation = translation * rotation * scaling;
  update_global_transformations(m_transformation_tree->root_node);
}

const glm::mat4 &
SkinnedMesh::node_global_transformation(const std::string &node_name) const {
  auto node_ptr_it = m_transformation_tree->nodes_index.find(node_name);
  assert(node_ptr_it != m_transformation_tree->nodes_index.end() &&
         "node name valid");
  const auto &node_transform = get_node_transformation(node_ptr_it->second);
  return node_transform.global_transformation;
}

const glm::mat4 &
SkinnedMesh::node_local_transformation(const std::string &node_name) const {
  auto node_ptr_it = m_transformation_tree->nodes_index.find(node_name);
  assert(node_ptr_it != m_transformation_tree->nodes_index.end() &&
         "node name valid");
  const auto &node_transform = get_node_transformation(node_ptr_it->second);
  return node_transform.local_transformation;
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

  // render all
  for (const auto render_object : *m_render_objects) {
    assert(!render_object->meshes.empty() && "render object has meshes");
    for (unsigned int mesh_id : render_object->meshes) {
      render_mesh(shader, mesh_id,
                  get_node_transformation(render_object).global_transformation);
    }
  }
}

void SkinnedMesh::render(Shader &shader, const Camera &camera,
                         const Light &light,
                         const std::vector<unsigned int> &ids_to_render) const {

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

  for (unsigned int id : ids_to_render) {
    assert(!(*m_render_objects)[id]->meshes.empty() &&
           "render object has meshes");
    for (unsigned int mesh_id : (*m_render_objects)[id]->meshes) {
      render_mesh(shader, mesh_id,
                  get_node_transformation((*m_render_objects)[id])
                      .global_transformation);
    }
  }
}

void SkinnedMesh::render_mesh(Shader &shader, unsigned int mesh_id,
                              const glm::mat4 &transformation) const {
  // basic rendering
  auto const &mesh_entry = (*m_entries)[mesh_id];

  // bind
  glBindVertexArray(mesh_entry.m_vao);

  assert(mesh_entry.m_material_index < m_materials->size());
  (*m_materials)[mesh_entry.m_material_index].set_slots(shader, "diffuse");
  (*m_materials)[mesh_entry.m_material_index].set_uv_transformations(
      shader, "uv_transformation");
  (*m_materials)[mesh_entry.m_material_index].bind();

  // set transformation
  if (mesh_entry.m_has_bones) {
    // model transformation is already included in bones transformations,
    // so there is no need to include it twice - set identity matrix for
    // model
    glUniformMatrix4fv(glGetUniformLocation(shader.id(), "model"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(1.0f)));
  } else {
    // there are no bones, use model transformation
    glUniformMatrix4fv(glGetUniformLocation(shader.id(), "model"), 1, GL_FALSE,
                       glm::value_ptr(transformation));
  }

  // draw
  glDrawElements(GL_TRIANGLES, mesh_entry.m_indices_count, GL_UNSIGNED_INT, 0);

  // unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  (*m_materials)[mesh_entry.m_material_index].unbind();
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

  // each triangle that will be rendered for this entry will have gDrawIndex
  // set to mesh entry index, triangles indices will be set automatically in
  // shader

  for (unsigned int id = 0; id < m_render_objects->size(); ++id) {
    shader.set_uniform("gDrawIndex", id);
    assert(!(*m_render_objects)[id]->meshes.empty() &&
           "render object has meshes");
    for (unsigned int mesh_id : (*m_render_objects)[id]->meshes) {
      render_mesh(shader, mesh_id,
                  get_node_transformation((*m_render_objects)[id])
                      .global_transformation);
    }
  }
}

void SkinnedMesh::render_to_texture(
    Shader &shader, const Camera &camera,
    const std::vector<unsigned int> &ids_to_render) const {
  // render to texture method used to render information about
  // each mesh entry and its triangles
  shader.activate();
  // set camera matrix
  shader.set_uniform("camMatrix", camera.matrix());
  // set bones transformations
  set_bones_transformation_uniforms(shader);

  for (unsigned int id : ids_to_render) {
    shader.set_uniform("gDrawIndex", id);
    assert(!(*m_render_objects)[id]->meshes.empty() &&
           "render object has meshes");
    for (unsigned int mesh_id : (*m_render_objects)[id]->meshes) {
      render_mesh(shader, mesh_id,
                  get_node_transformation((*m_render_objects)[id])
                      .global_transformation);
    }
  }
}

void SkinnedMesh::render_primitive(Shader &shader, const Camera &camera,
                                   unsigned int object_index,
                                   unsigned int primitive_index) const {
  // render only specific primitive (triangle) of given mesh entry
  // this method is used for testing to make sure the right primitive is
  // selected on click

  assert(object_index < m_render_objects->size() && "valid mesh entry index");
  assert(!(*m_render_objects)[object_index]->meshes.empty() &&
         "render object has meshes");
  // TODO: don't take always the first mesh
  unsigned int mesh_id = (*m_render_objects)[object_index]->meshes[0];
  const auto &mesh = (*m_entries)[mesh_id];

  shader.activate();
  // set camera matrix
  shader.set_uniform("camMatrix", camera.matrix());
  // set bones transformations
  set_bones_transformation_uniforms(shader);

  // render only specific primitive (traingle)
  // there is no need to use textures here, since triangle will have
  // predefined color in shader

  // bind
  glBindVertexArray(mesh.m_vao);

  // set transformation
  if (mesh.m_has_bones) {
    // model transformation is already included in bones transformations,
    // so there is no need to include it twice - set identity matrix for
    // model
    glUniformMatrix4fv(glGetUniformLocation(shader.id(), "model"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(1.0f)));
  } else {
    // there are no bones, use model transformation
    glUniformMatrix4fv(glGetUniformLocation(shader.id(), "model"), 1, GL_FALSE,
                       glm::value_ptr(get_node_transformation(
                                          (*m_render_objects)[object_index])
                                          .global_transformation));
  }

  // draw
  glDrawElements(
      GL_TRIANGLES, 3, GL_UNSIGNED_INT,
      (const void *)(sizeof(unsigned int) * primitive_index * 3) /* offset */);

  // unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SkinnedMesh::set_bones_transformation_uniforms(Shader &shader) const {
  // set all bones transformations
  for (int i = 0; i < m_bone_transformations.size(); ++i) {
    shader.set_uniform("gBones[" + std::to_string(i) + "]",
                       m_bone_transformations[i]);
  }
}

std::optional<unsigned int>
SkinnedMesh::get_bone_index(const std::string &name) {
  auto it = m_bone_index->find(name);
  if (it != m_bone_index->end()) {
    return it->second;
  }
  return std::nullopt;
}

unsigned int SkinnedMesh::add_bone(const aiBone *bone) {
  // add bone if not exists and return bone index
  assert(bone);
  auto bone_index = get_bone_index(bone->mName.data);
  if (bone_index) {
    return *bone_index;
  }

  // new bone in a mesh
  unsigned int new_bone_index = m_bones->size();
  m_bone_index->emplace(bone->mName.data, new_bone_index);
  m_bones->push_back(
      {bone->mName.data, utility::convert_to_glm_mat4(bone->mOffsetMatrix)});
  m_bone_transformations.push_back(glm::mat4(1.0f));
  return new_bone_index;
}

void SkinnedMesh::set_bones_bounding_boxes() {
  // fill m_bones_bounding_boxes with aabb for each bone that has the most
  // influence to at least one vertex
  for (int i = 0; i < m_bones->size(); ++i) {
    auto bounding_box = (*m_bones)[i].get_bounding_box();
    if (bounding_box) {
      m_bones_bounding_boxes->emplace(i, std::move(*bounding_box));
    }
  }
}

std::unique_ptr<BVHNode<BoundingBox>>
SkinnedMesh::get_bvh(const glm::mat4 &user_transformation, bool packed) const {
  if (!packed) {
    // should not be used for skinned mesh with bones
    return get_bvh(*m_transformation_tree->root_node, user_transformation);
  }

  // used for skinned mesh with bones
  std::vector<BoundingBox> transformed_bounding_boxes;
  transformed_bounding_boxes.reserve(m_bones_bounding_boxes->size() +
                                     m_mesh_bounding_boxes->size());
  for (const auto &bounding_box_pair : *m_bones_bounding_boxes) {
    transformed_bounding_boxes.emplace_back(bounding_box_pair.second.transform(
        user_transformation * m_bone_transformations[bounding_box_pair.first]));
  }

  for (const auto &render_object : *m_render_objects) {
    assert(!render_object->meshes.empty() && "render object has meshes");

    for (unsigned int mesh_index : render_object->meshes) {
      if (!(*m_entries)[mesh_index].m_has_bones) {
        auto mesh_box_it = m_mesh_bounding_boxes->find(mesh_index);
        assert(mesh_box_it != m_mesh_bounding_boxes->end() && "mesh box found");
        transformed_bounding_boxes.emplace_back(mesh_box_it->second.transform(
            user_transformation *
            get_node_transformation(render_object).global_transformation));
      }
    }
  }

  return std::make_unique<BVHNode<BoundingBox>>(
      BoundingBox::bounding_aabb(transformed_bounding_boxes));
}

std::unique_ptr<BVHNode<BoundingBox>>
SkinnedMesh::get_bvh(const TransformationNode &node,
                     const glm::mat4 &transformation) const {
  BVHNode<BoundingBox> current_node;
  current_node.name = node.name;

  if (!node.meshes.empty()) {
    auto node_to_render_object_it = m_nodes_to_render_object_index->find(&node);
    assert(node_to_render_object_it != m_nodes_to_render_object_index->end() &&
           "render object exists");
    current_node.render_object_id = node_to_render_object_it->second;
  }

  auto current_transformation =
      transformation * get_node_transformation(&node).local_transformation;

  std::vector<BoundingBox> boxes;
  for (auto &child : node.children) {
    auto child_bvh = get_bvh(*child, current_transformation);
    if (child_bvh) {
      boxes.push_back(child_bvh->volume);
      current_node.children.emplace_back(std::move(child_bvh));
    }
  }

  for (unsigned int mesh_index : node.meshes) {
    auto mesh_box = m_mesh_bounding_boxes->find(mesh_index);
    assert(mesh_box != m_mesh_bounding_boxes->end() &&
           "mesh bounding box exists");
    boxes.push_back(mesh_box->second.transform(current_transformation));
    current_node.children.emplace_back(
        std::make_unique<BVHNode<BoundingBox>>(boxes.back()));
  }

  if (boxes.empty()) {
    // node is useless for bounding volume hierarchy
    return nullptr;
  }

  current_node.volume = (boxes.size() == 1) ? std::move(boxes[0])
                                            : BoundingBox::bounding_aabb(boxes);

  return std::make_unique<BVHNode<BoundingBox>>(std::move(current_node));
}

void SkinnedMesh::create_transition_animation(
    const std::string &position_name, float duration,
    const std::string &bone_to_ignore) {
  m_transitions_animations.clear();
  std::vector<Channel> channels;
  std::unordered_map<std::string, int> channels_map;

  auto position_it = m_positions->find(position_name);
  if (position_it == m_positions->end()) {
    position_it = m_animations->find(position_name);
    assert(position_it != m_animations->end() && "valid position name");
  }

  for (const auto &node_transformations : m_node_transformations) {
    const std::string &name = node_transformations.first->name;
    if (name == bone_to_ignore) {
      continue;
    }

    const TransformationNodeMutable &transformations =
        node_transformations.second;

    std::vector<KeyPosition> positions;
    positions.push_back({transformations.local_translation, 0});
    std::vector<KeyRotation> rotations;
    rotations.push_back({transformations.local_rotation, 0});
    std::vector<KeyScale> scaling;
    scaling.push_back({transformations.local_scaling, 0});

    Channel *channel = position_it->second.get_channel(name);
    if (channel) {
      const auto &position_channel = channel->positions_channel();
      const auto &scaling_channel = channel->scales_channel();
      const auto &rotation_channel = channel->rotations_channel();

      if (!position_channel.empty()) {
        positions.push_back({position_channel[0].position, duration});
      }

      if (!scaling_channel.empty()) {
        scaling.push_back({scaling_channel[0].scale, duration});
      }

      if (!rotation_channel.empty()) {
        rotations.push_back({rotation_channel[0].orientation, duration});
      }
    }

    channels_map.emplace(name, channels.size());
    channels.emplace_back(name, positions, rotations, scaling);
  }

  m_transitions_animations.emplace_back("transition", std::move(channels),
                                        std::move(channels_map), duration, 1);
}

// MeshEntry
SkinnedMesh::MeshEntry::MeshEntry(const std::vector<MeshVertex> &vertices,
                                  const std::vector<GLuint> &indices,
                                  bool has_bones, unsigned int material_index)
    : m_material_index(material_index), m_has_bones(has_bones),
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
