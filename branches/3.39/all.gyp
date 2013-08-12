# Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': ['talk/build/common.gypi'],
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        'webrtc/webrtc.gyp:*',
      ],
      # TODO(henrike): fix build errors on 64 bit Mac for libjingle. See issue
      # 2124.  Once done the entire conditional below can be removed.
      'conditions': [
        ['OS!="mac" or target_arch!="x64" or libjingle_objc==1', {
          'dependencies': [
            'talk/libjingle.gyp:*',
            'talk/libjingle_examples.gyp:*',
            'talk/libjingle_tests.gyp:*',
          ],
        }],
      ],
    },
  ],
}
