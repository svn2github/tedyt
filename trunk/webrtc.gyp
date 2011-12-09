# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': [ 'src/build/common.gypi', ],
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        # TODO(andrew): re-enable when libjingle is rolled.
        #'peerconnection/peerconnection.gyp:*',
        'src/common_audio/common_audio.gyp:*',
        'src/common_video/common_video.gyp:*',
        'src/modules/modules.gyp:*',
        'src/system_wrappers/source/system_wrappers.gyp:*',
        'src/video_engine/video_engine.gyp:*',
        'src/voice_engine/voice_engine.gyp:*',
        'test/test.gyp:*',
      ],
    },
  ],
  'conditions': [
    ['build_with_chromium==1', {
      # Exclude components in engine_configuration.h.
      'defines': [
        'WEBRTC_CHROMIUM_BUILD',
      ],
    }, ],  # build_with_chromium=1
  ],  # conditions
}
