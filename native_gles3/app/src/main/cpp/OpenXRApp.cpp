#include "OpenXRApp.h"
#include "android_logger.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
#include <vector>

static void oxr_check_errors(XrInstance &instance, XrResult ret,
                             const char *func, const char *fname, int line) {
  if (XR_FAILED(ret)) {
    char errbuf[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(instance, ret, errbuf);

    LOGE("[OXR ERROR] %s(%d):%s: %s\n", fname, line, func, errbuf);
  }
}
#define OXR_CHECK(func)                                                        \
  oxr_check_errors(instance_, func, #func, __FILE__, __LINE__);

OpenXRApp::OpenXRApp() {}
OpenXRApp::~OpenXRApp() {}

void OpenXRApp::createInstance(struct android_app *app) {
  PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
  xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                        (PFN_xrVoidFunction *)&xrInitializeLoaderKHR);

  {
    XrLoaderInitInfoAndroidKHR info = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    info.applicationVM = app->activity->vm;
    info.applicationContext = app->activity->clazz;

    xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&info);
  }

  {
    std::vector<const char *> extensions;
    extensions.push_back("XR_KHR_android_create_instance");
    extensions.push_back("XR_KHR_opengl_es_enable");
#if defined(USE_OXR_HANDTRACK)
    extensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    extensions.push_back(XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME);
    extensions.push_back(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
    extensions.push_back(XR_FB_HAND_TRACKING_CAPSULES_EXTENSION_NAME);
#endif
#if defined(USE_OXR_PASSTHROUGH)
    extensions.push_back(XR_FB_PASSTHROUGH_EXTENSION_NAME);
#endif

    XrInstanceCreateInfoAndroidKHR ciAndroid = {
        XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    ciAndroid.applicationVM = app->activity->vm;
    ciAndroid.applicationActivity = app->activity->clazz;

    XrInstanceCreateInfo ci = {XR_TYPE_INSTANCE_CREATE_INFO};
    ci.next = &ciAndroid;
    ci.enabledExtensionCount = extensions.size();
    ci.enabledExtensionNames = extensions.data();
    ci.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strncpy(ci.applicationInfo.applicationName, "OXR_GLES_APP",
            XR_MAX_ENGINE_NAME_SIZE - 1);

    OXR_CHECK(xrCreateInstance(&ci, &instance_));

    // query instance name, version
    XrInstanceProperties prop = {XR_TYPE_INSTANCE_PROPERTIES};
    xrGetInstanceProperties(instance_, &prop);
    LOGI("OpenXR Instance Runtime   : \"%s\", Version: %u.%u.%u",
         prop.runtimeName, XR_VERSION_MAJOR(prop.runtimeVersion),
         XR_VERSION_MINOR(prop.runtimeVersion),
         XR_VERSION_PATCH(prop.runtimeVersion));
  }

  {
    XrSystemGetInfo sysInfo = {XR_TYPE_SYSTEM_GET_INFO};
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    OXR_CHECK(xrGetSystem(instance_, &sysInfo, &systemId_));

    /* query system properties*/
    XrSystemProperties prop = {XR_TYPE_SYSTEM_PROPERTIES};
#if defined(USE_OXR_HANDTRACK)
    XrSystemHandTrackingPropertiesEXT handTrackProp{
        XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
    prop.next = &handTrackProp;
#endif
    xrGetSystemProperties(instance_, systemId_, &prop);

    LOGI("-----------------------------------------------------------------");
    LOGI("System Properties         : Name=\"%s\", VendorId=%x",
         prop.systemName, prop.vendorId);
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
  }
}

bool OpenXRApp::confirmGLES(int major, int minor) {
  PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR;
  xrGetInstanceProcAddr(
      instance_, "xrGetOpenGLESGraphicsRequirementsKHR",
      (PFN_xrVoidFunction *)&xrGetOpenGLESGraphicsRequirementsKHR);

  XrGraphicsRequirementsOpenGLESKHR gfxReq = {
      XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
  xrGetOpenGLESGraphicsRequirementsKHR(instance_, systemId_, &gfxReq);

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
  return true;
}

static XrSpace oxr_create_ref_space(XrSession session,
                                    XrReferenceSpaceType ref_space_type) {
  XrSpace space;
  XrReferenceSpaceCreateInfo ci = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  ci.referenceSpaceType = ref_space_type;
  ci.poseInReferenceSpace.orientation.w = 1;
  xrCreateReferenceSpace(session, &ci, &space);
  return space;
}

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

void OpenXRApp::createSession() {

  XrGraphicsBindingOpenGLESAndroidKHR gfxBind = {
      XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};

  gfxBind.display = eglGetCurrentDisplay();
  if (gfxBind.display == EGL_NO_DISPLAY) {
    fprintf(stderr, "ERR: %s(%d)\n", __FILE__, __LINE__);
  }

  gfxBind.context = eglGetCurrentContext();
  if (gfxBind.context == EGL_NO_CONTEXT) {
    fprintf(stderr, "ERR: %s(%d)\n", __FILE__, __LINE__);
  }

  EGLint cfg_attribs[] = {EGL_CONFIG_ID, 0, EGL_NONE};
  eglQueryContext(gfxBind.display, gfxBind.context, EGL_CONFIG_ID,
                  &cfg_attribs[1]);
  int ival;
  if (eglChooseConfig(gfxBind.display, cfg_attribs, &gfxBind.config, 1,
                      &ival) != EGL_TRUE) {
    fprintf(stderr, "ERR: %s(%d)\n", __FILE__, __LINE__);
  }

  XrSessionCreateInfo ci = {XR_TYPE_SESSION_CREATE_INFO};
  ci.next = &gfxBind;
  ci.systemId = systemId_;

  xrCreateSession(instance_, &ci, &session_);

  appSpace_ = oxr_create_ref_space(session_, XR_REFERENCE_SPACE_TYPE_LOCAL);
  stageSpace_ = oxr_create_ref_space(session_, XR_REFERENCE_SPACE_TYPE_STAGE);

  // swapchain
  uint32_t viewCount;
  XrViewConfigurationView *conf_views =
      oxr_enumerate_viewconfig(instance_, systemId_, &viewCount);

  for (uint32_t i = 0; i < viewCount; i++) {
    const XrViewConfigurationView &vp = conf_views[i];
    uint32_t vp_w = vp.recommendedImageRectWidth;
    uint32_t vp_h = vp.recommendedImageRectHeight;

    LOGI("Swapchain for view %d: WH(%d, %d), SampleCount=%d", i, vp_w, vp_h,
         vp.recommendedSwapchainSampleCount);

    ViewSurface sfc;
    sfc.width = vp_w;
    sfc.height = vp_h;
    sfc.swapchain = oxr_create_swapchain(session_, sfc.width, sfc.height);
    // oxr_alloc_swapchain_rtargets(sfc.swapchain, sfc.width, sfc.height,
    //                              sfc.rtarget_array);

    viewSwapchains_.push_back(sfc);
  }
}

int64_t OpenXRApp::beginFrame()
{
  return 0;
}

void OpenXRApp::endFrame(int64_t time)
{ 
}
