/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "webrtc/test/direct_transport.h"

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/call.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {
namespace test {

DirectTransport::DirectTransport()
    : lock_(CriticalSectionWrapper::CreateCriticalSection()),
      packet_event_(EventWrapper::Create()),
      thread_(ThreadWrapper::CreateThread(NetworkProcess, this)),
      clock_(Clock::GetRealTimeClock()),
      shutting_down_(false),
      receiver_(NULL),
      delay_ms_(0) {
  unsigned int thread_id;
  EXPECT_TRUE(thread_->Start(thread_id));
}

DirectTransport::DirectTransport(int delay_ms)
    : lock_(CriticalSectionWrapper::CreateCriticalSection()),
      packet_event_(EventWrapper::Create()),
      thread_(ThreadWrapper::CreateThread(NetworkProcess, this)),
      clock_(Clock::GetRealTimeClock()),
      shutting_down_(false),
      receiver_(NULL),
      delay_ms_(delay_ms) {
  unsigned int thread_id;
  EXPECT_TRUE(thread_->Start(thread_id));
}

DirectTransport::~DirectTransport() { StopSending(); }

void DirectTransport::StopSending() {
  {
    CriticalSectionScoped crit_(lock_.get());
    shutting_down_ = true;
  }

  packet_event_->Set();
  EXPECT_TRUE(thread_->Stop());
}

void DirectTransport::SetReceiver(PacketReceiver* receiver) {
  receiver_ = receiver;
}

bool DirectTransport::SendRTP(const uint8_t* data, size_t length) {
  QueuePacket(data, length, clock_->TimeInMilliseconds() + delay_ms_);
  return true;
}

bool DirectTransport::SendRTCP(const uint8_t* data, size_t length) {
  QueuePacket(data, length, clock_->TimeInMilliseconds() + delay_ms_);
  return true;
}

DirectTransport::Packet::Packet() : length(0), delivery_time_ms(0) {}

DirectTransport::Packet::Packet(const uint8_t* data,
                                size_t length,
                                int64_t delivery_time_ms)
    : length(length), delivery_time_ms(delivery_time_ms) {
  EXPECT_LE(length, sizeof(this->data));
  memcpy(this->data, data, length);
}

void DirectTransport::QueuePacket(const uint8_t* data,
                                  size_t length,
                                  int64_t delivery_time_ms) {
  CriticalSectionScoped crit(lock_.get());
  if (receiver_ == NULL)
    return;
  packet_queue_.push_back(Packet(data, length, delivery_time_ms));
  packet_event_->Set();
}

bool DirectTransport::NetworkProcess(void* transport) {
  return static_cast<DirectTransport*>(transport)->SendPackets();
}

bool DirectTransport::SendPackets() {
  while (true) {
    Packet p;
    {
      CriticalSectionScoped crit(lock_.get());
      if (packet_queue_.empty())
        break;
      p = packet_queue_.front();
      if (p.delivery_time_ms > clock_->TimeInMilliseconds())
        break;
      packet_queue_.pop_front();
    }
    receiver_->DeliverPacket(p.data, p.length);
  }
  uint32_t time_until_next_delivery = WEBRTC_EVENT_INFINITE;
  {
    CriticalSectionScoped crit(lock_.get());
    if (!packet_queue_.empty()) {
      int64_t now_ms = clock_->TimeInMilliseconds();
      const int64_t delivery_time_ms = packet_queue_.front().delivery_time_ms;
      if (delivery_time_ms > now_ms) {
        time_until_next_delivery = delivery_time_ms - now_ms;
      } else {
        time_until_next_delivery = 0;
      }
    }
  }

  switch (packet_event_->Wait(time_until_next_delivery)) {
    case kEventSignaled:
      packet_event_->Reset();
      break;
    case kEventTimeout:
      break;
    case kEventError:
      // TODO(pbos): Log a warning here?
      return true;
  }

  CriticalSectionScoped crit(lock_.get());
  return shutting_down_ ? false : true;
}
}  // namespace test
}  // namespace webrtc
