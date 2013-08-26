# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'variables': {
    'use_libjpeg_turbo%': '<(use_libjpeg_turbo)',
    'conditions': [
      ['use_libjpeg_turbo==1', {
        'libjpeg_include_dir%': [ '<(DEPTH)/third_party/libjpeg_turbo', ],
      }, {
        'libjpeg_include_dir%': [ '<(DEPTH)/third_party/libjpeg', ],
       }],
    ],
  },
  'includes': ['../build/common.gypi'],
  'targets': [
    {
      'target_name': 'common_video',
      'type': 'static_library',
      'include_dirs': [
        '<(webrtc_root)/modules/interface/',
        'interface',
        'jpeg/include',
        'libyuv/include',
      ],
      'dependencies': [
        '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'interface',
          'jpeg/include',
          'libyuv/include',
        ],
      },
      'conditions': [
        ['build_libjpeg==1', {
          'dependencies': ['<(libjpeg_gyp_path):libjpeg',],
        }, {
          # Need to add a directory normally exported by libjpeg.gyp.
          'include_dirs': ['<(libjpeg_include_dir)'],
        }],
        ['build_libyuv==1', {
          'dependencies': ['<(DEPTH)/third_party/libyuv/libyuv.gyp:libyuv',],
        }, {
          # Need to add a directory normally exported by libyuv.gyp.
          'include_dirs': ['<(libyuv_dir)/include',],
        }],
      ],
      'sources': [
        'interface/i420_video_frame.h',
        'interface/native_handle.h',
        'interface/texture_video_frame.h',
        'i420_video_frame.cc',
        'jpeg/include/jpeg.h',
        'jpeg/data_manager.cc',
        'jpeg/data_manager.h',
        'jpeg/jpeg.cc',
        'libyuv/include/webrtc_libyuv.h',
        'libyuv/include/scaler.h',
        'libyuv/webrtc_libyuv.cc',
        'libyuv/scaler.cc',
        'plane.h',
        'plane.cc',
        'texture_video_frame.cc'
      ],
      # Silence jpeg struct padding warnings.
      'msvs_disabled_warnings': [ 4324, ],
    },
  ],  # targets
  'conditions': [
    ['include_tests==1', {
      'targets': [
        {
          'target_name': 'common_video_unittests',
          'type': '<(gtest_target_type)',
          'dependencies': [
             'common_video',
             '<(DEPTH)/testing/gtest.gyp:gtest',
             '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
             '<(webrtc_root)/test/test.gyp:test_support_main',
          ],
          'sources': [
            'i420_video_frame_unittest.cc',
            'jpeg/jpeg_unittest.cc',
            'libyuv/libyuv_unittest.cc',
            'libyuv/scaler_unittest.cc',
            'plane_unittest.cc',
            'texture_video_frame_unittest.cc'
          ],
          # Disable warnings to enable Win64 build, issue 1323.
          'msvs_disabled_warnings': [
            4267,  # size_t to int truncation.
          ],
          'conditions': [
            # TODO(henrike): remove build_with_chromium==1 when the bots are
            # using Chromium's buildbots.
            ['build_with_chromium==1 and OS=="android" and gtest_target_type=="shared_library"', {
              'dependencies': [
                '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
              ],
            }],
          ],
        },
      ],  # targets
      'conditions': [
        # TODO(henrike): remove build_with_chromium==1 when the bots are using
        # Chromium's buildbots.
        ['build_with_chromium==1 and OS=="android" and gtest_target_type=="shared_library"', {
          'targets': [
            {
              'target_name': 'common_video_unittests_apk_target',
              'type': 'none',
              'dependencies': [
                '<(apk_tests_path):common_video_unittests_apk',
              ],
            },
          ],
        }],
        ['test_isolation_mode != "noop"', {
          'targets': [
            {
              'target_name': 'common_video_unittests_run',
              'type': 'none',
              'dependencies': [
                '<(import_isolate_path):import_isolate_gypi',
                'common_video_unittests',
              ],
              'includes': [
                'common_video_unittests.isolate',
              ],
              'sources': [
                'common_video_unittests.isolate',
              ],
            },
          ],
        }],
      ],
    }],  # include_tests
  ],
}
