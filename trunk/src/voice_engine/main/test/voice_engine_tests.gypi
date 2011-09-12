# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'variables': {
    'autotest_name': 'voe_auto_test',
  },
  'targets': [
    {
      'target_name': 'merged_lib_voice',
      'type': 'none',
      'dependencies': [
        '<(autotest_name)',
      ],
      'actions': [
        {
          'variables': {
            'output_lib_name': 'webrtc_voice_engine',
            'output_lib': '<(PRODUCT_DIR)/<(STATIC_LIB_PREFIX)<(output_lib_name)_<(OS)<(STATIC_LIB_SUFFIX)',
          },
          'action_name': 'merge_libs',
          'inputs': ['<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)<(autotest_name)<(EXECUTABLE_SUFFIX)'],
          'outputs': ['<(output_lib)'],
          'action': ['python',
                     '../build/merge_libs.py',
                     '<(PRODUCT_DIR)',
                     '<(output_lib)'],
        },
      ],
    },
    # Auto test - command line test for all platforms
    {
      'target_name': '<(autotest_name)',
      'type': 'executable',
      'dependencies': [
        'voice_engine_core',
        '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
      ],
      'include_dirs': [
        'auto_test',
        '<(webrtc_root)/modules/interface',
        '<(webrtc_root)/modules/audio_device/main/interface',
      ],
      'sources': [
        'auto_test/voe_cpu_test.cc',
        'auto_test/voe_cpu_test.h',
        'auto_test/voe_extended_test.cc',
        'auto_test/voe_extended_test.h',
        'auto_test/voe_standard_test.cc',
        'auto_test/voe_standard_test.h',
        'auto_test/voe_stress_test.cc',
        'auto_test/voe_stress_test.h',
        'auto_test/voe_test_defines.h',
        'auto_test/voe_test_interface.h',
        'auto_test/voe_unit_test.cc',
        'auto_test/voe_unit_test.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'dependencies': [
            'voice_engine.gyp:voe_ui_win_test',
          ],
        }],
      ],
    },
    {
      # command line test that should work on linux/mac/win
      'target_name': 'voe_cmd_test',
      'type': 'executable',
      'dependencies': [
        '<(webrtc_root)/../testing/gtest.gyp:gtest',
        'voice_engine_core',
        '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
      ],
      'sources': [
        'cmd_test/voe_cmd_test.cc',
      ],
    },
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        # WinTest - GUI test for Windows
        {
          'target_name': 'voe_ui_win_test',
          'type': 'executable',
          'dependencies': [
            'voice_engine_core',
            '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
          ],
          'include_dirs': [
            'win_test',
          ],
          'sources': [
            'win_test/Resource.h',
            'win_test/WinTest.cpp',
            'win_test/WinTest.h',
            'win_test/WinTest.rc',
            'win_test/WinTestDlg.cpp',
            'win_test/WinTestDlg.h',
            'win_test/res/WinTest.ico',
            'win_test/res/WinTest.rc2',
            'win_test/stdafx.cpp',
            'win_test/stdafx.h',
          ],
          'configurations': {
            'Common_Base': {
              'msvs_configuration_attributes': {
                'conditions': [
                  ['component=="shared_library"', {
                    'UseOfMFC': '2',  # Shared DLL
                  },{
                    'UseOfMFC': '1',  # Static
                  }],
                ],
              },
            },
          },
          'msvs_settings': {
            'VCLinkerTool': {
              'SubSystem': '2',   # Windows
            },
          },
        },
      ],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
