#include "OpenXRApp.h"
#include "android_logger.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
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
  oxr_check_errors(instance, func, #func, __FILE__, __LINE__);

struct OpenXRAppImpl {
  XrInstance instance = {0};
  XrSystemId systemId = {0};
  XrSession session = {0};

  OpenXRAppImpl(android_app *app) {
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

      OXR_CHECK(xrCreateInstance(&ci, &instance));

      // query instance name, version
      XrInstanceProperties prop = {XR_TYPE_INSTANCE_PROPERTIES};
      xrGetInstanceProperties(instance, &prop);
      LOGI("OpenXR Instance Runtime   : \"%s\", Version: %u.%u.%u",
           prop.runtimeName, XR_VERSION_MAJOR(prop.runtimeVersion),
           XR_VERSION_MINOR(prop.runtimeVersion),
           XR_VERSION_PATCH(prop.runtimeVersion));
    }

    {
      XrSystemGetInfo sysInfo = {XR_TYPE_SYSTEM_GET_INFO};
      sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

      OXR_CHECK(xrGetSystem(instance, &sysInfo, &systemId));

      /* query system properties*/
      XrSystemProperties prop = {XR_TYPE_SYSTEM_PROPERTIES};
#if defined(USE_OXR_HANDTRACK)
      XrSystemHandTrackingPropertiesEXT handTrackProp{
          XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
      prop.next = &handTrackProp;
#endif
      xrGetSystemProperties(instance, systemId, &prop);

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
      LOGI("System HandTracking Props : %d",
           handTrackProp.supportsHandTracking);
#endif
      LOGI("-----------------------------------------------------------------");
    }

    // TODO: OpenGLES

    {
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
      eglQueryContext(gfxBind.display, gfxBind.context, EGL_CONFIG_ID, &cfg_attribs[1]);
      int ival;
      if (eglChooseConfig(gfxBind.display, cfg_attribs, &gfxBind.config, 1, &ival) != EGL_TRUE) {
        fprintf(stderr, "ERR: %s(%d)\n", __FILE__, __LINE__);
      }

      XrSessionCreateInfo ci = {XR_TYPE_SESSION_CREATE_INFO};
      ci.next = &gfxBind;
      ci.systemId = systemId;

      xrCreateSession(instance, &ci, &session);
    }
  }
};

OpenXRApp::OpenXRApp(android_app *app) : impl_(new OpenXRAppImpl(app)) {}

OpenXRApp::~OpenXRApp() {}
