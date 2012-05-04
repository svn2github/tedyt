# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'targets': [
    {
      'target_name': 'test_bitrate_controller',
      'type': 'executable',
      'dependencies': [
        'bitrate_controller',
        '<(webrtc_root)/../test/test.gyp:test_support_main',
        '<(webrtc_root)/../testing/gtest.gyp:gtest',
      ],

      'include_dirs': [
        '../include',
        '../',
        '../../../system_wrappers/interface',
      ],

      'sources': [
        'test_bitrate_controller.cc',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
