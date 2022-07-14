#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace utility {
void print_assimp_matrix(const aiMatrix4x4 &m);
void print_glm_mat4(const glm::mat4 &m);
void print_glm_mat3(const glm::mat3 &m);

glm::mat4 convert_to_glm_mat4(const aiMatrix4x4 &from);
glm::vec3 convert_to_glm_vec3(const aiVector3D &vec);
glm::quat convert_to_glm_quat(const aiQuaternion &orientation);

glm::mat3 create_glm_mat3_scaling(float x, float y);
glm::mat3 create_glm_mat3_translation(float x, float y);
glm::mat3 create_glm_mat3_rotation(float radians);
}; // namespace utility

#endif /* _UTILITY_H_ */
