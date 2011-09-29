/*
 * libjingle
 * Copyright 2011, Google Inc.
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

#include <string>

#include "gtest/gtest.h"
#include "talk/app/webrtc_dev/peerconnectionmessage.h"
#include "talk/base/logging.h"
#include "talk/base/scoped_ptr.h"
#include "talk/session/phone/channelmanager.h"

using webrtc::PeerConnectionMessage;

static const char kStreamLabel1[] = "local_stream_1";
static const char kAudioTrackLabel1[] = "local_audio_1";
static const char kVideoTrackLabel1[] = "local_video_1";
static const char kVideoTrackLabel2[] = "local_video_2";

static const char kStreamLabel2[] = "local_stream_2";
static const char kAudioTrackLabel2[] = "local_audio_2";
static const char kVideoTrackLabel3[] = "local_video_3";

class PeerConnectionMessageTest: public testing::Test {
 public:
  PeerConnectionMessageTest()
      : ssrc_counter_(0) {
    channel_manager_.reset(new cricket::ChannelManager(
        talk_base::Thread::Current()));
    EXPECT_TRUE(channel_manager_->Init());
    session_description_factory_.reset(
        new cricket::MediaSessionDescriptionFactory(channel_manager_.get()));
    options_.audio_sources.push_back(cricket::SourceParam(++ssrc_counter_,
        kAudioTrackLabel1, kStreamLabel1));
    options_.video_sources.push_back(cricket::SourceParam(++ssrc_counter_,
        kVideoTrackLabel1, kStreamLabel1));
    options_.video_sources.push_back(cricket::SourceParam(++ssrc_counter_,
        kVideoTrackLabel2, kStreamLabel1));

    // kStreamLabel2 with 1 audio track and 1 video track
    options_.audio_sources.push_back(cricket::SourceParam(++ssrc_counter_,
        kAudioTrackLabel2, kStreamLabel2));
    options_.video_sources.push_back(cricket::SourceParam(++ssrc_counter_,
        kVideoTrackLabel3, kStreamLabel2));

    options_.is_video = true;
  }

 protected:
  talk_base::scoped_ptr<cricket::ChannelManager> channel_manager_;
  talk_base::scoped_ptr<cricket::MediaSessionDescriptionFactory>
      session_description_factory_;
  cricket::MediaSessionOptions options_;

 private:
  int ssrc_counter_;
};

TEST_F(PeerConnectionMessageTest, Serialize) {
  talk_base::scoped_ptr<cricket::SessionDescription> offer(
      session_description_factory_->CreateOffer(options_));

  std::vector<cricket::Candidate> candidates;
  // TODO(ronghuawu): Populate the test candidates.

  std::string message;
  scoped_refptr<PeerConnectionMessage> offer_message =
      PeerConnectionMessage::Create(PeerConnectionMessage::kOffer,
                                    offer.release(),
                                    candidates);
  EXPECT_TRUE(offer_message->Serialize(&message));
  LOG(LS_ERROR) << message;
  // TODO(ronghuawu): Verify the serialized message.
}
