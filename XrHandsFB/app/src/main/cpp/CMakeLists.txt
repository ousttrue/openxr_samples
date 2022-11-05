set(TARGET_NAME openxr_loader)
add_library(${TARGET_NAME} SHARED IMPORTED)
set(OPENXR_LOADER_INCLUDES ${OPENXR_SDK_DIR}/include
                           ${MOBILE_SDK_DIR}/OpenXR/Include)
set_target_properties(
  ${TARGET_NAME}
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENXR_LOADER_INCLUDES}"
    IMPORTED_LOCATION
    ${MOBILE_SDK_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/Release/libopenxr_loader.so
)

set(TARGET_NAME minizip)
add_library(
  ${TARGET_NAME} STATIC
  ${MOBILE_SDK_DIR}/3rdParty/minizip/src/ioapi.c
  ${MOBILE_SDK_DIR}/3rdParty/minizip/src/miniunz.c
  ${MOBILE_SDK_DIR}/3rdParty/minizip/src/mztools.c
  ${MOBILE_SDK_DIR}/3rdParty/minizip/src/unzip.c
  ${MOBILE_SDK_DIR}/3rdParty/minizip/src/zip.c)
target_include_directories(${TARGET_NAME}
                           PUBLIC ${MOBILE_SDK_DIR}/3rdParty/minizip/src)
target_link_libraries(${TARGET_NAME} PRIVATE z)

set(TARGET_NAME stb)
add_library(
  ${TARGET_NAME} STATIC
  ${MOBILE_SDK_DIR}/3rdParty/stb/src/stb_image.c
  ${MOBILE_SDK_DIR}/3rdParty/stb/src/stb_image_write.c
  ${MOBILE_SDK_DIR}/3rdParty/stb/src/stb_vorbis.c)
target_include_directories(${TARGET_NAME}
                           PUBLIC ${MOBILE_SDK_DIR}/3rdParty/stb/src)

set(TARGET_NAME ktx)
add_library(${TARGET_NAME} SHARED IMPORTED)
set_target_properties(
  ${TARGET_NAME}
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${MOBILE_SDK_DIR}/3rdParty/khronos/ktx/include
    IMPORTED_LOCATION
    ${MOBILE_SDK_DIR}/3rdParty/khronos/ktx/lib/android/arm64-v8a/libktx_read.so
    ${MOBILE_SDK_DIR}/3rdParty/khronos/ktx/lib/android/arm64-v8a/libktx.so
    ${MOBILE_SDK_DIR}/3rdParty/khronos/ktx/lib/android/arm64-v8a/libobjUtil.a)

set(OVR_DIR ${MOBILE_SDK_DIR}/1stParty/OVR)
set(TARGET_NAME OVR)
add_library(${TARGET_NAME} INTERFACE)
target_include_directories(${TARGET_NAME} INTERFACE ${OVR_DIR}/Include)

set(SAMPLE_COMMON_DIR ${MOBILE_SDK_DIR}/SampleCommon/Src)
set(TARGET_NAME SampleCommon)
add_library(
  ${TARGET_NAME} STATIC
  ${SAMPLE_COMMON_DIR}/GUI/ActionComponents.cpp
  ${SAMPLE_COMMON_DIR}/GUI/AnimComponents.cpp
  ${SAMPLE_COMMON_DIR}/GUI/CollisionPrimitive.cpp
  ${SAMPLE_COMMON_DIR}/GUI/DefaultComponent.cpp
  ${SAMPLE_COMMON_DIR}/GUI/Fader.cpp
  ${SAMPLE_COMMON_DIR}/GUI/GazeCursor.cpp
  ${SAMPLE_COMMON_DIR}/GUI/GuiSys.cpp
  ${SAMPLE_COMMON_DIR}/GUI/MetaDataManager.cpp
  ${SAMPLE_COMMON_DIR}/GUI/Reflection.cpp
  ${SAMPLE_COMMON_DIR}/GUI/ReflectionData.cpp
  ${SAMPLE_COMMON_DIR}/GUI/SoundLimiter.cpp
  ${SAMPLE_COMMON_DIR}/GUI/VRMenu.cpp
  ${SAMPLE_COMMON_DIR}/GUI/VRMenuComponent.cpp
  ${SAMPLE_COMMON_DIR}/GUI/VRMenuEvent.cpp
  ${SAMPLE_COMMON_DIR}/GUI/VRMenuEventHandler.cpp
  ${SAMPLE_COMMON_DIR}/GUI/VRMenuMgr.cpp
  ${SAMPLE_COMMON_DIR}/GUI/VRMenuObject.cpp
  ${SAMPLE_COMMON_DIR}/Input/ArmModel.cpp
  ${SAMPLE_COMMON_DIR}/Input/AxisRenderer.cpp
  ${SAMPLE_COMMON_DIR}/Input/ControllerRenderer.cpp
  ${SAMPLE_COMMON_DIR}/Input/Skeleton.cpp
  ${SAMPLE_COMMON_DIR}/Input/SkeletonRenderer.cpp
  ${SAMPLE_COMMON_DIR}/Input/TinyUI.cpp
  ${SAMPLE_COMMON_DIR}/Locale/OVR_Locale.cpp
  ${SAMPLE_COMMON_DIR}/Locale/tinyxml2.cpp
  ${SAMPLE_COMMON_DIR}/Misc/Log.c
  ${SAMPLE_COMMON_DIR}/Model/ModelCollision.cpp
  ${SAMPLE_COMMON_DIR}/Model/ModelFile_glTF.cpp
  ${SAMPLE_COMMON_DIR}/Model/ModelFile_OvrScene.cpp
  ${SAMPLE_COMMON_DIR}/Model/ModelFile.cpp
  ${SAMPLE_COMMON_DIR}/Model/ModelRender.cpp
  ${SAMPLE_COMMON_DIR}/Model/ModelTrace.cpp
  ${SAMPLE_COMMON_DIR}/Model/SceneView.cpp
  ${SAMPLE_COMMON_DIR}/OVR_BinaryFile2.cpp
  ${SAMPLE_COMMON_DIR}/OVR_FileSys.cpp
  ${SAMPLE_COMMON_DIR}/OVR_Lexer2.cpp
  ${SAMPLE_COMMON_DIR}/OVR_MappedFile.cpp
  ${SAMPLE_COMMON_DIR}/OVR_Stream.cpp
  ${SAMPLE_COMMON_DIR}/OVR_Uri.cpp
  ${SAMPLE_COMMON_DIR}/OVR_UTF8Util.cpp
  ${SAMPLE_COMMON_DIR}/PackageFiles.cpp
  ${SAMPLE_COMMON_DIR}/Render/BeamRenderer.cpp
  ${SAMPLE_COMMON_DIR}/Render/BitmapFont.cpp
  ${SAMPLE_COMMON_DIR}/Render/DebugLines.cpp
  ${SAMPLE_COMMON_DIR}/Render/EaseFunctions.cpp
  ${SAMPLE_COMMON_DIR}/Render/Egl.c
  ${SAMPLE_COMMON_DIR}/Render/GeometryBuilder.cpp
  ${SAMPLE_COMMON_DIR}/Render/GeometryRenderer.cpp
  ${SAMPLE_COMMON_DIR}/Render/GlBuffer.cpp
  ${SAMPLE_COMMON_DIR}/Render/GlGeometry.cpp
  ${SAMPLE_COMMON_DIR}/Render/GlProgram.cpp
  ${SAMPLE_COMMON_DIR}/Render/GlTexture.cpp
  ${SAMPLE_COMMON_DIR}/Render/PanelRenderer.cpp
  ${SAMPLE_COMMON_DIR}/Render/ParticleSystem.cpp
  ${SAMPLE_COMMON_DIR}/Render/PointList.cpp
  ${SAMPLE_COMMON_DIR}/Render/Ribbon.cpp
  ${SAMPLE_COMMON_DIR}/Render/SurfaceRender.cpp
  ${SAMPLE_COMMON_DIR}/Render/SurfaceTexture.cpp
  ${SAMPLE_COMMON_DIR}/Render/TextureAtlas.cpp
  ${SAMPLE_COMMON_DIR}/Render/TextureManager.cpp
  ${SAMPLE_COMMON_DIR}/System.cpp)
target_include_directories(
  ${TARGET_NAME} PUBLIC ${SAMPLE_COMMON_DIR}
                        ${MOBILE_SDK_DIR}/1stParty/utilities/include)
target_link_libraries(${TARGET_NAME} PUBLIC OVR minizip stb ktx)

set(SAMPLE_FRAMEWORK_DIR ${MOBILE_SDK_DIR}/SampleXrFramework/Src)
set(TARGET_NAME SampleXrFramewok)
add_library(
  ${TARGET_NAME} STATIC
  ${SAMPLE_FRAMEWORK_DIR}/XrApp.cpp
  ${SAMPLE_FRAMEWORK_DIR}/Input/HandMaskRenderer.cpp
  ${SAMPLE_FRAMEWORK_DIR}/Input/HandRenderer.cpp
  ${SAMPLE_FRAMEWORK_DIR}/Render/Framebuffer.cpp)
target_include_directories(${TARGET_NAME} PUBLIC ${SAMPLE_FRAMEWORK_DIR})
target_link_libraries(${TARGET_NAME} PUBLIC native_app_glue OVR SampleCommon
                                            openxr_loader)

set(TARGET_NAME xrhandsfb)
add_library(${TARGET_NAME} SHARED
            ${MOBILE_SDK_DIR}/XrSamples/XrHandsFB/Src/main.cpp)
target_link_libraries(
  ${TARGET_NAME}
  PRIVATE OVR
          SampleCommon
          SampleXrFramewok
          openxr_loader
          android
          log
          GLESv3
          EGL)
