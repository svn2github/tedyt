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

#ifndef TALK_APP_WEBRTC_MEDIASTREAMPROXY_H_
#define TALK_APP_WEBRTC_MEDIASTREAMPROXY_H_

#include <string>
#include <vector>

#include "talk/app/webrtc_dev/mediastreamimpl.h"
#include "talk/base/thread.h"

namespace webrtc {
using talk_base::scoped_refptr;

// MediaStreamProxy is a proxy for the MediaStream interface. The purpose is
// to make sure MediaStreamImpl is only accessed from the signaling thread.
// It can be used as a proxy for both local and remote MediaStreams.
class MediaStreamProxy : public LocalMediaStreamInterface,
                         public talk_base::MessageHandler {
 public:
  static scoped_refptr<MediaStreamProxy> Create(
      const std::string& label,
      talk_base::Thread* signaling_thread);

  static scoped_refptr<MediaStreamProxy> Create(
      const std::string& label,
      talk_base::Thread* signaling_thread,
      LocalMediaStreamInterface* media_stream_impl);

  // Implement LocalStream.
  virtual bool AddTrack(AudioTrackInterface* track);
  virtual bool AddTrack(VideoTrackInterface* track);

  // Implement MediaStream.
  virtual std::string label() const;
  virtual AudioTracks* audio_tracks() {
    return audio_tracks_;
  }
  virtual VideoTracks* video_tracks() {
    return video_tracks_;
  }
  virtual ReadyState ready_state();
  virtual void set_ready_state(ReadyState new_state);

  // Implement Notifier
  virtual void RegisterObserver(ObserverInterface* observer);
  virtual void UnregisterObserver(ObserverInterface* observer);

 protected:
  MediaStreamProxy(const std::string& label,
                   talk_base::Thread* signaling_thread,
                   LocalMediaStreamInterface* media_stream_impl);

  template <class T>
  class MediaStreamTrackListProxy : public MediaStreamTrackListInterface<T>,
                                    public talk_base::MessageHandler {
   public:
    MediaStreamTrackListProxy(talk_base::Thread* signaling_thread);

    void SetImplementation(MediaStreamTrackListInterface<T>* track_list);
    virtual size_t count();
    virtual T* at(size_t index);

   private:
    void Send(uint32 id, talk_base::MessageData* data) const;
    void OnMessage(talk_base::Message* msg);

    talk_base::scoped_refptr<MediaStreamTrackListInterface<T> > track_list_;
    mutable talk_base::Thread* signaling_thread_;
  };
  typedef MediaStreamTrackListProxy<AudioTrackInterface> AudioTrackListProxy;
  typedef MediaStreamTrackListProxy<VideoTrackInterface> VideoTrackListProxy;

  void Send(uint32 id, talk_base::MessageData* data) const;
  // Implement MessageHandler.
  virtual void OnMessage(talk_base::Message* msg);

  mutable talk_base::Thread* signaling_thread_;
  scoped_refptr<LocalMediaStreamInterface> media_stream_impl_;
  scoped_refptr<AudioTrackListProxy> audio_tracks_;
  scoped_refptr<VideoTrackListProxy> video_tracks_;
};

}  // namespace webrtc

#endif  // TALK_APP_WEBRTC_MEDIASTREAMPROXY_H_
