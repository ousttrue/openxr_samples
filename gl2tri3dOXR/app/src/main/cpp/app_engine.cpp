#include "app_engine.h"
#include "android_native_app_glue.h"
#include "util_egl.h"
#include "util_log.h"
#include "util_render_target.h"
#include <GLES3/gl31.h>
#include <array>
#include <memory>
#include <stdint.h>
#include <string>

#define OXR_CHECK(func) oxr_check_errors(func, #func, __FILE__, __LINE__);

#define ENUM_CASE_STR(name, val)                                               \
  case name:                                                                   \
    return #name;
#define MAKE_TO_STRING_FUNC(enumType)                                          \
  inline const char *to_string(enumType e) {                                   \
    switch (e) {                                                               \
      XR_LIST_ENUM_##enumType(ENUM_CASE_STR) default                           \
          : return "Unknown " #enumType;                                       \
    }                                                                          \
  }

MAKE_TO_STRING_FUNC(XrReferenceSpaceType);
MAKE_TO_STRING_FUNC(XrViewConfigurationType);
MAKE_TO_STRING_FUNC(XrEnvironmentBlendMode);
MAKE_TO_STRING_FUNC(XrSessionState);
MAKE_TO_STRING_FUNC(XrResult);
MAKE_TO_STRING_FUNC(XrFormFactor);

/* ----------------------------------------------------------------------------
 * * Create OpenXR Instance with Android/OpenGLES binding
 * ----------------------------------------------------------------------------
 */

std::string oxr_get_runtime_name(XrInstance instance) {
  XrInstanceProperties prop = {XR_TYPE_INSTANCE_PROPERTIES};
  xrGetInstanceProperties(instance, &prop);

  char strbuf[128];
  snprintf(strbuf, 127, "%s (%u.%u.%u)", prop.runtimeName,
           XR_VERSION_MAJOR(prop.runtimeVersion),
           XR_VERSION_MINOR(prop.runtimeVersion),
           XR_VERSION_PATCH(prop.runtimeVersion));
  std::string runtime_name = strbuf;
  return runtime_name;
}

/* ----------------------------------------------------------------------------
 * * Get OpenXR Sysem
 * ----------------------------------------------------------------------------
 */
XrSystemId oxr_get_system(XrInstance instance) {
  XrSystemGetInfo sysInfo = {XR_TYPE_SYSTEM_GET_INFO};
  sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  XrSystemId sysid;
  // OXR_CHECK(
  xrGetSystem(instance, &sysInfo, &sysid);
  // );

  /* query system properties*/
  XrSystemProperties prop = {XR_TYPE_SYSTEM_PROPERTIES};
#if defined(USE_OXR_HANDTRACK)
  XrSystemHandTrackingPropertiesEXT handTrackProp{
      XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
  prop.next = &handTrackProp;
#endif
  xrGetSystemProperties(instance, sysid, &prop);

  LOGI("-----------------------------------------------------------------");
  LOGI("System Properties         : Name=\"%s\", VendorId=%x", prop.systemName,
       prop.vendorId);
  LOGI("System Graphics Properties: SwapchainMaxWH=(%d, %d), MaxLayers=%d",
       prop.graphicsProperties.maxSwapchainImageWidth,
       prop.graphicsProperties.maxSwapchainImageHeight,
       prop.graphicsProperties.maxLayerCount);
  LOGI("System Tracking Properties: Orientation=%d, Position=%d",
       prop.trackingProperties.orientationTracking,
       prop.trackingProperties.positionTracking);
#if defined(USE_OXR_HANDTRACK)
  LOGI("System HandTracking Props : %d", handTrackProp.supportsHandTracking);
#endif
  LOGI("-----------------------------------------------------------------");

  return sysid;
}

std::string oxr_get_system_name(XrInstance instance, XrSystemId sysid) {
  XrSystemProperties prop = {XR_TYPE_SYSTEM_PROPERTIES};
  xrGetSystemProperties(instance, sysid, &prop);

  std::string sys_name = prop.systemName;
  return sys_name;
}

/* ----------------------------------------------------------------------------
 * * Confirm OpenGLES version.
 * ----------------------------------------------------------------------------
 */
int oxr_confirm_gfx_requirements(XrInstance instance, XrSystemId sysid) {
  PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR;
  xrGetInstanceProcAddr(
      instance, "xrGetOpenGLESGraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)&xrGetOpenGLESGraphicsRequirementsKHR);

  XrGraphicsRequirementsOpenGLESKHR gfxReq = {
      XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
  xrGetOpenGLESGraphicsRequirementsKHR(instance, sysid, &gfxReq);

  GLint major, minor;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  XrVersion glver = XR_MAKE_VERSION(major, minor, 0);

  LOGI("GLES version: %" PRIx64 ", supported: (%" PRIx64 " - %" PRIx64 ")\n",
       glver, gfxReq.minApiVersionSupported, gfxReq.maxApiVersionSupported);

  if (glver < gfxReq.minApiVersionSupported ||
      glver > gfxReq.maxApiVersionSupported) {
    LOGE("GLES version %" PRIx64 " is not supported. (%" PRIx64 " - %" PRIx64
         ")\n",
         glver, gfxReq.minApiVersionSupported, gfxReq.maxApiVersionSupported);
    return -1;
  }

  return 0;
}

/* ----------------------------------------------------------------------------
 * * Frame operation
 * ----------------------------------------------------------------------------
 */
int oxr_begin_frame(XrSession session, XrTime *dpy_time) {
  XrFrameWaitInfo frameWait = {XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState frameState = {XR_TYPE_FRAME_STATE};
  xrWaitFrame(session, &frameWait, &frameState);

  XrFrameBeginInfo frameBegin = {XR_TYPE_FRAME_BEGIN_INFO};
  xrBeginFrame(session, &frameBegin);

  *dpy_time = frameState.predictedDisplayTime;
  return (int)frameState.shouldRender;
}

int oxr_end_frame(XrSession session, XrTime dpy_time,
                  std::vector<XrCompositionLayerBaseHeader *> &layers) {
  XrFrameEndInfo frameEnd{XR_TYPE_FRAME_END_INFO};
  frameEnd.displayTime = dpy_time;
  frameEnd.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  frameEnd.layerCount = (uint32_t)layers.size();
  frameEnd.layers = layers.data();
  xrEndFrame(session, &frameEnd);

  return 0;
}

/* ----------------------------------------------------------------------------
 * * Space operation
 * ----------------------------------------------------------------------------
 */
XrSpace oxr_create_ref_space(XrSession session,
                             XrReferenceSpaceType ref_space_type) {
  XrSpace space;
  XrReferenceSpaceCreateInfo ci = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  ci.referenceSpaceType = ref_space_type;
  ci.poseInReferenceSpace.orientation.w = 1;
  xrCreateReferenceSpace(session, &ci, &space);

  return space;
}

XrSpace oxr_create_action_space(XrSession session, XrAction action,
                                XrPath subpath) {
  XrSpace space;
  XrActionSpaceCreateInfo ci = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
  ci.action = action;
  ci.poseInActionSpace.orientation.w = 1.0f;
  ci.subactionPath = subpath;
  xrCreateActionSpace(session, &ci, &space);

  return space;
}

/* ----------------------------------------------------------------------------
 * * Action operation
 * ----------------------------------------------------------------------------
 */
XrActionSet oxr_create_actionset(XrInstance instance, const char *name,
                                 const char *local_name, int priority) {
  XrActionSet actionset;
  XrActionSetCreateInfo ci = {XR_TYPE_ACTION_SET_CREATE_INFO};
  ci.priority = priority;
  strcpy(ci.actionSetName, name);
  strcpy(ci.localizedActionSetName, local_name);
  xrCreateActionSet(instance, &ci, &actionset);

  return actionset;
}

XrAction oxr_create_action(XrActionSet actionset, XrActionType type,
                           const char *name, const char *local_name,
                           int subpath_num, XrPath *subpath_array) {
  XrAction action;
  XrActionCreateInfo ci = {XR_TYPE_ACTION_CREATE_INFO};
  ci.actionType = type;
  ci.countSubactionPaths = subpath_num;
  ci.subactionPaths = subpath_array;
  strncpy(ci.actionName, name, XR_MAX_ACTION_NAME_SIZE);
  strncpy(ci.localizedActionName, local_name,
          XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
  xrCreateAction(actionset, &ci, &action);

  return action;
}

XrPath oxr_str2path(XrInstance instance, const char *str) {
  XrPath path;
  xrStringToPath(instance, str, &path);
  return path;
}

int oxr_bind_interaction(XrInstance instance, const char *profile,
                         std::vector<XrActionSuggestedBinding> &bindings) {
  XrPath profPath;
  xrStringToPath(instance, profile, &profPath);

  XrInteractionProfileSuggestedBinding bind{
      XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
  bind.interactionProfile = profPath;
  bind.suggestedBindings = bindings.data();
  bind.countSuggestedBindings = (uint32_t)bindings.size();

  xrSuggestInteractionProfileBindings(instance, &bind);

  return 0;
}

int oxr_attach_actionsets(XrSession session, XrActionSet actionSet) {
  XrSessionActionSetsAttachInfo info{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  info.countActionSets = 1;
  info.actionSets = &actionSet;
  xrAttachSessionActionSets(session, &info);

  return 0;
}

int oxr_sync_actions(XrSession session, XrActionSet actionSet) {
  const XrActiveActionSet activeActionSet{actionSet, XR_NULL_PATH};

  XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
  syncInfo.countActiveActionSets = 1;
  syncInfo.activeActionSets = &activeActionSet;
  xrSyncActions(session, &syncInfo);

  return 0;
}

XrActionStateBoolean oxr_get_action_state_boolean(XrSession session,
                                                  XrAction action,
                                                  XrPath subpath) {
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
  getInfo.action = action;
  getInfo.subactionPath = subpath;

  XrActionStateBoolean stat{XR_TYPE_ACTION_STATE_BOOLEAN};
  xrGetActionStateBoolean(session, &getInfo, &stat);

  return stat;
}

XrActionStateFloat oxr_get_action_state_float(XrSession session,
                                              XrAction action, XrPath subpath) {
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
  getInfo.action = action;
  getInfo.subactionPath = subpath;

  XrActionStateFloat stat{XR_TYPE_ACTION_STATE_FLOAT};
  xrGetActionStateFloat(session, &getInfo, &stat);

  return stat;
}

XrActionStatePose oxr_get_action_state_pose(XrSession session, XrAction action,
                                            XrPath subpath) {
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
  getInfo.action = action;
  getInfo.subactionPath = subpath;

  XrActionStatePose stat{XR_TYPE_ACTION_STATE_POSE};
  xrGetActionStatePose(session, &getInfo, &stat);

  return stat;
}

XrActionStateVector2f oxr_get_action_state_vector2(XrSession session,
                                                   XrAction action,
                                                   XrPath subpath) {
  XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
  getInfo.action = action;
  getInfo.subactionPath = subpath;

  XrActionStateVector2f stat = {XR_TYPE_ACTION_STATE_VECTOR2F};
  xrGetActionStateVector2f(session, &getInfo, &stat);

  return stat;
}

int oxr_apply_haptic_feedback_vibrate(XrSession session, XrAction action,
                                      XrPath subpath, XrDuration dura,
                                      float freq, float amp) {
  XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
  vibration.duration = dura;
  vibration.frequency = freq;
  vibration.amplitude = amp;

  XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};
  info.action = action;
  info.subactionPath = subpath;
  xrApplyHapticFeedback(session, &info, (XrHapticBaseHeader *)&vibration);

  return 0;
}

/* ----------------------------------------------------------------------------
 * * Session operation
 * ----------------------------------------------------------------------------
 */

XrSession oxr_create_session(XrInstance instance, XrSystemId sysid) {
  XrGraphicsBindingOpenGLESAndroidKHR gfxBind = {
      XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
  gfxBind.display = egl_get_display();
  gfxBind.config = egl_get_config();
  gfxBind.context = egl_get_context();

  XrSessionCreateInfo ci = {XR_TYPE_SESSION_CREATE_INFO};
  ci.next = &gfxBind;
  ci.systemId = sysid;

  XrSession session;
  xrCreateSession(instance, &ci, &session);

  return session;
}

int oxr_begin_session(XrSession session) {
  XrViewConfigurationType viewType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};
  bi.primaryViewConfigurationType = viewType;
  xrBeginSession(session, &bi);

  return 0;
}

int AppEngine::oxr_handle_session_state_changed(
    XrSession session, XrEventDataSessionStateChanged &ev, bool *exitLoop,
    bool *reqRestart) {
  XrSessionState old_state = m_session_state;
  XrSessionState new_state = ev.state;
  m_session_state = new_state;

  LOGI("  [SessionState]: %s -> %s (session=%p, time=%ld)",
       to_string(old_state), to_string(new_state), ev.session, ev.time);

  if ((ev.session != XR_NULL_HANDLE) && (ev.session != session)) {
    LOGE("XrEventDataSessionStateChanged for unknown session");
    return -1;
  }

  switch (new_state) {
  case XR_SESSION_STATE_READY:
    oxr_begin_session(session);
    m_session_running = true;
    break;

  case XR_SESSION_STATE_STOPPING:
    xrEndSession(session);
    m_session_running = false;
    break;

  case XR_SESSION_STATE_EXITING:
    *exitLoop = true;
    *reqRestart =
        false; // Do not attempt to restart because user closed this session.
    break;

  case XR_SESSION_STATE_LOSS_PENDING:
    *exitLoop = true;
    *reqRestart = true; // Poll for a new instance.
    break;

  default:
    break;
  }
  return 0;
}

static XrEventDataBuffer s_evDataBuf;

static XrEventDataBaseHeader *oxr_poll_event(XrInstance instance,
                                             XrSession session) {
  XrEventDataBaseHeader *ev =
      reinterpret_cast<XrEventDataBaseHeader *>(&s_evDataBuf);
  *ev = {XR_TYPE_EVENT_DATA_BUFFER};

  XrResult xr = xrPollEvent(instance, &s_evDataBuf);
  if (xr == XR_EVENT_UNAVAILABLE)
    return nullptr;

  if (xr != XR_SUCCESS) {
    LOGE("xrPollEvent");
    return NULL;
  }

  if (ev->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
    XrEventDataEventsLost *evLost =
        reinterpret_cast<XrEventDataEventsLost *>(ev);
    LOGW("%p events lost", evLost);
  }
  return ev;
}

int AppEngine::oxr_poll_events(XrInstance instance, XrSession session,
                               bool *exit_loop, bool *req_restart) {
  *exit_loop = false;
  *req_restart = false;

  // Process all pending messages.
  while (XrEventDataBaseHeader *ev = oxr_poll_event(instance, session)) {
    switch (ev->type) {
    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
      LOGW("XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING");
      *exit_loop = true;
      *req_restart = true;
      return -1;
    }

    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
      LOGW("XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED");
      XrEventDataSessionStateChanged sess_ev =
          *(XrEventDataSessionStateChanged *)ev;
      oxr_handle_session_state_changed(session, sess_ev, exit_loop,
                                       req_restart);
      break;
    }

    case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      LOGW("XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED");
      break;

    case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
      LOGW("XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING");
      break;

    default:
      LOGE("Unknown event type %d", ev->type);
      break;
    }
  }
  return 0;
}

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

void viewsurface::createBackbuffers() {
  auto img_gles = getSwapchainImages();
  for (uint32_t j = 0; j < img_gles.size(); j++) {
    GLuint tex_c = img_gles[j].image;
    auto render_target = render_target::create(tex_c, width, height);
    assert(render_target);
    backbuffers.push_back(render_target);
  }
}

std::shared_ptr<render_target> viewsurface::acquireSwapchain() {
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
  return backbuffers[imgIdx];
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

/* ----------------------------------------------------------------------------
 * * Error handling
 * ----------------------------------------------------------------------------
 */
void AppEngine::oxr_check_errors(XrResult ret, const char *func,
                                 const char *fname, int line) {
  if (XR_FAILED(ret)) {
    char errbuf[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(m_instance, ret, errbuf);

    LOGE("[OXR ERROR] %s(%d):%s: %s\n", fname, line, func, errbuf);
  }
}

/* ----------------------------------------------------------------------------
 * * Initialize OpenXR with OpenGLES renderer
 * ----------------------------------------------------------------------------
 */
void AppEngine::InitOpenXR_GLES(struct android_app *app) {
  // initialize loader
  PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
  xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                        (PFN_xrVoidFunction *)&xrInitializeLoaderKHR);
  XrLoaderInitInfoAndroidKHR info = {
      .type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
      .applicationVM = app->activity->vm,
      .applicationContext = app->activity->clazz,
  };
  OXR_CHECK(xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&info));

  // enumerate extension properties
  // uint32_t ext_count = 0;
  // OXR_CHECK(xrEnumerateInstanceExtensionProperties(NULL, 0, &ext_count,
  // NULL)); XrExtensionProperties extProps[ext_count]; for (uint32_t i = 0; i <
  // ext_count; i++) {
  //   extProps[i].type = XR_TYPE_EXTENSION_PROPERTIES;
  //   extProps[i].next = NULL;
  // }
  // OXR_CHECK(xrEnumerateInstanceExtensionProperties(NULL, ext_count,
  // &ext_count,
  //                                                  extProps));
  // for (uint32_t i = 0; i < ext_count; i++) {
  //   LOGI("InstanceExt[%2d/%2d]: %s\n", i, ext_count,
  //   extProps[i].extensionName);
  // }

  // create instance
  XrInstanceCreateInfoAndroidKHR ciAndroid = {
      .type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
      .applicationVM = app->activity->vm,
      .applicationActivity = app->activity->clazz,
  };
  std::array<const char *, 2> extensions = {
      "XR_KHR_android_create_instance",
      "XR_KHR_opengl_es_enable",
  };
  XrInstanceCreateInfo ci = {
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .next = &ciAndroid,
      .applicationInfo =
          {
              .apiVersion = XR_CURRENT_API_VERSION,
          },
      .enabledExtensionCount = (uint32_t)extensions.size(),
      .enabledExtensionNames = extensions.data(),
  };
  strncpy(ci.applicationInfo.applicationName, "OXR_GLES_APP",
          XR_MAX_ENGINE_NAME_SIZE - 1);
  OXR_CHECK(xrCreateInstance(&ci, &m_instance));

  // query instance name, version
  XrInstanceProperties prop = {.type = XR_TYPE_INSTANCE_PROPERTIES};
  xrGetInstanceProperties(m_instance, &prop);
  LOGI("OpenXR Instance Runtime   : \"%s\", Version: %u.%u.%u",
       prop.runtimeName, XR_VERSION_MAJOR(prop.runtimeVersion),
       XR_VERSION_MINOR(prop.runtimeVersion),
       XR_VERSION_PATCH(prop.runtimeVersion));

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
