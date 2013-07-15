/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RECEIVER_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RECEIVER_H_

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class RTPPayloadRegistry;

class TelephoneEventHandler {
 public:
  virtual ~TelephoneEventHandler() {}

  // The following three methods implement the TelephoneEventHandler interface.
  // Forward DTMFs to decoder for playout.
  virtual void SetTelephoneEventForwardToDecoder(bool forward_to_decoder) = 0;

  // Is forwarding of outband telephone events turned on/off?
  virtual bool TelephoneEventForwardToDecoder() const = 0;

  // Is TelephoneEvent configured with payload type payload_type
  virtual bool TelephoneEventPayloadType(const int8_t payload_type) const = 0;
};

class RtpReceiver {
 public:
  // Creates a video-enabled RTP receiver.
  static RtpReceiver* CreateVideoReceiver(
      int id, Clock* clock,
      RtpData* incoming_payload_callback,
      RtpFeedback* incoming_messages_callback,
      RTPPayloadRegistry* rtp_payload_registry);

  // Creates an audio-enabled RTP receiver.
  static RtpReceiver* CreateAudioReceiver(
      int id, Clock* clock,
      RtpAudioFeedback* incoming_audio_feedback,
      RtpData* incoming_payload_callback,
      RtpFeedback* incoming_messages_callback,
      RTPPayloadRegistry* rtp_payload_registry);

  virtual ~RtpReceiver() {}

  // Returns a TelephoneEventHandler if available.
  virtual TelephoneEventHandler* GetTelephoneEventHandler() = 0;

  // Registers a receive payload in the payload registry and notifies the media
  // receiver strategy.
  virtual int32_t RegisterReceivePayload(
      const char payload_name[RTP_PAYLOAD_NAME_SIZE],
      const int8_t payload_type,
      const uint32_t frequency,
      const uint8_t channels,
      const uint32_t rate) = 0;

  // De-registers |payload_type| from the payload registry.
  virtual int32_t DeRegisterReceivePayload(const int8_t payload_type) = 0;

  // Parses the media specific parts of an RTP packet and updates the receiver
  // state. This for instance means that any changes in SSRC and payload type is
  // detected and acted upon.
  virtual bool IncomingRtpPacket(RTPHeader* rtp_header,
                                 const uint8_t* incoming_rtp_packet,
                                 int incoming_rtp_packet_length,
                                 PayloadUnion payload_specific,
                                 bool in_order) = 0;

  // Returns the currently configured NACK method.
  virtual NACKMethod NACK() const = 0;

  // Turn negative acknowledgement (NACK) requests on/off.
  virtual int32_t SetNACKStatus(const NACKMethod method,
                                int max_reordering_threshold) = 0;

  // Returns the last received timestamp.
  virtual uint32_t TimeStamp() const = 0;
  // Returns the time in milliseconds when the last timestamp was received.
  virtual int32_t LastReceivedTimeMs() const = 0;

  // Returns the remote SSRC of the currently received RTP stream.
  virtual uint32_t SSRC() const = 0;

  // Returns the current remote CSRCs.
  virtual int32_t CSRCs(uint32_t array_of_csrc[kRtpCsrcSize]) const = 0;

  // Returns the current energy of the RTP stream received.
  virtual int32_t Energy(uint8_t array_of_energy[kRtpCsrcSize]) const = 0;

  // Enable/disable RTX and set the SSRC to be used.
  virtual void SetRTXStatus(bool enable, uint32_t ssrc) = 0;

  // Returns the current RTX status and the SSRC and payload type used.
  virtual void RTXStatus(bool* enable, uint32_t* ssrc,
                         int* payload_type) const = 0;

  // Sets the RTX payload type.
  virtual void SetRtxPayloadType(int payload_type) = 0;

  // Returns true if the packet with RTP header |header| is likely to be a
  // retransmitted packet, false otherwise.
  virtual bool RetransmitOfOldPacket(const RTPHeader& header, int jitter,
                                     int min_rtt) const = 0;

  // Returns true if |sequence_number| is received in order, false otherwise.
  virtual bool InOrderPacket(const uint16_t sequence_number) const = 0;
};
}  // namespace webrtc

#endif  // WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RECEIVER_H_
