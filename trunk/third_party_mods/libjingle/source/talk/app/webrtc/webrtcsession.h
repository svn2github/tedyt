/*
 * libjingle
 * Copyright 2004--2011, Google Inc.
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

#ifndef TALK_APP_WEBRTC_WEBRTCSESSION_H_
#define TALK_APP_WEBRTC_WEBRTCSESSION_H_

#include <string>
#include <vector>

#include "talk/base/logging.h"
#include "talk/base/messagehandler.h"
#include "talk/p2p/base/candidate.h"
#include "talk/p2p/base/session.h"
#include "talk/session/phone/channel.h"
#include "talk/session/phone/mediachannel.h"

namespace cricket {
class ChannelManager;
class Transport;
class TransportChannel;
class VoiceChannel;
class VideoChannel;
struct ConnectionInfo;
}

namespace Json {
class Value;
}

namespace webrtc {

struct StreamInfo {
  explicit StreamInfo(const std::string stream_id)
    : channel(NULL),
      transport(NULL),
      video(false),
      stream_id(stream_id) {}

  StreamInfo()
    : channel(NULL),
      transport(NULL),
      video(false) {}

  cricket::BaseChannel* channel;
  cricket::TransportChannel* transport;
  bool video;
  std::string stream_id;
};

typedef std::vector<cricket::AudioCodec> AudioCodecs;
typedef std::vector<cricket::VideoCodec> VideoCodecs;

class WebRTCSession : public cricket::BaseSession {
 public:
  WebRTCSession(const std::string& id,
                    const std::string& direction,
                    cricket::PortAllocator* allocator,
                    cricket::ChannelManager* channelmgr,
                    talk_base::Thread* signaling_thread);

  ~WebRTCSession();

  bool Initiate();
  bool Connect();
  bool OnRemoteDescription(cricket::SessionDescription* sdp,
      const std::vector<cricket::Candidate>& candidates);
  bool OnInitiateMessage(cricket::SessionDescription* sdp,
      const std::vector<cricket::Candidate>& candidates);
  void OnMute(bool mute);
  void OnCameraMute(bool mute);
  bool CreateVoiceChannel(const std::string& stream_id);
  bool CreateVideoChannel(const std::string& stream_id);
  bool RemoveStream(const std::string& stream_id);
  void RemoveAllStreams();

  // Returns true if we have either a voice or video stream matching this label.
  bool HasStream(const std::string& label) const;
  bool HasStream(bool video) const;

  // Returns true if there's one or more audio channels in the session.
  bool HasAudioStream() const;

  // Returns true if there's one or more video channels in the session.
  bool HasVideoStream() const;

  bool SetVideoRenderer(const std::string& stream_id,
                        cricket::VideoRenderer* renderer);

  sigslot::signal1<WebRTCSession*> SignalRemoveStream;
  sigslot::signal2<const std::string&, bool> SignalAddStream;
  sigslot::signal2<const std::string&, bool> SignalRemoveStream2;
  sigslot::signal2<const std::string&, bool> SignalRtcMediaChannelCreated;
  // Triggered when the local candidate is ready
  sigslot::signal2<const cricket::SessionDescription*,
      const std::vector<cricket::Candidate>&> SignalLocalDescription;
  // This callback will trigger if setting up a call times out.
  sigslot::signal0<> SignalFailedCall;

  bool muted() const { return muted_; }
  bool camera_muted() const { return camera_muted_; }
  const std::vector<cricket::Candidate>& local_candidates() {
    return local_candidates_;
  }
  const std::string& id() const { return id_; }
  bool incoming() const { return incoming_; }
  cricket::PortAllocator* port_allocator() const { return port_allocator_; }
  talk_base::Thread* signaling_thread() const { return signaling_thread_; }

 protected:
  // methods from cricket::BaseSession
  virtual void SetError(cricket::BaseSession::Error error);
  virtual cricket::TransportChannel* CreateChannel(
      const std::string& content_name, const std::string& name);
  virtual cricket::TransportChannel* GetChannel(
      const std::string& content_name, const std::string& name);
  virtual void DestroyChannel(
      const std::string& content_name, const std::string& name);

 private:
  // Dummy functions inherited from cricket::BaseSession.
  // They should never be called.
  virtual bool Accept(const cricket::SessionDescription* sdesc) {
    return true;
  }
  virtual bool Reject(const std::string& reason) {
    return true;
  }
  virtual bool TerminateWithReason(const std::string& reason) {
    return true;
  }
  virtual talk_base::Thread* worker_thread();

  // methods signaled by the transport
  void OnRequestSignaling(cricket::Transport* transport);
  void OnCandidatesReady(cricket::Transport* transport,
                         const std::vector<cricket::Candidate>& candidates);
  void OnWritableState(cricket::Transport* transport);
  void OnTransportError(cricket::Transport* transport);
  void OnChannelGone(cricket::Transport* transport);

  bool CheckForStreamDeleteMessage(
      const std::vector<cricket::Candidate>& candidates);

  void UpdateTransportWritableState();
  bool CheckAllTransportsWritable();
  void StartTransportTimeout(int timeout);
  void NotifyTransportState();

  cricket::SessionDescription* CreateOffer();
  cricket::SessionDescription* CreateAnswer(
      const cricket::SessionDescription* answer);

  // from MessageHandler
  virtual void OnMessage(talk_base::Message* message);

  virtual cricket::Transport* CreateTransport();
  cricket::Transport* GetTransport();

  typedef std::map<std::string, cricket::TransportChannel*> TransportChannelMap;

  bool SetVideoCapture(bool capture);
  void DisableLocalCandidate(const std::string& name);
  bool OnRemoteDescriptionUpdate(const cricket::SessionDescription* desc,
      const std::vector<cricket::Candidate>& candidates);
  void RemoveStreamOnRequest(const cricket::Candidate& candidate);
  void EnableAllStreams();

  cricket::Transport* transport_;
  cricket::ChannelManager* channel_manager_;
  std::vector<StreamInfo*> streams_;
  TransportChannelMap transport_channels_;
  bool all_transports_writable_;
  bool muted_;
  bool camera_muted_;
  int setup_timeout_;
  std::vector<cricket::Candidate> local_candidates_;

  talk_base::Thread* signaling_thread_;
  std::string id_;
  bool incoming_;
  cricket::PortAllocator* port_allocator_;

  static const char kIncomingDirection[];
  static const char kOutgoingDirection[];
};

}  // namespace webrtc

#endif  // TALK_APP_WEBRTC_WEBRTCSESSION_H_
