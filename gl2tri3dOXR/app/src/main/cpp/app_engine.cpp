#include "app_engine.h"
#include "openxr/openxr.h"
#include "render_scene.h"
#include "util_egl.h"
#include "util_oxr.h"

AppEngine::AppEngine(android_app *app) : m_app(app) {}

AppEngine::~AppEngine() {}

/* ----------------------------------------------------------------------------
 * * Interfaces to android application framework
 * ----------------------------------------------------------------------------
 */
struct android_app *AppEngine::AndroidApp(void) const { return m_app; }

/* ----------------------------------------------------------------------------
 * * Initialize OpenXR with OpenGLES renderer
 * ----------------------------------------------------------------------------
 */
void AppEngine::InitOpenXR_GLES() {
  void *vm = m_app->activity->vm;
  void *clazz = m_app->activity->clazz;

  oxr_initialize_loader(vm, clazz);

  m_instance = oxr_create_instance(vm, clazz);
  m_systemId = oxr_get_system(m_instance);

  egl_init_with_pbuffer_surface(3, 24, 0, 0, 16, 16);
  oxr_confirm_gfx_requirements(m_instance, m_systemId);

  init_gles_scene();

  m_session = oxr_create_session(m_instance, m_systemId);
  m_appSpace = oxr_create_ref_space(m_session, XR_REFERENCE_SPACE_TYPE_LOCAL);
  m_stageSpace = oxr_create_ref_space(m_session, XR_REFERENCE_SPACE_TYPE_STAGE);

  m_viewSurface = oxr_create_viewsurface(m_instance, m_systemId, m_session);
}

/* ------------------------------------------------------------------------------------
 * * Update  Frame (Event handle, Render)
 * ------------------------------------------------------------------------------------
 */
void AppEngine::UpdateFrame() {
  bool exit_loop, req_restart;
  oxr_poll_events(m_instance, m_session, &exit_loop, &req_restart);

  if (!oxr_is_session_running()) {
    return;
  }

  std::vector<XrView> views;
  std::vector<XrCompositionLayerProjectionView> projLayerViews;
  XrTime dpy_time;
  auto viewCount = BeginFrame(&dpy_time, projLayerViews, views);
  /* Acquire Stage Location (rerative to the View Location) */
  XrSpaceLocation stageLoc{XR_TYPE_SPACE_LOCATION};
  xrLocateSpace(m_stageSpace, m_appSpace, dpy_time, &stageLoc);

  for (int i = 0; i < viewCount; ++i) {
    RenderLayer(dpy_time, stageLoc, i, projLayerViews[i], views[i]);
  }
  std::vector<XrCompositionLayerBaseHeader *> all_layers;
  EndFrame(dpy_time, projLayerViews);
}

/* ------------------------------------------------------------------------------------
 * * RenderFrame (Frame/Layer/View)
 * ------------------------------------------------------------------------------------
 */
int AppEngine::BeginFrame(
    XrTime *dpy_time,
    std::vector<XrCompositionLayerProjectionView> &projLayerViews,
    std::vector<XrView> &views) {
  oxr_begin_frame(m_session, dpy_time);

  /* Acquire View Location */
  uint32_t viewCount = (uint32_t)m_viewSurface.size();

  views.resize(viewCount);
  oxr_locate_views(m_session, *dpy_time, m_appSpace, &viewCount, views.data());

  projLayerViews.resize(viewCount);

  return viewCount;
}

void AppEngine::EndFrame(
    XrTime dpy_time,
    std::vector<XrCompositionLayerProjectionView> &projLayerViews) {
  XrCompositionLayerProjection projLayer;
  projLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  projLayer.space = m_appSpace;
  projLayer.viewCount = (uint32_t)projLayerViews.size();
  projLayer.views = projLayerViews.data();

  std::vector<XrCompositionLayerBaseHeader *> all_layers;
  all_layers.push_back(
      reinterpret_cast<XrCompositionLayerBaseHeader *>(&projLayer));

  /* Compose all layers */
  oxr_end_frame(m_session, dpy_time, all_layers);
}

void AppEngine::RenderLayer(XrTime dpy_time, XrSpaceLocation &stageLoc, int i,
                            XrCompositionLayerProjectionView &layerView,
                            XrView &view) {

  /* Render each view */
  XrSwapchainSubImage subImg;
  render_target_t rtarget;
  oxr_acquire_viewsurface(m_viewSurface[i], rtarget, subImg);

  layerView = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
  layerView.pose = view.pose;
  layerView.fov = view.fov;
  layerView.subImage = subImg;

  render_gles_scene(layerView, rtarget, stageLoc.pose, i);

  oxr_release_viewsurface(m_viewSurface[i]);
}
