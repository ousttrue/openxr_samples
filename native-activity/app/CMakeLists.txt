#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
cmake_minimum_required(VERSION 3.6.0)
project(native-activity VERSION 0.1.0)

#
# native_app_glue as a static lib
#
set(TARGET_NAME native_app_glue)
set(ANDROID_NATIVE_APP_GLUE
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
add_library(${TARGET_NAME} STATIC
            ${ANDROID_NATIVE_APP_GLUE}/android_native_app_glue.c)
target_include_directories(${TARGET_NAME} PUBLIC ${ANDROID_NATIVE_APP_GLUE})

# Export ANativeActivity_onCreate(), Refer to:
# https://github.com/android-ndk/ndk/issues/381.
target_link_options(${TARGET_NAME} PUBLIC -u ANativeActivity_onCreate)

subdirs(src/main/cpp)
