#ifndef _SHADER_H_
#define _SHADER_H_

#include <GL/glew.h>
#include <cassert>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

std::string get_file_contents(const char *filename);

class Shader {
public:
  Shader(const char *vertexFile, const char *fragmentFile);
  ~Shader();

  template <typename T>
  void set_uniform(const std::string &uniform_name, T &&value) {
    // make sure shader is activated before seting uniform
    GLint uniform_location = glGetUniformLocation(m_id, uniform_name.data());
    if (uniform_location == -1) {
      // if uniform is unused, glsl compiler will remove it
      // std::cout << "Uniform " << uniform_name << " not found or unused." <<
      // std::endl;
      return;
    }

    if constexpr (std::is_same_v<std::decay_t<T>, int>) {
      glUniform1i(uniform_location, value);
    } else if constexpr (std::is_same_v<std::decay_t<T>, unsigned int>) {
      glUniform1ui(uniform_location, value);
    } else if constexpr (std::is_same_v<std::decay_t<T>, glm::mat3>) {
      glUniformMatrix3fv(uniform_location, 1,
                         // don't need to transpose the matrix because glm is
                         // already column major
                         GL_FALSE, glm::value_ptr(std::forward<T>(value)));
    } else if constexpr (std::is_same_v<std::decay_t<T>, glm::mat4>) {
      glUniformMatrix4fv(uniform_location, 1,
                         // don't need to transpose the matrix because glm is
                         // already column major
                         GL_FALSE, glm::value_ptr(std::forward<T>(value)));
    } else if constexpr (std::is_same_v<std::decay_t<T>, glm::vec3>) {
      glUniform3f(uniform_location, value.x, value.y, value.z);
    } else if constexpr (std::is_same_v<std::decay_t<T>, glm::vec4>) {
      glUniform4f(uniform_location, value.x, value.y, value.z, value.w);
    } else {
      assert(false && "type unknown");
    }
  }

  void activate();
  void del();

  GLuint id() const { return m_id; }

private:
  void compile_errors(unsigned int shader, const char *type);

  GLuint m_id;
};

#endif /* _SHADER_H_ */
