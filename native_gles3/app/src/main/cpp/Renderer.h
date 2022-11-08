#pragma once
#include <GLES/gl.h>

class Renderer {
  GLuint program_;
  GLuint position_;

public:
  Renderer();
  ~Renderer();
  void draw(int width, int height);
};
