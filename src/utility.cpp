#include "utility.h"
#include <cmath>
#include <iostream>

namespace utility {
glm::mat4 convert_to_glm_mat4(const aiMatrix4x4 &from) {
  glm::mat4 to;
  // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
  to[0][0] = from.a1;
  to[1][0] = from.a2;
  to[2][0] = from.a3;
  to[3][0] = from.a4;

  to[0][1] = from.b1;
  to[1][1] = from.b2;
  to[2][1] = from.b3;
  to[3][1] = from.b4;

  to[0][2] = from.c1;
  to[1][2] = from.c2;
  to[2][2] = from.c3;
  to[3][2] = from.c4;

  to[0][3] = from.d1;
  to[1][3] = from.d2;
  to[2][3] = from.d3;
  to[3][3] = from.d4;
  return to;
}

glm::vec3 convert_to_glm_vec3(const aiVector3D &vec) {
  return glm::vec3(vec.x, vec.y, vec.z);
}

glm::quat convert_to_glm_quat(const aiQuaternion &orientation) {
  return glm::quat(orientation.w, orientation.x, orientation.y, orientation.z);
}

glm::mat3 create_glm_mat3_scaling(float x, float y) {
  // column major
  return glm::mat3(x, 0, 0, 0, y, 0, 0, 0, 1);
}

glm::mat3 create_glm_mat3_translation(float x, float y) {
  // column major
  return glm::mat3(1, 0, 0, 0, 1, 0, x, y, 1);
}

glm::mat3 create_glm_mat3_rotation(float radians) {
  float cos = std::cos(radians);
  float sin = std::sin(radians);
  // column major
  return glm::mat3(cos, sin, 0, -sin, cos, 0, 0, 0, 1);
}

void print_space() {
  int space_count = 0;
  for (int i = 0; i < space_count; i++) {
    printf(" ");
  }
}

void print_assimp_matrix(const aiMatrix4x4 &m) {
  print_space();
  printf("%f %f %f %f\n", m.a1, m.a2, m.a3, m.a4);
  print_space();
  printf("%f %f %f %f\n", m.b1, m.b2, m.b3, m.b4);
  print_space();
  printf("%f %f %f %f\n", m.c1, m.c2, m.c3, m.c4);
  print_space();
  printf("%f %f %f %f\n", m.d1, m.d2, m.d3, m.d4);
}

void print_glm_mat4(const glm::mat4 &m) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      std::cout << m[j][i] << " ";
    }
    std::cout << std::endl;
  }
}

void print_glm_mat3(const glm::mat3 &m) {
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      std::cout << m[j][i] << " ";
    }
    std::cout << std::endl;
  }
}

} // namespace utility
