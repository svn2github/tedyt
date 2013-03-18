/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <algorithm>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"

namespace webrtc {
const int kVideoNackListSize = 10;
const int kTestId = 123;
const WebRtc_UWord32 kTestSsrc = 3456;
const WebRtc_UWord16 kTestSequenceNumber = 2345;
const WebRtc_UWord32 kTestNumberOfPackets = 450;
const int kTestNumberOfRtxPackets = 49;

class VerifyingRtxReceiver : public RtpData {
 public:
  VerifyingRtxReceiver() {}

  virtual WebRtc_Word32 OnReceivedPayloadData(
      const WebRtc_UWord8* data,
      const WebRtc_UWord16 size,
      const webrtc::WebRtcRTPHeader* rtp_header) {
    if (!sequence_numbers_.empty()) {
      EXPECT_EQ(kTestSsrc, rtp_header->header.ssrc);
    }
    sequence_numbers_.push_back(rtp_header->header.sequenceNumber);
    return 0;
  }
  std::vector<WebRtc_UWord16 > sequence_numbers_;
};

class RtxLoopBackTransport : public webrtc::Transport {
 public:
  explicit RtxLoopBackTransport(uint32_t rtx_ssrc)
      : count_(0),
        packet_loss_(0),
        rtx_ssrc_(rtx_ssrc),
        count_rtx_ssrc_(0),
        module_(NULL) {
  }
  void SetSendModule(RtpRtcp* rtpRtcpModule) {
    module_ = rtpRtcpModule;
  }
  void DropEveryNthPacket(int n) {
    packet_loss_ = n;
  }
  virtual int SendPacket(int channel, const void *data, int len) {
    count_++;
    const unsigned char* ptr = static_cast<const unsigned  char*>(data);
    uint32_t ssrc = (ptr[8] << 24) + (ptr[9] << 16) + (ptr[10] << 8) + ptr[11];
    if (ssrc == rtx_ssrc_) count_rtx_ssrc_++;
    if (packet_loss_ > 0) {
      if ((count_ % packet_loss_) == 0) {
        return len;
      }
    }
    if (module_->IncomingPacket((const WebRtc_UWord8*)data, len) == 0) {
      return len;
    }
    return -1;
  }
  virtual int SendRTCPPacket(int channel, const void *data, int len) {
    if (module_->IncomingPacket((const WebRtc_UWord8*)data, len) == 0) {
      return len;
    }
    return -1;
  }
  int count_;
  int packet_loss_;
  uint32_t rtx_ssrc_;
  int count_rtx_ssrc_;
  RtpRtcp* module_;
};

class RtpRtcpNackTest : public ::testing::Test {
 protected:
  RtpRtcpNackTest()
      : rtp_rtcp_module_(NULL),
        transport_(kTestSsrc + 1),
        receiver_(),
        payload_data_length(sizeof(payload_data)),
        fake_clock(123456) {}
  ~RtpRtcpNackTest() {}

  virtual void SetUp() {
    RtpRtcp::Configuration configuration;
    configuration.id = kTestId;
    configuration.audio = false;
    configuration.clock = &fake_clock;
    configuration.incoming_data = &receiver_;
    configuration.outgoing_transport = &transport_;
    rtp_rtcp_module_ = RtpRtcp::CreateRtpRtcp(configuration);

    EXPECT_EQ(0, rtp_rtcp_module_->SetSSRC(kTestSsrc));
    EXPECT_EQ(0, rtp_rtcp_module_->SetRTCPStatus(kRtcpCompound));
    EXPECT_EQ(0, rtp_rtcp_module_->SetNACKStatus(kNackRtcp, 450));
    EXPECT_EQ(0, rtp_rtcp_module_->SetStorePacketsStatus(true, 600));
    EXPECT_EQ(0, rtp_rtcp_module_->SetSendingStatus(true));
    EXPECT_EQ(0, rtp_rtcp_module_->SetSequenceNumber(kTestSequenceNumber));
    EXPECT_EQ(0, rtp_rtcp_module_->SetStartTimestamp(111111));

    transport_.SetSendModule(rtp_rtcp_module_);

    VideoCodec video_codec;
    memset(&video_codec, 0, sizeof(video_codec));
    video_codec.plType = 123;
    memcpy(video_codec.plName, "I420", 5);

    EXPECT_EQ(0, rtp_rtcp_module_->RegisterSendPayload(video_codec));
    EXPECT_EQ(0, rtp_rtcp_module_->RegisterReceivePayload(video_codec));

    for (int n = 0; n < payload_data_length; n++) {
      payload_data[n] = n % 10;
    }
  }

  virtual void TearDown() {
    delete rtp_rtcp_module_;
  }

  RtpRtcp* rtp_rtcp_module_;
  RtxLoopBackTransport transport_;
  VerifyingRtxReceiver receiver_;
  WebRtc_UWord8  payload_data[65000];
  int payload_data_length;
  SimulatedClock fake_clock;
};

TEST_F(RtpRtcpNackTest, RTCP) {
  WebRtc_UWord32 timestamp = 3000;
  WebRtc_UWord16 nack_list[kVideoNackListSize];
  transport_.DropEveryNthPacket(10);

  for (int frame = 0; frame < 10; ++frame) {
    EXPECT_EQ(0, rtp_rtcp_module_->SendOutgoingData(webrtc::kVideoFrameDelta,
                                                    123,
                                                    timestamp,
                                                    timestamp / 90,
                                                    payload_data,
                                                    payload_data_length));

    std::sort(receiver_.sequence_numbers_.begin(),
              receiver_.sequence_numbers_.end());

    std::vector<WebRtc_UWord16> missing_sequence_numbers;
    std::vector<WebRtc_UWord16>::iterator it =
        receiver_.sequence_numbers_.begin();

    while (it != receiver_.sequence_numbers_.end()) {
      WebRtc_UWord16 sequence_number_1 = *it;
      ++it;
      if (it != receiver_.sequence_numbers_.end()) {
        WebRtc_UWord16 sequence_number_2 = *it;
        // Add all missing sequence numbers to list.
        for (WebRtc_UWord16 i = sequence_number_1 + 1; i < sequence_number_2;
            ++i) {
          missing_sequence_numbers.push_back(i);
        }
      }
    }
    int n = 0;
    for (it = missing_sequence_numbers.begin();
        it != missing_sequence_numbers.end(); ++it) {
      nack_list[n++] = (*it);
    }
    rtp_rtcp_module_->SendNACK(nack_list, n);
    fake_clock.AdvanceTimeMilliseconds(33);
    rtp_rtcp_module_->Process();

    // Prepare next frame.
    timestamp += 3000;
  }
  std::sort(receiver_.sequence_numbers_.begin(),
            receiver_.sequence_numbers_.end());
  EXPECT_EQ(kTestSequenceNumber, *(receiver_.sequence_numbers_.begin()));
  EXPECT_EQ(kTestSequenceNumber + kTestNumberOfPackets - 1,
            *(receiver_.sequence_numbers_.rbegin()));
  EXPECT_EQ(kTestNumberOfPackets, receiver_.sequence_numbers_.size());
  EXPECT_EQ(0, transport_.count_rtx_ssrc_);
}

TEST_F(RtpRtcpNackTest, RTXNack) {
  EXPECT_EQ(0, rtp_rtcp_module_->SetRTXReceiveStatus(true, kTestSsrc + 1));
  EXPECT_EQ(0, rtp_rtcp_module_->SetRTXSendStatus(kRtxRetransmitted,
                                                  true, kTestSsrc + 1));

  transport_.DropEveryNthPacket(10);

  WebRtc_UWord32 timestamp = 3000;
  WebRtc_UWord16 nack_list[kVideoNackListSize];

  for (int frame = 0; frame < 10; ++frame) {
    EXPECT_EQ(0, rtp_rtcp_module_->SendOutgoingData(webrtc::kVideoFrameDelta,
                                                    123,
                                                    timestamp,
                                                    timestamp / 90,
                                                    payload_data,
                                                    payload_data_length));

    std::sort(receiver_.sequence_numbers_.begin(),
              receiver_.sequence_numbers_.end());

    std::vector<WebRtc_UWord16> missing_sequence_numbers;


    std::vector<WebRtc_UWord16>::iterator it =
        receiver_.sequence_numbers_.begin();
    while (it != receiver_.sequence_numbers_.end()) {
      int sequence_number_1 = *it;
      ++it;
      if (it != receiver_.sequence_numbers_.end()) {
        int sequence_number_2 = *it;
        // Add all missing sequence numbers to list.
        for (int i = sequence_number_1 + 1; i < sequence_number_2; ++i) {
          missing_sequence_numbers.push_back(i);
        }
      }
    }
    int n = 0;
    for (it = missing_sequence_numbers.begin();
        it != missing_sequence_numbers.end(); ++it) {
      nack_list[n++] = (*it);
    }
    rtp_rtcp_module_->SendNACK(nack_list, n);
    fake_clock.AdvanceTimeMilliseconds(33);
    rtp_rtcp_module_->Process();

    // Prepare next frame.
    timestamp += 3000;
  }
  std::sort(receiver_.sequence_numbers_.begin(),
            receiver_.sequence_numbers_.end());
  EXPECT_EQ(kTestSequenceNumber, *(receiver_.sequence_numbers_.begin()));
  EXPECT_EQ(kTestSequenceNumber + kTestNumberOfPackets - 1,
            *(receiver_.sequence_numbers_.rbegin()));
  EXPECT_EQ(kTestNumberOfPackets, receiver_.sequence_numbers_.size());
  EXPECT_EQ(kTestNumberOfRtxPackets, transport_.count_rtx_ssrc_);
}

TEST_F(RtpRtcpNackTest, RTXAllNoLoss) {
  EXPECT_EQ(0, rtp_rtcp_module_->SetRTXReceiveStatus(true, kTestSsrc + 1));
  EXPECT_EQ(0, rtp_rtcp_module_->SetRTXSendStatus(kRtxAll,
                                                  true, kTestSsrc + 1));
  transport_.DropEveryNthPacket(0);

  WebRtc_UWord32 timestamp = 3000;

  for (int frame = 0; frame < 10; ++frame) {
    EXPECT_EQ(0, rtp_rtcp_module_->SendOutgoingData(webrtc::kVideoFrameDelta,
                                                    123,
                                                    timestamp,
                                                    timestamp / 90,
                                                    payload_data,
                                                    payload_data_length));

    fake_clock.AdvanceTimeMilliseconds(33);
    rtp_rtcp_module_->Process();

    // Prepare next frame.
    timestamp += 3000;
  }
  std::sort(receiver_.sequence_numbers_.begin(),
            receiver_.sequence_numbers_.end());
  EXPECT_EQ(kTestSequenceNumber, *(receiver_.sequence_numbers_.begin()));
  EXPECT_EQ(kTestSequenceNumber + kTestNumberOfPackets - 1,
            *(receiver_.sequence_numbers_.rbegin()));
  // We have transmitted all packets twice, and loss was set to 0.
  EXPECT_EQ(kTestNumberOfPackets * 2u, receiver_.sequence_numbers_.size());
  // Half of the packets should be via RTX.
  EXPECT_EQ(static_cast<int>(kTestNumberOfPackets),
      transport_.count_rtx_ssrc_);
}

TEST_F(RtpRtcpNackTest, RTXAllWithLoss) {
  EXPECT_EQ(0, rtp_rtcp_module_->SetRTXReceiveStatus(true, kTestSsrc + 1));
  EXPECT_EQ(0, rtp_rtcp_module_->SetRTXSendStatus(kRtxAll,
                                                  true,
                                                  kTestSsrc + 1));

  int loss = 10;
  transport_.DropEveryNthPacket(loss);

  WebRtc_UWord32 timestamp = 3000;
  WebRtc_UWord16 nack_list[kVideoNackListSize];

  for (int frame = 0; frame < 10; ++frame) {
    EXPECT_EQ(0, rtp_rtcp_module_->SendOutgoingData(webrtc::kVideoFrameDelta,
                                                    123,
                                                    timestamp,
                                                    timestamp / 90,
                                                    payload_data,
                                                    payload_data_length));
    std::sort(receiver_.sequence_numbers_.begin(),
              receiver_.sequence_numbers_.end());
    std::vector<WebRtc_UWord16> missing_sequence_numbers;

    std::vector<WebRtc_UWord16>::iterator it =
        receiver_.sequence_numbers_.begin();
    while (it != receiver_.sequence_numbers_.end()) {
      int sequence_number_1 = *it;
      ++it;
      if (it != receiver_.sequence_numbers_.end()) {
        int sequence_number_2 = *it;
        // Add all missing sequence numbers to list.
        for (int i = sequence_number_1 + 1; i < sequence_number_2; ++i) {
          missing_sequence_numbers.push_back(i);
        }
      }
    }
    int n = 0;
    for (it = missing_sequence_numbers.begin();
        it != missing_sequence_numbers.end(); ++it) {
      nack_list[n++] = (*it);
    }
    if (n > 0)
      rtp_rtcp_module_->SendNACK(nack_list, n);
    fake_clock.AdvanceTimeMilliseconds(33);
    rtp_rtcp_module_->Process();

    // Prepare next frame.
    timestamp += 3000;
  }
  std::sort(receiver_.sequence_numbers_.begin(),
            receiver_.sequence_numbers_.end());
  EXPECT_EQ(kTestSequenceNumber, *(receiver_.sequence_numbers_.begin()));
  EXPECT_EQ(kTestSequenceNumber + kTestNumberOfPackets - 1,
            *(receiver_.sequence_numbers_.rbegin()));
  // Got everything but 10% loss.
  EXPECT_EQ(2u * (kTestNumberOfPackets - kTestNumberOfPackets / 10),
            receiver_.sequence_numbers_.size());
  EXPECT_EQ(static_cast<int>(kTestNumberOfPackets),
            transport_.count_rtx_ssrc_);
}

}  // namespace webrtc
