#pragma once

namespace render_interface {

struct float3 {
  float x, y, z;
};
struct quat {
  float x, y, z, w;
};
struct pose {
  quat rotation;
  float3 position;
};
struct rect {
  int x, y, width, height;
};
struct projection {
  float left, right, up, down;
};

struct ViewInfo {
  pose stage;
  rect viewport;
  projection projection;
  pose view;
};

} // namespace render_interface
