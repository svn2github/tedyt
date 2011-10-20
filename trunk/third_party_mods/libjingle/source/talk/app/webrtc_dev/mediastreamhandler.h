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

// This file contains classes for listening on changes on MediaStreams and
// MediaTracks and making sure appropriate action is taken.
// Example: If a user sets a rendererer on a local video track the renderer is
// connected to the appropriate camera.

#ifndef TALK_APP_WEBRTC_DEV_MEDIASTREAMHANDLER_H_
#define TALK_APP_WEBRTC_DEV_MEDIASTREAMHANDLER_H_

#include <list>
#include <vector>

#include "talk/app/webrtc_dev/mediastream.h"
#include "talk/app/webrtc_dev/mediastreamprovider.h"
#include "talk/app/webrtc_dev/peerconnection.h"
#include "talk/base/thread.h"

namespace webrtc {

// VideoTrackHandler listen to events on a VideoTrack instance and
// executes the requested change.
class VideoTrackHandler : public ObserverInterface {
 public:
  VideoTrackHandler(VideoTrackInterface* track,
                    MediaProviderInterface* provider);
  virtual ~VideoTrackHandler();
  virtual void OnChanged();

 protected:
  virtual void OnRendererChanged() = 0;
  virtual void OnStateChanged() = 0;
  virtual void OnEnabledChanged() = 0;

  MediaProviderInterface* provider_;
  VideoTrackInterface* video_track_;

 private:
  MediaStreamTrackInterface::TrackState state_;
  bool enabled_;
  talk_base::scoped_refptr<VideoRendererWrapperInterface> renderer_;
};

class LocalVideoTrackHandler : public VideoTrackHandler {
 public:
  LocalVideoTrackHandler(LocalVideoTrackInterface* track,
                         MediaProviderInterface* provider);
  virtual ~LocalVideoTrackHandler();

 protected:
  virtual void OnRendererChanged();
  virtual void OnStateChanged();
  virtual void OnEnabledChanged();

 private:
  talk_base::scoped_refptr<LocalVideoTrackInterface> local_video_track_;
};

class RemoteVideoTrackHandler : public VideoTrackHandler {
 public:
  RemoteVideoTrackHandler(VideoTrackInterface* track,
                          MediaProviderInterface* provider);
  virtual ~RemoteVideoTrackHandler();

 protected:
  virtual void OnRendererChanged();
  virtual void OnStateChanged();
  virtual void OnEnabledChanged();

 private:
  talk_base::scoped_refptr<VideoTrackInterface> remote_video_track_;
};

class MediaStreamHandler : public ObserverInterface {
 public:
  MediaStreamHandler(MediaStreamInterface* stream,
                     MediaProviderInterface* provider);
  ~MediaStreamHandler();
  MediaStreamInterface* stream();
  virtual void OnChanged();

 protected:
  MediaProviderInterface* provider_;
  typedef std::vector<VideoTrackHandler*> VideoTrackHandlers;
  VideoTrackHandlers video_handlers_;
  talk_base::scoped_refptr<MediaStreamInterface> stream_;
};

class LocalMediaStreamHandler : public MediaStreamHandler {
 public:
  LocalMediaStreamHandler(MediaStreamInterface* stream,
                          MediaProviderInterface* provider);
};

class RemoteMediaStreamHandler : public MediaStreamHandler {
 public:
  RemoteMediaStreamHandler(MediaStreamInterface* stream,
                           MediaProviderInterface* provider);
};

class MediaStreamHandlers {
 public:
  explicit MediaStreamHandlers(MediaProviderInterface* provider);
  ~MediaStreamHandlers();
  void AddRemoteStream(MediaStreamInterface* stream);
  void RemoveRemoteStream(MediaStreamInterface* stream);
  void CommitLocalStreams(StreamCollectionInterface* streams);

 private:
  typedef std::list<MediaStreamHandler*> StreamHandlerList;
  StreamHandlerList local_streams_handlers_;
  StreamHandlerList remote_streams_handlers_;
  MediaProviderInterface* provider_;
};

}  // namespace webrtc

#endif  // TALK_APP_WEBRTC_MEDIASTREAMOBSERVER_H_

