#include "Renderer.h"
#include "android_logger.h"
#include <GLES3/gl31.h>
#include <initializer_list>

const char gVertexShader[] = "attribute vec4 vPosition;\n"
                             "void main() {\n"
                             "  gl_Position = vPosition;\n"
                             "}\n";

const char gFragmentShader[] = "precision mediump float;\n"
                               "void main() {\n"
                               "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
                               "}\n";

const GLfloat vertices[] = {0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f};

// static GLuint loadShader(GLenum shaderType, const char *pSource) {
//   GLuint shader = glCreateShader(shaderType);
//   glShaderSource(shader, 1, &pSource, nullptr);
//   glCompileShader(shader);
//   GLint compiled;
//   glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
//   return shader;
// }

// static GLuint createProgram(const char *pVertexSource,
//                             const char *pFragmentSource) {
//   GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
//   GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
//   GLuint program = glCreateProgram();
//   glAttachShader(program, vertexShader);
//   glAttachShader(program, pixelShader);
//   glLinkProgram(program);
//   GLint linkStatus = GL_FALSE;
//   glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
//   return program;
// }

Renderer::Renderer() {
  // Check openGL on the system
  auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
  for (auto name : opengl_info) {
    auto info = glGetString(name);
    LOGI("OpenGL Info: %s", info);
  }

  //  program_ = createProgram(gVertexShader, gFragmentShader);
  //  position_ = glGetAttribLocation(program_, "vPosition");
}

Renderer::~Renderer() {}

int Renderer::major() const {
  GLint major;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  return major;
}

int Renderer::minor() const {
  GLint minor;
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  return minor;
}

void Renderer::draw(int width, int height) {
  glViewport(0, 0, width, height);
  glClearColor(1.0f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  //   glUseProgram(program_);
  //   glVertexAttribPointer(position_, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  //   glEnableVertexAttribArray(position_);
  //   glDrawArrays(GL_TRIANGLES, 0, 3);
}