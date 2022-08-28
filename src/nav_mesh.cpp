#include "nav_mesh.h"
#include <algorithm>
#include <ctime>
#include <glm/geometric.hpp>
#include <optional>
#include <queue>
#include <unordered_set>
#include <vector>

#define EPS 0.0001

// ------------------- Bezier -------------------------------------------
Bezier::Bezier(glm::vec3 p1, glm::vec3 p2)
    : m_points({std::move(p1), std::move(p2)}),
      // cache for linear curve to find derivative
      m_v1(m_points[1] - m_points[0]) {}

Bezier::Bezier(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
    : m_points({std::move(p1), std::move(p2), std::move(p3)}),
      // cache for quadratic curve to find derivative
      m_v1(2.0f * m_points[0] - 4.0f * m_points[1] + 2.0f * m_points[2]),
      m_v2(-2.0f * m_points[0] + 2.0f * m_points[1]) {}

glm::vec3 Bezier::get_point(float t) const {
  // get point for parameter t in [0, 1]
  // for t=0 return the first curve point
  // for t=1 return the last curve point
  assert(t >= 0 && t <= 1 && "t is in [0,1]");
  assert((m_points.size() == 2 || m_points.size() == 3) &&
         "it is a segment or quadratic Bezier curve");

  if (m_points.size() == 2) {
    // it is a line segment
    return m_points[0] + t * (m_points[1] - m_points[0]);
  }

  // it is a quadratic Bezier curve
  return (1 - t) * (1 - t) * m_points[0] + 2 * (1 - t) * t * m_points[1] +
         t * t * m_points[2];
}

glm::vec3 Bezier::get_derivative(float t) const {
  // get derivative in point t in [0, 1]
  assert(t >= 0 && t <= 1 && "t is in [0,1]");
  assert((m_points.size() == 2 || m_points.size() == 3) &&
         "it is a segment or quadratic Bezier curve");

  return m_points.size() == 2 ? m_v1 : t * m_v1 + m_v2;
}

float Bezier::advance_t(float &t, float delta) const {
  // advance parameter t such that maximum delta length is moved along the curve
  // t should stay in [0, 1] interval
  // return how much length is moved

  // idea is aproximate a curve with derivative at point t and to increase t
  // such that we moved for delta length along derivative in point t
  // it will be a good aproximation if delta is small, if delta is "big", devide
  // delta length into multiple smaller delta steps
  assert(t >= 0 && t <= 1 && "t is in [0,1]");
  assert((m_points.size() == 2 || m_points.size() == 3) &&
         "it is a segment or quadratic Bezier curve");

  float l = glm::length(get_derivative(t));
  float remaining_distance = (1 - t) * l;

  if (m_points.size() == 2) {
    // it is a line segment
    if (delta <= remaining_distance) {
      // check upper bound just because of floating point precision error
      t = std::min(t + delta / l, 1.0f);
      return delta;
    }

    // we moved all remaining distance which is less than delta length
    t = 1;
    return remaining_distance;
  }

  if (delta < 0.01) {
    t = t + delta / l;
  } else {
    // delta is "big" and aproximation won't be good enough so break one step
    // into multiple smaller steps
    float smaller_delta = delta / 10;
    for (int i = 0; i < 10; i++) {
      t = t + smaller_delta / l;
      l = glm::length(t * m_v1 + m_v2);
    }
  }

  if (t <= 1) {
    return delta;
  }

  // we moved all remaining distance which is less than delta length
  t = 1;
  return remaining_distance;
}
// ------------------------- Bezier End ---------------------------

// ------------------------- Path ---------------------------------
Path::Path(std::vector<Bezier> curves)
    : m_curves(std::move(curves)), m_current_curve(0), m_t(0) {}

std::pair<glm::vec3, glm::vec3>
Path::get_next_point_and_direction(float delta_distance) {
  // get the next point and direction such that delta distance is moved across
  // the curve
  assert(!m_curves.empty() && "curves is not empty");
  assert(m_current_curve < m_curves.size() && "current_curve valid");

  while (m_current_curve < m_curves.size()) {
    float distance_moved =
        m_curves[m_current_curve].advance_t(m_t, delta_distance);
    if (distance_moved < delta_distance) {
      // go to the next curve to finish the step
      ++m_current_curve;
      m_t = 0;
      delta_distance -= distance_moved;
    } else {
      // we moved for delta distance
      return {m_curves[m_current_curve].get_point(m_t),
              m_curves[m_current_curve].get_derivative(m_t)};
    }
  }

  // we reached the end of the path
  m_current_curve = m_curves.size() - 1;
  m_t = 1;
  return {m_curves.back().get_point(m_t), m_curves.back().get_derivative(m_t)};
}

bool Path::is_path_done() const {
  return m_current_curve == m_curves.size() - 1 && m_t == 1;
}
// ------------------------- Path End ------------------------------

// -------------------------- NavMesh -------------------------------
NavMesh::NavMesh(const std::string &filename) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                            aiProcess_JoinIdenticalVertices);

  if (scene) {
    init_from_scene(scene);
  } else {
    printf("Error parsing '%s': '%s'\n", filename.c_str(),
           importer.GetErrorString());
  }
}

void NavMesh::init_from_scene(const aiScene *scene) {
  assert(scene->mNumMeshes == 1 && "scene should have one nav mesh");
  const auto &mesh = scene->mMeshes[0];
  m_vertices.reserve(mesh->mNumVertices);

  // get vertices
  for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
    const aiVector3D *ai_vertex = &(mesh->mVertices[i]);
    m_vertices.emplace_back(ai_vertex->x, ai_vertex->y + 0.01, ai_vertex->z);
  }

  // edge key for hashing
  auto edge_key = [](unsigned int i, unsigned int j) {
    if (j < i) {
      std::swap(i, j);
    }
    return ((size_t)i << 32) | j;
  };

  // vector should have either size 1 or 2 because edge belongs to at most 2
  // triangles
  std::unordered_map<size_t, std::vector<unsigned int>> edge_to_triangles_id;

  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
    const aiFace &face = mesh->mFaces[i];
    assert(face.mNumIndices == 3);

    unsigned int a = face.mIndices[0];
    unsigned int b = face.mIndices[1];
    unsigned int c = face.mIndices[2];
    unsigned int triangle_id = m_triangles.size();

    m_triangles.emplace_back(a, b, c);
    edge_to_triangles_id[edge_key(a, b)].push_back(triangle_id);
    edge_to_triangles_id[edge_key(b, c)].push_back(triangle_id);
    edge_to_triangles_id[edge_key(c, a)].push_back(triangle_id);
  }

  auto add_neighbour = [&](unsigned int triangle_id, unsigned int a,
                           unsigned int b) {
    const auto &triangles_ab = edge_to_triangles_id[edge_key(a, b)];
    assert(
        (triangles_ab.size() == 1 && triangles_ab[0] == triangle_id) ||
        (triangles_ab.size() == 2 && triangles_ab[0] != triangles_ab[1] &&
         (triangles_ab[0] == triangle_id || triangles_ab[1] == triangle_id)) &&
            "edge belongs to 1 or 2 triangles and one of them is this "
            "one");

    if (triangles_ab.size() > 1) {
      m_triangles[triangle_id].neighbours.push_back(
          triangles_ab[0] != triangle_id ? triangles_ab[0] : triangles_ab[1]);
    }
  };

  for (unsigned int i = 0; i < m_triangles.size(); ++i) {
    auto &triangle = m_triangles[i];
    add_neighbour(i, triangle.a, triangle.b);
    add_neighbour(i, triangle.b, triangle.c);
    add_neighbour(i, triangle.c, triangle.a);
    assert(triangle.neighbours.size() > 0 && triangle.neighbours.size() <= 3 &&
           "triangle has 1, 2 or 3 neighbours");
  }

  std::cout << "NavMesh: vertices " << m_vertices.size() << ", triangles "
            << m_triangles.size() << std::endl;
}

void NavMesh::render(Shader &shader, const Camera &camera) const {
  // used only for testing
  std::vector<unsigned int> indices;
  for (const auto &t : m_triangles) {
    indices.push_back(t.a);
    indices.push_back(t.b);

    indices.push_back(t.b);
    indices.push_back(t.c);

    indices.push_back(t.c);
    indices.push_back(t.a);
  }

  GLuint vao, vbo, ebo;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_vertices.size(),
               m_vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                        (const GLvoid *)0);

  glEnableVertexAttribArray(0);

  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(),
               indices.data(), GL_STATIC_DRAW);

  shader.activate();

  glUniform3f(glGetUniformLocation(shader.id(), "Color"), 0, 1, 0);

  shader.set_uniform("camMatrix", camera.matrix());
  glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);

  // unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

glm::vec3 NavMesh::mid_point_common_edge(unsigned int i, unsigned int j) const {
  // find the middle point of the common edge of triangles with ids
  // i and j
  const auto &t1 = m_triangles[i];
  const auto &t2 = m_triangles[j];

  if (t1.a != t2.a && t1.a != t2.b && t1.a != t2.c) {
    assert(t1.b == t2.a || t1.b == t2.b ||
           t1.b == t2.c && "b should be common point");
    assert(t1.c == t2.a || t1.c == t2.b ||
           t1.c == t2.c && "c should be common point");
    return (m_vertices[t1.b] + m_vertices[t1.c]) / 2.0f;
  }

  if (t1.b != t2.a && t1.b != t2.b && t1.b != t2.c) {
    assert(t1.a == t2.a || t1.a == t2.b ||
           t1.a == t2.c && "a should be common point");
    assert(t1.c == t2.a || t1.c == t2.b ||
           t1.c == t2.c && "c should be common point");
    return (m_vertices[t1.a] + m_vertices[t1.c]) / 2.0f;
  }

  assert(t1.c != t2.a && t1.c != t2.b && t1.c != t2.c &&
         "c shouldn't be common point");
  assert(t1.a == t2.a || t1.a == t2.b ||
         t1.a == t2.c && "a should be common point");
  assert(t1.b == t2.a || t1.b == t2.b ||
         t1.b == t2.c && "b should be common point");
  return (m_vertices[t1.a] + m_vertices[t1.b]) / 2.0f;
}

bool NavMesh::is_point_in_triangle(unsigned int i,
                                   const glm::vec3 &point) const {
  // return true if point is in triangle with id i

  // point is in triangle ABC iff all vectors AB x AP, BC x BP and CA x CP
  // point in the same direction

  const auto &t = m_triangles[i];
  // get triangle vertices
  const auto &A = m_vertices[t.a];
  const auto &B = m_vertices[t.b];
  const auto &C = m_vertices[t.c];

  // project point to plane where nav mesh is y=0
  auto P = glm::vec3(point[0], A[1], point[2]);

  // AB x AP
  if (glm::cross(B - A, P - A)[1] <= 0) {
    return
        // BC x BP
        glm::cross(C - B, P - B)[1] <= 0 &&
        // CA x CP
        glm::cross(A - C, P - C)[1] <= 0;
  }

  // AB x AP is >=0
  return
      // BC x BP
      glm::cross(C - B, P - B)[1] >= 0 &&
      // CA x CP
      glm::cross(A - C, P - C)[1] >= 0;
}

std::optional<unsigned int>
NavMesh::get_triangle_for_point(const glm::vec3 &point) const {
  // return triangle id of triangle that contains the given point
  // if no such triangle exists return null

  for (unsigned int i = 0; i < m_triangles.size(); ++i) {
    if (is_point_in_triangle(i, point)) {
      return i;
    }
  }

  return std::nullopt;
}

std::vector<unsigned int>
NavMesh::get_triangle_path(const glm::vec3 &src, const glm::vec3 &dest) const {
  // get array of triangle ids that form a path from src ponint to dest point
  std::vector<unsigned int> path;
  auto maybe_src_traingle = get_triangle_for_point(src);
  if (!maybe_src_traingle) {
    assert(false && "source point not in triangle");
    // no such path exists
    return path;
  }
  auto maybe_dest_traingle = get_triangle_for_point(dest);
  if (!maybe_dest_traingle) {
    assert(false && "destination point not in triangle");
    // no such path exists
    return path;
  }

  // do BFS to find a path (TODO: A* is faster)
  unsigned int src_triangle = *maybe_src_traingle;
  unsigned int dest_triangle = *maybe_dest_traingle;

  std::unordered_map<unsigned int, unsigned int> parent;

  std::unordered_set<unsigned int> visited;
  std::queue<unsigned int> queue;
  queue.emplace(src_triangle);
  visited.emplace(src_triangle);

  unsigned int current;
  while (!queue.empty()) {
    current = queue.front();
    queue.pop();
    if (current == dest_triangle) {
      break;
    }

    for (unsigned int i : m_triangles[current].neighbours) {
      if (visited.find(i) == visited.end()) {
        // not yet visited
        visited.emplace(i);
        queue.emplace(i);
        parent[i] = current;
      }
    }
  }

  assert(current == dest_triangle && "path is found");

  while (1) {
    path.push_back(current);
    auto parent_it = parent.find(current);
    if (parent_it == parent.end()) {
      break;
    }
    current = parent_it->second;
  }

  std::reverse(path.begin(), path.end());
  return path;
}

Path NavMesh::get_path(const glm::vec3 &src, const glm::vec3 &dest) const {
  // get path that begins in src point and ends in dest point
  auto triangles_points = get_triangle_path(src, dest);
  if (triangles_points.empty()) {
    // path doesn't exist
    return {};
  }

  // first find points that will form a path
  // points: src, mid point of triangles 0 and 1, mid point of triangles 1 and
  // 2, ..., mid point of triangles n-2 and n-1, dest
  std::vector<glm::vec3> points;
  points.push_back(src);
  for (int i = 0; i < triangles_points.size() - 1; ++i) {
    auto mid_point =
        mid_point_common_edge(triangles_points[i], triangles_points[i + 1]);
    mid_point[1] = src[1];
    points.push_back(std::move(mid_point));
  }
  points.push_back(dest);

  // path contains linear (segment) and quadratic bezier curves:
  // linear (P0, point on 2/3 of [P0, P1] segment)
  // quadratic (point on 2/3 of [P0, P1] segment, P1, point on 1/3 of [P1, P2]
  // segment)
  // linear (point on 1/3 of [P1, P2] segment, point on 2/3 of [P1, P2] segment)
  // quadratic (point on 2/3 of [P1, P2] segment, P2, point on 1/3 of [P2, P3]
  // segment)
  // ...
  // linear (point on 1/3 of [Pn-2, Pn-1] segment, Pn-1)
  std::vector<Bezier> curves;
  if (points.size() == 2) {
    curves.emplace_back(points[0], points[1]);
  } else {
    auto Prev = points[0];
    for (int i = 1; i < points.size() - 1; ++i) {
      auto P0 =
          points[i - 1] + (float)(2.0 / 3.0) * (points[i] - points[i - 1]);
      auto P2 = points[i] + (float)(1.0 / 3.0) * (points[i + 1] - points[i]);
      curves.emplace_back(Prev, P0);
      curves.emplace_back(P0, points[i], P2);
      Prev = P2;
    }
    curves.emplace_back(Prev, points.back());
  }

  return Path(std::move(curves));
}

glm::vec3 NavMesh::get_random_point() const {
  // use current time as seed for random generator
  std::srand(std::time(nullptr));
  unsigned int random_triangle_id =
      std::rand() / ((RAND_MAX + 1u) / m_triangles.size());
  assert(random_triangle_id >= 0 && random_triangle_id < m_triangles.size() &&
         "random triangle id is valid");

  const auto &t = m_triangles[random_triangle_id];
  // get triangle vertices
  const auto &A = m_vertices[t.a];
  const auto &B = m_vertices[t.b];
  const auto &C = m_vertices[t.c];

  auto a = B - A;
  auto b = C - A;

  float u1 = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX));
  float u2 = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX));

  if (u1 + u2 > 1) {
    u1 = 1 - u1;
    u2 = 1 - u2;
  }

  auto point_in_triangle = u1 * a + u2 * b + A;
  assert(is_point_in_triangle(random_triangle_id, point_in_triangle) &&
         "random point is in triangle");

  return point_in_triangle;
}

// ------------------------- NavMesh End -------------------------------
