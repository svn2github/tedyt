/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/test/fake_encoder.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace webrtc {
namespace test {

FakeEncoder::FakeEncoder(Clock* clock)
    : clock_(clock),
      callback_(NULL),
      target_bitrate_kbps_(0),
      max_target_bitrate_kbps_(-1),
      last_encode_time_ms_(0) {
  // Generate some arbitrary not-all-zero data
  for (size_t i = 0; i < sizeof(encoded_buffer_); ++i) {
    encoded_buffer_[i] = static_cast<uint8_t>(i);
  }
}

FakeEncoder::~FakeEncoder() {}

void FakeEncoder::SetMaxBitrate(int max_kbps) {
  assert(max_kbps >= -1);  // max_kbps == -1 disables it.
  max_target_bitrate_kbps_ = max_kbps;
}

int32_t FakeEncoder::InitEncode(const VideoCodec* config,
                                int32_t number_of_cores,
                                uint32_t max_payload_size) {
  config_ = *config;
  target_bitrate_kbps_ = config_.startBitrate;
  return 0;
}

int32_t FakeEncoder::Encode(
    const I420VideoFrame& input_image,
    const CodecSpecificInfo* codec_specific_info,
    const std::vector<VideoFrameType>* frame_types) {
  assert(config_.maxFramerate > 0);
  int time_since_last_encode_ms = 1000 / config_.maxFramerate;
  int64_t time_now_ms = clock_->TimeInMilliseconds();
  if (last_encode_time_ms_ > 0) {
    // For all frames but the first we can estimate the display time by looking
    // at the display time of the previous frame.
    time_since_last_encode_ms = time_now_ms - last_encode_time_ms_;
  }

  int bits_available = target_bitrate_kbps_ * time_since_last_encode_ms;
  int min_bits =
      config_.simulcastStream[0].minBitrate * time_since_last_encode_ms;
  if (bits_available < min_bits)
    bits_available = min_bits;
  int max_bits = max_target_bitrate_kbps_ * time_since_last_encode_ms;
  if (max_bits > 0 && max_bits < bits_available)
    bits_available = max_bits;
  last_encode_time_ms_ = time_now_ms;

  assert(config_.numberOfSimulcastStreams > 0);
  for (int i = 0; i < config_.numberOfSimulcastStreams; ++i) {
    CodecSpecificInfo specifics;
    memset(&specifics, 0, sizeof(specifics));
    specifics.codecType = kVideoCodecGeneric;
    specifics.codecSpecific.generic.simulcast_idx = i;
    int min_stream_bits =
        config_.simulcastStream[i].minBitrate * time_since_last_encode_ms;
    int max_stream_bits =
        config_.simulcastStream[i].maxBitrate * time_since_last_encode_ms;
    int stream_bits = (bits_available > max_stream_bits) ? max_stream_bits :
        bits_available;
    int stream_bytes = (stream_bits + 7) / 8;
    if (static_cast<size_t>(stream_bytes) > sizeof(encoded_buffer_))
      stream_bytes = sizeof(encoded_buffer_);

    EncodedImage encoded(
        encoded_buffer_, stream_bytes, sizeof(encoded_buffer_));
    encoded._timeStamp = input_image.timestamp();
    encoded.capture_time_ms_ = input_image.render_time_ms();
    encoded._frameType = (*frame_types)[i];
    // Always encode something on the first frame.
    if (min_stream_bits > bits_available && i > 0) {
      encoded._length = 0;
      encoded._frameType = kSkipFrame;
    }
    assert(callback_ != NULL);
    if (callback_->Encoded(encoded, &specifics, NULL) != 0)
      return -1;

    bits_available -= encoded._length * 8;
  }
  return 0;
}

int32_t FakeEncoder::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  callback_ = callback;
  return 0;
}

int32_t FakeEncoder::Release() { return 0; }

int32_t FakeEncoder::SetChannelParameters(uint32_t packet_loss, int rtt) {
  return 0;
}

int32_t FakeEncoder::SetRates(uint32_t new_target_bitrate, uint32_t framerate) {
  target_bitrate_kbps_ = new_target_bitrate;
  return 0;
}

}  // namespace test
}  // namespace webrtc
