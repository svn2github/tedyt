/*
 * libjingle
 * Copyright 2012, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This file contains mock implementations of observers used in PeerConnection.

#ifndef TALK_APP_WEBRTC_TEST_MOCKPEERCONNECTIONOBSERVERS_H_
#define TALK_APP_WEBRTC_TEST_MOCKPEERCONNECTIONOBSERVERS_H_

#include <string>

#include "talk/app/webrtc/datachannelinterface.h"

namespace webrtc {

class MockCreateSessionDescriptionObserver
    : public webrtc::CreateSessionDescriptionObserver {
 public:
  MockCreateSessionDescriptionObserver()
      : called_(false),
        result_(false) {}
  virtual ~MockCreateSessionDescriptionObserver() {}
  virtual void OnSuccess(SessionDescriptionInterface* desc) {
    called_ = true;
    result_ = true;
    desc_.reset(desc);
  }
  virtual void OnFailure(const std::string& error) {
    called_ = true;
    result_ = false;
  }
  bool called() const { return called_; }
  bool result() const { return result_; }
  SessionDescriptionInterface* release_desc() {
    return desc_.release();
  }

 private:
  bool called_;
  bool result_;
  rtc::scoped_ptr<SessionDescriptionInterface> desc_;
};

class MockSetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  MockSetSessionDescriptionObserver()
      : called_(false),
        result_(false) {}
  virtual ~MockSetSessionDescriptionObserver() {}
  virtual void OnSuccess() {
    called_ = true;
    result_ = true;
  }
  virtual void OnFailure(const std::string& error) {
    called_ = true;
    result_ = false;
  }
  bool called() const { return called_; }
  bool result() const { return result_; }

 private:
  bool called_;
  bool result_;
};

class MockDataChannelObserver : public webrtc::DataChannelObserver {
 public:
  explicit MockDataChannelObserver(webrtc::DataChannelInterface* channel)
     : channel_(channel), received_message_count_(0) {
    channel_->RegisterObserver(this);
    state_ = channel_->state();
  }
  virtual ~MockDataChannelObserver() {
    channel_->UnregisterObserver();
  }

  virtual void OnStateChange() { state_ = channel_->state(); }
  virtual void OnMessage(const DataBuffer& buffer) {
    last_message_.assign(buffer.data.data(), buffer.data.length());
    ++received_message_count_;
  }

  bool IsOpen() const { return state_ == DataChannelInterface::kOpen; }
  const std::string& last_message() const { return last_message_; }
  size_t received_message_count() const { return received_message_count_; }

 private:
  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  DataChannelInterface::DataState state_;
  std::string last_message_;
  size_t received_message_count_;
};

class MockStatsObserver : public webrtc::StatsObserver {
 public:
  MockStatsObserver()
      : called_(false) {}
  virtual ~MockStatsObserver() {}
  virtual void OnComplete(const StatsReports& reports) {
    called_ = true;
    reports_.clear();
    reports_.reserve(reports.size());
    StatsReports::const_iterator it;
    for (it = reports.begin(); it != reports.end(); ++it)
      reports_.push_back(StatsReportCopyable(*(*it)));
  }

  bool called() const { return called_; }
  size_t number_of_reports() const { return reports_.size(); }

  int AudioOutputLevel() {
    return GetStatsValue(StatsReport::kStatsReportTypeSsrc,
                         StatsReport::kStatsValueNameAudioOutputLevel);
  }

  int AudioInputLevel() {
    return GetStatsValue(StatsReport::kStatsReportTypeSsrc,
                         StatsReport::kStatsValueNameAudioInputLevel);
  }

  int BytesReceived() {
    return GetStatsValue(StatsReport::kStatsReportTypeSsrc,
                         StatsReport::kStatsValueNameBytesReceived);
  }

  int BytesSent() {
    return GetStatsValue(StatsReport::kStatsReportTypeSsrc,
                         StatsReport::kStatsValueNameBytesSent);
  }

  int AvailableReceiveBandwidth() {
    return GetStatsValue(StatsReport::kStatsReportTypeBwe,
                         StatsReport::kStatsValueNameAvailableReceiveBandwidth);
  }

 private:
  int GetStatsValue(const std::string& type, StatsReport::StatsValueName name) {
    if (reports_.empty()) {
      return 0;
    }
    for (size_t i = 0; i < reports_.size(); ++i) {
      if (reports_[i].type != type)
        continue;
      webrtc::StatsReport::Values::const_iterator it =
          reports_[i].values.begin();
      for (; it != reports_[i].values.end(); ++it) {
        if (it->name == name) {
          return rtc::FromString<int>(it->value);
        }
      }
    }
    return 0;
  }

  bool called_;
  std::vector<StatsReportCopyable> reports_;
};

}  // namespace webrtc

#endif  // TALK_APP_WEBRTC_TEST_MOCKPEERCONNECTIONOBSERVERS_H_
