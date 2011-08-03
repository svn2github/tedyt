#  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license
#  that can be found in the LICENSE file in the root of the source
#  tree. An additional intellectual property rights grant can be found
#  in the file PATENTS.  All contributing project authors may
#  be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH:= $(call my-dir)

# apm test app

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= \
    $(call all-proto-files-under, .) \
    unit_test.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../interface \
    $(LOCAL_PATH)/../../../../interface \
    $(LOCAL_PATH)/../../../../.. \
    $(LOCAL_PATH)/../../../../../system_wrappers/interface \
    $(LOCAL_PATH)/../../../../../common_audio/signal_processing_library/main/interface \
    external/gtest/include \
    external/protobuf/src

LOCAL_STATIC_LIBRARIES := \
    libgtest \
    libprotobuf-cpp-2.3.0-lite

LOCAL_SHARED_LIBRARIES := \
    libstlport \
    libwebrtc_audio_preprocessing

LOCAL_MODULE:= webrtc_apm_unit_test

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_EXECUTABLE)
