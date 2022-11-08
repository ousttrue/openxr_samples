#include "app_engine.h"
#include "openxr/openxr.h"
#include "util_egl.h"
#include "util_oxr.h"
#include <stdint.h>

std::vector<XrSwapchainImageOpenGLESKHR>
viewsurface::getSwapchainImages() const {
  uint32_t imgCnt;
  xrEnumerateSwapchainImages(swapchain, 0, &imgCnt, NULL);
  std::vector<XrSwapchainImageOpenGLESKHR> img_gles(imgCnt);
  for (uint32_t i = 0; i < imgCnt; i++) {
    img_gles[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
  }
  xrEnumerateSwapchainImages(swapchain, imgCnt, &imgCnt,
                             (XrSwapchainImageBaseHeader *)img_gles.data());
  return img_gles;
}

uint32_t viewsurface::acquireSwapchain() {
  projLayerView = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
  projLayerView.pose = view.pose;
  projLayerView.fov = view.fov;

  XrSwapchainSubImage subImg;
  subImg.swapchain = swapchain;
  subImg.imageRect.offset.x = 0;
  subImg.imageRect.offset.y = 0;
  subImg.imageRect.extent.width = width;
  subImg.imageRect.extent.height = height;
  subImg.imageArrayIndex = 0;
  projLayerView.subImage = subImg;

  XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
  waitInfo.timeout = XR_INFINITE_DURATION;
  xrWaitSwapchainImage(swapchain, &waitInfo);

  uint32_t imgIdx;
  XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
  xrAcquireSwapchainImage(swapchain, &acquireInfo, &imgIdx);
  return imgIdx;
}

void viewsurface::releaseSwapchain() const {
  // oxr_release_viewsurface(m_viewSurface[i]);
  XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  xrReleaseSwapchainImage(swapchain, &releaseInfo);
}

/* ----------------------------------------------------------------------------
 * * View operation
 * ----------------------------------------------------------------------------
 */
static XrViewConfigurationView *oxr_enumerate_viewconfig(XrInstance instance,
                                                         XrSystemId sysid,
                                                         uint32_t *numview) {
  uint32_t numConf;
  XrViewConfigurationView *conf;
  XrViewConfigurationType viewType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  xrEnumerateViewConfigurationViews(instance, sysid, viewType, 0, &numConf,
                                    NULL);

  conf = (XrViewConfigurationView *)calloc(sizeof(XrViewConfigurationView),
                                           numConf);
  for (uint32_t i = 0; i < numConf; i++)
    conf[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;

  xrEnumerateViewConfigurationViews(instance, sysid, viewType, numConf,
                                    &numConf, conf);

  LOGI("ViewConfiguration num: %d", numConf);
  for (uint32_t i = 0; i < numConf; i++) {
    XrViewConfigurationView &vp = conf[i];
    LOGI("ViewConfiguration[%d/%d]: MaxWH(%d, %d), MaxSample(%d)", i, numConf,
         vp.maxImageRectWidth, vp.maxImageRectHeight,
         vp.maxSwapchainSampleCount);
    LOGI("                        RecWH(%d, %d), RecSample(%d)",
         vp.recommendedImageRectWidth, vp.recommendedImageRectHeight,
         vp.recommendedSwapchainSampleCount);
  }

  *numview = numConf;
  return conf;
}

/* ----------------------------------------------------------------------------
 * * Swapchain operation
 * ----------------------------------------------------------------------------
 * *
 *
 *  --+-- view[0] -- viewSurface[0]
 *    |
 *    +-- view[1] -- viewSurface[1]
 *                   +----------------------------------------------+
 *                   | uint32_t    width, height                    |
 *                   | XrSwapchain swapchain                        |
 *                   | rtarget_array[0]: (fbo_id, texc_id, texz_id) |
 *                   | rtarget_array[1]: (fbo_id, texc_id, texz_id) |
 *                   | rtarget_array[2]: (fbo_id, texc_id, texz_id) |
 *                   +----------------------------------------------+
 *
 * ----------------------------------------------------------------------------
 */
static XrSwapchain oxr_create_swapchain(XrSession session, uint32_t width,
                                        uint32_t height) {
  XrSwapchainCreateInfo ci = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
  ci.usageFlags =
      XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
  ci.format = GL_RGBA8;
  ci.width = width;
  ci.height = height;
  ci.faceCount = 1;
  ci.arraySize = 1;
  ci.mipCount = 1;
  ci.sampleCount = 1;

  XrSwapchain swapchain;
  xrCreateSwapchain(session, &ci, &swapchain);

  return swapchain;
}

std::vector<viewsurface> oxr_create_viewsurface(XrInstance instance,
                                                XrSystemId sysid,
                                                XrSession session) {
  uint32_t viewCount;
  XrViewConfigurationView *conf_views =
      oxr_enumerate_viewconfig(instance, sysid, &viewCount);

  std::vector<viewsurface> sfcArray;
  for (uint32_t i = 0; i < viewCount; i++) {
    auto &vp = conf_views[i];
    LOGI("Swapchain for view %d: WH(%d, %d), SampleCount=%d", i,
         vp.recommendedImageRectWidth, vp.recommendedImageRectHeight,
         vp.recommendedSwapchainSampleCount);

    viewsurface sfc;
    sfc.width = vp.recommendedImageRectWidth;
    sfc.height = vp.recommendedImageRectHeight;
    sfc.swapchain = oxr_create_swapchain(session, sfc.width, sfc.height);
    sfcArray.push_back(sfc);
  }
  return sfcArray;
}

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
static bool oxr_locate_views(XrSession session, XrTime dpy_time, XrSpace space,
                             uint32_t view_cnt, XrView *view_array) {
  XrViewConfigurationType viewType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  XrViewState viewstat = {XR_TYPE_VIEW_STATE};
  uint32_t view_cnt_in = view_cnt;
  uint32_t view_cnt_out;

  XrViewLocateInfo vloc = {XR_TYPE_VIEW_LOCATE_INFO};
  vloc.viewConfigurationType = viewType;
  vloc.displayTime = dpy_time;
  vloc.space = space;
  xrLocateViews(session, &vloc, &viewstat, view_cnt_in, &view_cnt_out,
                view_array);

  if ((viewstat.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
      (viewstat.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
    return false; // There is no valid tracking poses for the views.
  }
  return true;
}

bool AppEngine::BeginFrame(XrPosef *stagePose) {
  oxr_begin_frame(m_session, &m_displayTime);

  /* Acquire View Location */
  std::vector<XrView> views(m_viewSurface.size());

  if (!oxr_locate_views(m_session, m_displayTime, m_appSpace, views.size(),
                        views.data())) {
    return false;
  }

  for (int i = 0; i < views.size(); ++i) {
    m_viewSurface[i].view = views[i];
  }

  /* Acquire Stage Location (rerative to the View Location) */
  XrSpaceLocation stageLoc{XR_TYPE_SPACE_LOCATION};
  xrLocateSpace(m_stageSpace, m_appSpace, m_displayTime, &stageLoc);
  *stagePose = stageLoc.pose;

  return true;
}

void AppEngine::EndFrame() {
  std::vector<XrCompositionLayerProjectionView> projLayerViews;
  for (auto view : m_viewSurface) {
    projLayerViews.push_back(view.projLayerView);
  }

  XrCompositionLayerProjection projLayer;
  projLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  projLayer.space = m_appSpace;
  projLayer.viewCount = (uint32_t)projLayerViews.size();
  projLayer.views = projLayerViews.data();

  std::vector<XrCompositionLayerBaseHeader *> all_layers;
  all_layers.push_back(
      reinterpret_cast<XrCompositionLayerBaseHeader *>(&projLayer));

  /* Compose all layers */
  oxr_end_frame(m_session, m_displayTime, all_layers);
}

viewsurface *AppEngine::GetView(size_t i) {
  if (i >= m_viewSurface.size()) {
    return nullptr;
  }
  auto view = &m_viewSurface[i];
  return view;
}
