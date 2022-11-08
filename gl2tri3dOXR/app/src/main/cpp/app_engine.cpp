#include "app_engine.h"
#include "openxr/openxr.h"
#include "util_egl.h"
#include "util_oxr.h"
#include <stdint.h>

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
}

void AppEngine::CreateSession() {
  m_session = oxr_create_session(m_instance, m_systemId);
  m_appSpace = oxr_create_ref_space(m_session, XR_REFERENCE_SPACE_TYPE_LOCAL);
  m_stageSpace = oxr_create_ref_space(m_session, XR_REFERENCE_SPACE_TYPE_STAGE);

  m_viewSurface = oxr_create_viewsurface(m_instance, m_systemId, m_session);
}

/* ------------------------------------------------------------------------------------
 * * Update  Frame (Event handle, Render)
 * ------------------------------------------------------------------------------------
 */
bool AppEngine::UpdateFrame() {
  bool exit_loop, req_restart;
  oxr_poll_events(m_instance, m_session, &exit_loop, &req_restart);
  if (!oxr_is_session_running()) {
    return false;
  }
  return true;
}

/* ------------------------------------------------------------------------------------
 * * RenderFrame (Frame/Layer/View)
 * ------------------------------------------------------------------------------------
 */
void AppEngine::BeginFrame(FrameInfo *frame) {
  oxr_begin_frame(m_session, &frame->time);

  /* Acquire View Location */
  frame->viewCount = (uint32_t)m_viewSurface.size();
  m_views.resize(frame->viewCount);

  oxr_locate_views(m_session, frame->time, m_appSpace, &frame->viewCount,
                   m_views.data());

  m_projLayerViews.resize(frame->viewCount);

  /* Acquire Stage Location (rerative to the View Location) */
  frame->stageLoc = {XR_TYPE_SPACE_LOCATION};
  xrLocateSpace(m_stageSpace, m_appSpace, frame->time, &frame->stageLoc);
}

void AppEngine::EndFrame(XrTime dpy_time) {
  XrCompositionLayerProjection projLayer;
  projLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  projLayer.space = m_appSpace;
  projLayer.viewCount = (uint32_t)m_projLayerViews.size();
  projLayer.views = m_projLayerViews.data();

  std::vector<XrCompositionLayerBaseHeader *> all_layers;
  all_layers.push_back(
      reinterpret_cast<XrCompositionLayerBaseHeader *>(&projLayer));

  /* Compose all layers */
  oxr_end_frame(m_session, dpy_time, all_layers);
}

void AppEngine::RenderLayer(const FrameInfo &frame, int i,
                            const RenderFunc &func) {

  m_projLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
  m_projLayerViews[i].pose = m_views[i].pose;
  m_projLayerViews[i].fov = m_views[i].fov;

  XrSwapchainSubImage subImg;
  subImg.swapchain = m_viewSurface[i].swapchain;
  subImg.imageRect.offset.x = 0;
  subImg.imageRect.offset.y = 0;
  subImg.imageRect.extent.width = m_viewSurface[i].width;
  subImg.imageRect.extent.height = m_viewSurface[i].height;
  subImg.imageArrayIndex = 0;
  m_projLayerViews[i].subImage = subImg;

  // uint32_t imgIdx = oxr_acquire_swapchain_img(m_viewSurface[i].swapchain);
  uint32_t imgIdx;
  XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
  xrAcquireSwapchainImage(m_viewSurface[i].swapchain, &acquireInfo, &imgIdx);

  XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
  waitInfo.timeout = XR_INFINITE_DURATION;
  xrWaitSwapchainImage(m_viewSurface[i].swapchain, &waitInfo);
  auto rtarget = m_viewSurface[i].rtarget_array[imgIdx];
  func(m_projLayerViews[i], rtarget, frame.stageLoc.pose);

  // oxr_release_viewsurface(m_viewSurface[i]);
  XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  xrReleaseSwapchainImage(m_viewSurface[i].swapchain, &releaseInfo);
}
