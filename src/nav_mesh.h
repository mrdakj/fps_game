#ifndef _NAV_MESH_H_
#define _NAV_MESH_H_

#include "camera.h"
#include "shader.h"
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
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Bezier {
  // linear Bezier and quadratic Bezier curves supported
public:
  // linear Bezier curve constructor
  Bezier(glm::vec3 p1, glm::vec3 p2);
  // quadratic Bezier curve constructor
  Bezier(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

  // get point for parameter t in [0, 1]
  glm::vec3 get_point(float t) const;
  // get derivative in point t in [0, 1]
  glm::vec3 get_derivative(float t) const;

  // advance parameter t such that maximum delta length is moved along the curve
  // t should stay in [0, 1] interval
  // return how much length is moved
  float advance_t(float &t, float delta) const;

private:
  // holds either two or three points
  std::vector<glm::vec3> m_points;
  // ----------- cache ----------------
  // curve derivative cache
  glm::vec3 m_v1;
  glm::vec3 m_v2;
};

class Path {
public:
  Path(std::vector<Bezier> curves = {});

  // get the next point and direction such that delta distance is moved across
  // the curve
  std::pair<glm::vec3, glm::vec3>
  get_next_point_and_direction(float delta_distance);

  bool is_path_done() const;

private:
  std::vector<Bezier> m_curves;
  // index of current curve we are in
  unsigned int m_current_curve;
  // parameter of current curve
  float m_t;
};

class NavMesh {
public:
  NavMesh(const std::string &filename);

  void render(Shader &shader, const Camera &camera) const;

  // get path that begins in src point and ends in dest point
  Path get_path(const glm::vec3 &src, const glm::vec3 &dest) const;

  glm::vec3 get_random_point() const;

private:
  void init_from_scene(const aiScene *scene);

  // return true if point is in triangle with the given id
  bool is_point_in_triangle(unsigned int i, const glm::vec3 &point) const;
  // return triangle id of triangle that contains the given point
  // if no such triangle exists return null
  std::optional<unsigned int>
  get_triangle_for_point(const glm::vec3 &point) const;

  // find the middle point of the common edge of triangles with ids
  // i and j
  glm::vec3 mid_point_common_edge(unsigned int i, unsigned int j) const;

  // get array of triangle ids that form a path from src ponint to dest point
  std::vector<unsigned int> get_triangle_path(const glm::vec3 &src,
                                              const glm::vec3 &dest) const;

private:
  struct Triangle {
    Triangle(unsigned int a, unsigned int b, unsigned int c)
        : a(a), b(b), c(c) {}
    // indices of vertices in m_vertices
    unsigned int a, b, c;
    // indices of triangles in m_triangles
    std::vector<unsigned int> neighbours;
  };

  // ----------------- member vars -------------------
private:
  std::vector<glm::vec3> m_vertices;
  std::vector<Triangle> m_triangles;
  // -------------------------------------------------
};

#endif /* _NAV_MESH_H_ */
