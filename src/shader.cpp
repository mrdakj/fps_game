#include "shader.h"
#include <cerrno>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

std::string get_file_contents(const char *filename) {
  std::ifstream in(filename, std::ios::binary);
  if (in) {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return contents;
  }

  throw errno;
}

Shader::Shader(const char *vertexFile, const char *fragmentFile) {
  std::string vertex_code = get_file_contents(vertexFile);
  std::string fragment_code = get_file_contents(fragmentFile);

  const char *vertex_source = vertex_code.c_str();
  const char *fragment_source = fragment_code.c_str();

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_source, NULL);
  glCompileShader(vertex_shader);
  compile_errors(vertex_shader, "VERTEX");

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  // attach vertex shader source to the vertex shader object
  glShaderSource(fragment_shader, 1, &fragment_source, NULL);
  // compile the vertex shader into machine code
  glCompileShader(fragment_shader);
  compile_errors(fragment_shader, "FRAGMENT");

  m_id = glCreateProgram();
  glAttachShader(m_id, vertex_shader);
  glAttachShader(m_id, fragment_shader);
  // link all the shaders together into the shader program
  glLinkProgram(m_id);
  compile_errors(m_id, "PROGRAM");

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void Shader::activate() { glUseProgram(m_id); }

void Shader::del() { glDeleteProgram(m_id); }

void Shader::compile_errors(unsigned int shader, const char *type) {
  GLint hasCompiled;
  char infoLog[1024];
  if (std::string(type) != "PROGRAM") {
    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
    std::cout << "SHADER_COMPILEATION_ERROR for:" << type << std::endl;
    std::cout << infoLog << std::endl;
  } else {
    glGetProgramiv(shader, GL_COMPILE_STATUS, &hasCompiled);
    if (hasCompiled == GL_FALSE) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      std::cout << "SHADER_LINKING_ERROR for:" << type << std::endl;
      std::cout << infoLog << std::endl;
    }
  }
}

Shader::~Shader() { del(); }
