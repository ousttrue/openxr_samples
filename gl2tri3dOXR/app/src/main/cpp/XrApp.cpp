#include "XrApp.h"
#include "android_native_app_glue.h"
#include "util_egl.h"
#include "util_log.h"
#include "util_render_target.h"
#include <GLES3/gl31.h>
#include <array>
#include <assert.h>
#include <memory>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
#include <string>
#include <vector>

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

static_assert(sizeof(XrPosef) == sizeof(render_interface::pose), "pose");

MAKE_TO_STRING_FUNC(XrReferenceSpaceType);
MAKE_TO_STRING_FUNC(XrViewConfigurationType);
MAKE_TO_STRING_FUNC(XrEnvironmentBlendMode);
MAKE_TO_STRING_FUNC(XrSessionState);
MAKE_TO_STRING_FUNC(XrResult);
MAKE_TO_STRING_FUNC(XrFormFactor);

struct viewsurface {
  XrSwapchain swapchain;
  uint32_t width;
  uint32_t height;

  XrView view;
  XrCompositionLayerProjectionView projLayerView;

  std::vector<std::shared_ptr<render_target>> backbuffers;

  std::vector<XrSwapchainImageOpenGLESKHR> getSwapchainImages() const {
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

  void createBackbuffers() {
    auto img_gles = getSwapchainImages();
    for (uint32_t j = 0; j < img_gles.size(); j++) {
      GLuint tex_c = img_gles[j].image;
      auto render_target = render_target::create(tex_c, width, height);
      assert(render_target);
      backbuffers.push_back(render_target);
    }
  }

  std::shared_ptr<render_target> acquireSwapchain() {
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
    XrSwapchainImageAcquireInfo acquireInfo{
        XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    xrAcquireSwapchainImage(swapchain, &acquireInfo, &imgIdx);
    return backbuffers[imgIdx];
  }

  void releaseSwapchain() const {
    // oxr_release_viewsurface(m_viewSurface[i]);
    XrSwapchainImageReleaseInfo releaseInfo{
        XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrReleaseSwapchainImage(swapchain, &releaseInfo);
  }
};

struct XrAppImpl {
  XrInstance m_instance;
  XrSession m_session;
  XrSystemId m_systemId;

  XrSessionState m_session_state = XR_SESSION_STATE_UNKNOWN;

  XrSpace m_appSpace;
  XrSpace m_stageSpace;
  std::vector<std::shared_ptr<struct viewsurface>> m_viewSurface;

  XrTime m_displayTime;

  XrEventDataBuffer m_evDataBuf;

  void oxr_check_errors(XrResult ret, const char *func, const char *fname,
                        int line) {
    if (XR_FAILED(ret)) {
      char errbuf[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString(m_instance, ret, errbuf);

      LOGE("[OXR ERROR] %s(%d):%s: %s\n", fname, line, func, errbuf);
    }
  }

  int oxr_handle_session_state_changed(XrSession session,
                                       XrEventDataSessionStateChanged &ev,
                                       bool *exitLoop, bool *reqRestart) {
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
    case XR_SESSION_STATE_READY: {
      XrSessionBeginInfo bi{
          .type = XR_TYPE_SESSION_BEGIN_INFO,
          .primaryViewConfigurationType =
              XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
      };
      xrBeginSession(session, &bi);
      m_session_running = true;
    } break;

    case XR_SESSION_STATE_STOPPING: {
      xrEndSession(session);
      m_session_running = false;
    } break;

    case XR_SESSION_STATE_EXITING: {
      *exitLoop = true;
      *reqRestart =
          false; // Do not attempt to restart because user closed this session.
    } break;

    case XR_SESSION_STATE_LOSS_PENDING: {
      *exitLoop = true;
      *reqRestart = true; // Poll for a new instance.
    } break;

    default:
      break;
    }
    return 0;
  }

  XrEventDataBaseHeader *oxr_poll_event(XrInstance instance,
                                        XrSession session) {
    XrEventDataBaseHeader *ev =
        reinterpret_cast<XrEventDataBaseHeader *>(&m_evDataBuf);
    *ev = {XR_TYPE_EVENT_DATA_BUFFER};

    XrResult xr = xrPollEvent(instance, &m_evDataBuf);
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

  int oxr_poll_events(XrInstance instance, XrSession session, bool *exit_loop,
                      bool *req_restart) {
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

  uint32_t SwapchainIndex() const;

  bool m_session_running = false;

  ///--------------------------------------------------------------------------
  /// Initialize OpenXR with OpenGLES renderer
  ///--------------------------------------------------------------------------
  bool CreateInstance(struct android_app *app) {
    // initialize loader
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                          (PFN_xrVoidFunction *)&xrInitializeLoaderKHR);
    XrLoaderInitInfoAndroidKHR info = {
        .type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
        .applicationVM = app->activity->vm,
        .applicationContext = app->activity->clazz,
    };
    if (XR_FAILED(
            xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&info))) {
      return false;
    }

    // enumerate extension properties
    // uint32_t ext_count = 0;
    // OXR_CHECK(xrEnumerateInstanceExtensionProperties(NULL, 0, &ext_count,
    // NULL)); XrExtensionProperties extProps[ext_count]; for (uint32_t i = 0; i
    // < ext_count; i++) {
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
    if (XR_FAILED(xrCreateInstance(&ci, &m_instance))) {
      return false;
    }

    // // query instance name, version
    // XrInstanceProperties instanceProp = {.type =
    // XR_TYPE_INSTANCE_PROPERTIES}; xrGetInstanceProperties(m_instance,
    // &instanceProp); LOGI("OpenXR Instance Runtime   : \"%s\", Version:
    // %u.%u.%u",
    //      instanceProp.runtimeName,
    //      XR_VERSION_MAJOR(instanceProp.runtimeVersion),
    //      XR_VERSION_MINOR(instanceProp.runtimeVersion),
    //      XR_VERSION_PATCH(instanceProp.runtimeVersion));

    XrSystemGetInfo sysInfo = {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };
    if (XR_FAILED(xrGetSystem(m_instance, &sysInfo, &m_systemId))) {
      return false;
    }

    // // query system properties
    // XrSystemProperties systemProp = {
    //     .type = XR_TYPE_SYSTEM_PROPERTIES,
    // };
    // xrGetSystemProperties(m_instance, m_systemId, &systemProp);
    // LOGI("-----------------------------------------------------------------");
    // LOGI("System Properties         : Name=\"%s\", VendorId=%x",
    // systemProp.systemName,
    //      systemProp.vendorId);
    // LOGI("System Graphics Properties: SwapchainMaxWH=(%d, %d), MaxLayers=%d",
    //      systemProp.graphicsProperties.maxSwapchainImageWidth,
    //      systemProp.graphicsProperties.maxSwapchainImageHeight,
    //      systemProp.graphicsProperties.maxLayerCount);
    // LOGI("System Tracking Properties: Orientation=%d, Position=%d",
    //      systemProp.trackingProperties.orientationTracking,
    //      systemProp.trackingProperties.positionTracking);
    // LOGI("-----------------------------------------------------------------");

    return true;
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
  bool oxr_create_viewsurface() {
    uint32_t numConf;
    XrViewConfigurationType viewType =
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    if (XR_FAILED(xrEnumerateViewConfigurationViews(
            m_instance, m_systemId, viewType, 0, &numConf, NULL))) {
      return false;
    }

    std::vector<XrViewConfigurationView> conf(numConf);
    for (uint32_t i = 0; i < numConf; i++) {
      conf[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
    }
    if (XR_FAILED(xrEnumerateViewConfigurationViews(m_instance, m_systemId,
                                                    viewType, numConf, &numConf,
                                                    conf.data()))) {
      return false;
    }

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

    for (uint32_t i = 0; i < numConf; i++) {
      auto &vp = conf[i];
      LOGI("Swapchain for view %d: WH(%d, %d), SampleCount=%d", i,
           vp.recommendedImageRectWidth, vp.recommendedImageRectHeight,
           vp.recommendedSwapchainSampleCount);
      auto sfc = std::make_shared<viewsurface>();
      sfc->width = vp.recommendedImageRectWidth;
      sfc->height = vp.recommendedImageRectHeight;
      XrSwapchainCreateInfo ci = {
          .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
          .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                        XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
          .format = GL_RGBA8,
          .sampleCount = 1,
          .width = sfc->width,
          .height = sfc->height,
          .faceCount = 1,
          .arraySize = 1,
          .mipCount = 1,
      };
      if (XR_FAILED(xrCreateSwapchain(m_session, &ci, &sfc->swapchain))) {
        return false;
      }

      m_viewSurface.push_back(sfc);
    }

    return true;
  }

  bool CreateSession(int major, int minor) {
    // oxr_confirm_gfx_requirements(m_instance, m_systemId);
    PFN_xrGetOpenGLESGraphicsRequirementsKHR
        xrGetOpenGLESGraphicsRequirementsKHR;
    xrGetInstanceProcAddr(
        m_instance, "xrGetOpenGLESGraphicsRequirementsKHR",
        (PFN_xrVoidFunction *)&xrGetOpenGLESGraphicsRequirementsKHR);

    XrGraphicsRequirementsOpenGLESKHR gfxReq = {
        .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR,
    };
    if (XR_FAILED(xrGetOpenGLESGraphicsRequirementsKHR(m_instance, m_systemId,
                                                       &gfxReq))) {
      return false;
    }

    XrVersion glver = XR_MAKE_VERSION(major, minor, 0);
    LOGI("GLES version: %" PRIx64 ", supported: (%" PRIx64 " - %" PRIx64 ")\n",
         glver, gfxReq.minApiVersionSupported, gfxReq.maxApiVersionSupported);
    if (glver < gfxReq.minApiVersionSupported ||
        glver > gfxReq.maxApiVersionSupported) {
      LOGE("GLES version %" PRIx64 " is not supported. (%" PRIx64 " - %" PRIx64
           ")\n",
           glver, gfxReq.minApiVersionSupported, gfxReq.maxApiVersionSupported);
      return false;
    }

    XrGraphicsBindingOpenGLESAndroidKHR gfxBind = {
        .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
        .display = egl_get_display(),
        .config = egl_get_config(),
        .context = egl_get_context(),
    };
    XrSessionCreateInfo ci = {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = &gfxBind,
        .systemId = m_systemId,
    };
    if (XR_FAILED(xrCreateSession(m_instance, &ci, &m_session))) {
      return false;
    }

    {
      XrReferenceSpaceCreateInfo ci = {
          .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
          .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
          .poseInReferenceSpace = {
              .orientation = {0, 0, 0, 1},
              .position = {0, 0, 0},
          }};
      xrCreateReferenceSpace(m_session, &ci, &m_appSpace);
    }
    {
      XrReferenceSpaceCreateInfo ci = {
          .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
          .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
          .poseInReferenceSpace = {
              .orientation = {0, 0, 0, 1},
              .position = {0, 0, 0},
          }};
      xrCreateReferenceSpace(m_session, &ci, &m_stageSpace);
    }

    oxr_create_viewsurface();

    // create backbuffer
    for (auto &view : m_viewSurface) {
      view->createBackbuffers();
      //   LOGI("SwapchainImage[%d/%d] FBO:%d, TEXC:%d, TEXZ:%d, WH(%d, %d)", i,
      //        imgCnt, rtarget->fbo_id, rtarget->texc_id, rtarget->texz_id,
      //        sfc->width, sfc->height);
    }
    return true;
  }

  /* ------------------------------------------------------------------------------------
   * * Update  Frame (Event handle, Render)
   * ------------------------------------------------------------------------------------
   */
  bool UpdateFrame() {
    bool exit_loop, req_restart;
    oxr_poll_events(m_instance, m_session, &exit_loop, &req_restart);
    if (!m_session_running) {
      return false;
    }
    return true;
  }

  render_interface::ViewInfo info_;

  void Render(const RenderFunc &func) {
    if (UpdateFrame()) {
      if (BeginFrame((XrPosef *)&info_.stage)) {
        for (auto &view : m_viewSurface) {
          info_.view = *((render_interface::pose *)&view->projLayerView.pose);
          info_.projection =
              *((render_interface::projection *)&view->projLayerView.fov);
          auto renderTarget = view->acquireSwapchain();
          info_.viewport.x = view->projLayerView.subImage.imageRect.offset.x;
          info_.viewport.y = view->projLayerView.subImage.imageRect.offset.y;
          info_.viewport.width =
              view->projLayerView.subImage.imageRect.extent.width;
          info_.viewport.height =
              view->projLayerView.subImage.imageRect.extent.height;
          func(renderTarget->fbo_id, info_);
          view->releaseSwapchain();
        }
        EndFrame();
      }
    }
  }

  bool BeginFrame(XrPosef *stagePose) {
    XrFrameWaitInfo frameWait = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState = {XR_TYPE_FRAME_STATE};
    xrWaitFrame(m_session, &frameWait, &frameState);
    XrFrameBeginInfo frameBegin = {XR_TYPE_FRAME_BEGIN_INFO};
    xrBeginFrame(m_session, &frameBegin);
    m_displayTime = frameState.predictedDisplayTime;
    if (!frameState.shouldRender) {
      return false;
    }

    // Acquire View Location
    std::vector<XrView> views(m_viewSurface.size());

    XrViewConfigurationType viewType =
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    XrViewState viewstat = {.type = XR_TYPE_VIEW_STATE};
    uint32_t view_cnt_out;
    XrViewLocateInfo vloc = {
        .type = XR_TYPE_VIEW_LOCATE_INFO,
        .viewConfigurationType = viewType,
        .displayTime = m_displayTime,
        .space = m_appSpace,
    };
    if (XR_FAILED(xrLocateViews(m_session, &vloc, &viewstat, views.size(),
                                &view_cnt_out, views.data()))) {
      return false;
    }
    if ((viewstat.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewstat.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
      return false; // There is no valid tracking poses for the views.
    }
    for (int i = 0; i < views.size(); ++i) {
      m_viewSurface[i]->view = views[i];
    }

    // Acquire Stage Location (rerative to the View Location)
    XrSpaceLocation stageLoc{XR_TYPE_SPACE_LOCATION};
    xrLocateSpace(m_stageSpace, m_appSpace, m_displayTime, &stageLoc);
    *stagePose = stageLoc.pose;

    return true;
  }

  void EndFrame() {
    std::vector<XrCompositionLayerProjectionView> projLayerViews;
    for (auto view : m_viewSurface) {
      projLayerViews.push_back(view->projLayerView);
    }

    XrCompositionLayerProjection projLayer;
    projLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projLayer.space = m_appSpace;
    projLayer.viewCount = (uint32_t)projLayerViews.size();
    projLayer.views = projLayerViews.data();

    std::vector<XrCompositionLayerBaseHeader *> all_layers;
    all_layers.push_back(
        reinterpret_cast<XrCompositionLayerBaseHeader *>(&projLayer));

    // Compose all layers
    XrFrameEndInfo frameEnd{
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = m_displayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = (uint32_t)all_layers.size(),
        .layers = all_layers.data(),
    };
    xrEndFrame(m_session, &frameEnd);
  }
};

///////////////////////////////////////////////////////////////////////////////
/// XrApp
///////////////////////////////////////////////////////////////////////////////
XrApp::XrApp() : impl_(new XrAppImpl) {}
XrApp::~XrApp() { delete impl_; }
bool XrApp::CreateInstance(struct android_app *app) {
  return impl_->CreateInstance(app);
}
bool XrApp::CreateSession_EGL(int major, int minor) {
  return impl_->CreateSession(major, minor);
}
bool XrApp::IsSessionRunning() const { return impl_->m_session_running; }
void XrApp::Render(const RenderFunc &func) { return impl_->Render(func); }
