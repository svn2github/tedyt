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

#include "talk/app/webrtc_dev/mediastreamhandler.h"

#include "talk/app/webrtc_dev/scoped_refptr_msg.h"

#ifdef WEBRTC_RELATIVE_PATH
#include "modules/video_capture/main/interface/video_capture.h"
#else
#include "third_party/webrtc/files/include/video_capture.h"
#endif

namespace webrtc {

enum {
  MSG_TRACK_STATECHANGED = 1,
  MSG_TRACK_RENDERERCHANGED = 2,
  MSG_TRACK_ENABLEDCHANGED = 3,
};

typedef talk_base::TypedMessageData<MediaStreamTrackInterface::TrackState>
    TrackStateMessageData;
typedef talk_base::TypedMessageData<bool> TrackEnabledMessageData;

VideoTrackHandler::VideoTrackHandler(VideoTrackInterface* track,
                                     MediaProviderInterface* provider)
    : provider_(provider),
      video_track_(track),
      state_(track->state()),
      enabled_(track->enabled()),
      renderer_(track->GetRenderer()),
      signaling_thread_(talk_base::Thread::Current()) {
  video_track_->RegisterObserver(this);
}

VideoTrackHandler::~VideoTrackHandler() {
  video_track_->UnregisterObserver(this);
}

void VideoTrackHandler::OnChanged() {
  if (state_ != video_track_->state()) {
    state_ = video_track_->state();
    TrackStateMessageData* state_param(new TrackStateMessageData(state_));
    signaling_thread_->Post(this, MSG_TRACK_STATECHANGED, state_param);
  }
  if (renderer_.get() != video_track_->GetRenderer()) {
    renderer_ = video_track_->GetRenderer();
    signaling_thread_->Post(this, MSG_TRACK_RENDERERCHANGED);
  }
  if (enabled_ != video_track_->enabled()) {
    enabled_ =video_track_->enabled();
    TrackEnabledMessageData* enabled_param(
        new TrackEnabledMessageData(enabled_));
    signaling_thread_->Post(this, MSG_TRACK_ENABLEDCHANGED, enabled_param);
  }
}

void VideoTrackHandler::OnMessage(talk_base::Message* msg) {
  switch (msg->message_id) {
    case MSG_TRACK_STATECHANGED: {
      TrackStateMessageData* data =
          static_cast<TrackStateMessageData*>(msg->pdata);
      OnStateChanged(data->data());
      delete data;
      break;
    }
    case MSG_TRACK_RENDERERCHANGED: {
      OnRendererChanged();
      break;
    }
    case MSG_TRACK_ENABLEDCHANGED: {
      TrackEnabledMessageData* data =
          static_cast<TrackEnabledMessageData*>(msg->pdata);
      OnEnabledChanged(data->data());
      break;
    }
  }
}

LocalVideoTrackHandler::LocalVideoTrackHandler(
    LocalVideoTrackInterface* track,
    MediaProviderInterface* provider)
    : VideoTrackHandler(track, provider),
      local_video_track_(track) {
}

void LocalVideoTrackHandler::OnRendererChanged() {
  VideoRendererInterface* renderer(video_track_->GetRenderer());
  if (renderer)
    provider_->SetLocalRenderer(video_track_->ssrc(), renderer->renderer());
  else
    provider_->SetLocalRenderer(video_track_->ssrc(), NULL);
}

void LocalVideoTrackHandler::OnStateChanged(
    MediaStreamTrackInterface::TrackState state) {
  if (state == VideoTrackInterface::kLive) {
    provider_->SetCaptureDevice(local_video_track_->ssrc(),
                                local_video_track_->GetVideoCapture());
    VideoRendererInterface* renderer(video_track_->GetRenderer());
    if (renderer)
      provider_->SetLocalRenderer(video_track_->ssrc(), renderer->renderer());
    else
      provider_->SetLocalRenderer(video_track_->ssrc(), NULL);
  }
}

void LocalVideoTrackHandler::OnEnabledChanged(bool enabled) {
  // TODO(perkj) What should happen when enabled is changed?
}

RemoteVideoTrackHandler::RemoteVideoTrackHandler(
    VideoTrackInterface* track,
    MediaProviderInterface* provider)
    : VideoTrackHandler(track, provider),
      remote_video_track_(track) {
}

void RemoteVideoTrackHandler::OnRendererChanged() {
  VideoRendererInterface* renderer(video_track_->GetRenderer());
  if (renderer)
    provider_->SetRemoteRenderer(video_track_->ssrc(), renderer->renderer());
  else
    provider_->SetRemoteRenderer(video_track_->ssrc(), NULL);
}

void RemoteVideoTrackHandler::OnStateChanged(
    MediaStreamTrackInterface::TrackState state) {
}

void RemoteVideoTrackHandler::OnEnabledChanged(bool enabled) {
  // TODO(perkj): What should happen when enabled is changed?
}

MediaStreamHandler::MediaStreamHandler(MediaStreamInterface* stream,
                                       MediaProviderInterface* provider)
    : stream_(stream),
      provider_(provider) {
}

MediaStreamHandler::~MediaStreamHandler() {
  for (VideoTrackHandlers::iterator it = video_handlers_.begin();
       it != video_handlers_.end(); ++it) {
    delete *it;
  }
}

MediaStreamInterface* MediaStreamHandler::stream() {
  return stream_.get();
}

void MediaStreamHandler::OnChanged() {
  // TODO(perkj): Implement state change and enabled changed.
}


LocalMediaStreamHandler::LocalMediaStreamHandler(
    MediaStreamInterface* stream,
    MediaProviderInterface* provider)
    : MediaStreamHandler(stream, provider) {
  MediaStreamTrackListInterface* tracklist(stream->tracks());

  for (size_t j = 0; j < tracklist->count(); ++j) {
    MediaStreamTrackInterface* track = tracklist->at(j);
    if (track->type() == MediaStreamTrackInterface::kVideo) {
      LocalVideoTrackInterface* video_track =
          static_cast<LocalVideoTrackInterface*>(track);
      VideoTrackHandler* handler(new LocalVideoTrackHandler(video_track,
                                                            provider));
      video_handlers_.push_back(handler);
    }
  }
}

RemoteMediaStreamHandler::RemoteMediaStreamHandler(
    MediaStreamInterface* stream,
    MediaProviderInterface* provider)
    : MediaStreamHandler(stream, provider) {
  MediaStreamTrackListInterface* tracklist(stream->tracks());

  for (size_t j = 0; j < tracklist->count(); ++j) {
    MediaStreamTrackInterface* track = tracklist->at(j);
    if (track->type() == MediaStreamTrackInterface::kVideo) {
      VideoTrackInterface* video_track =
          static_cast<VideoTrackInterface*>(track);
      VideoTrackHandler* handler(new RemoteVideoTrackHandler(video_track,
                                                             provider));
      video_handlers_.push_back(handler);
    }
  }
}

MediaStreamHandlers::MediaStreamHandlers(MediaProviderInterface* provider)
    : provider_(provider) {
}

MediaStreamHandlers::~MediaStreamHandlers() {
  for (StreamHandlerList::iterator it = remote_streams_handlers_.begin();
       it != remote_streams_handlers_.end(); ++it) {
    delete *it;
  }
  for (StreamHandlerList::iterator it = local_streams_handlers_.begin();
       it != local_streams_handlers_.end(); ++it) {
    delete *it;
  }
}

void MediaStreamHandlers::AddRemoteStream(MediaStreamInterface* stream) {
  RemoteMediaStreamHandler* handler = new RemoteMediaStreamHandler(stream,
                                                                   provider_);
  remote_streams_handlers_.push_back(handler);
}

void MediaStreamHandlers::RemoveRemoteStream(MediaStreamInterface* stream) {
  StreamHandlerList::iterator it = remote_streams_handlers_.begin();
  for (; it != remote_streams_handlers_.end(); ++it) {
    if ((*it)->stream() == stream) {
      delete *it;
      break;
    }
  }
  ASSERT(it != remote_streams_handlers_.end());
  remote_streams_handlers_.erase(it);
}

void MediaStreamHandlers::CommitLocalStreams(StreamCollection* streams) {
  // Iterate the old list of local streams.
  // If its not found in the new collection it have been removed.
  // We can not erase from the old collection at the same time as we iterate.
  // That is what the ugly while(1) fix.
  while (1) {
    StreamHandlerList::iterator it = local_streams_handlers_.begin();
    for (; it != local_streams_handlers_.end(); ++it) {
      if (streams->find((*it)->stream()->label()) == NULL) {
        delete *it;
        break;
      }
    }
    if (it != local_streams_handlers_.end()) {
      local_streams_handlers_.erase(it);
      continue;
    }
    break;
  }

  // Iterate the new collection of local streams.
  // If its not found in the old collection it have been added.
  for (size_t j = 0; j < streams->count(); ++j) {
    MediaStreamInterface* stream = streams->at(j);
    StreamHandlerList::iterator it = local_streams_handlers_.begin();
    for (; it != local_streams_handlers_.end(); ++it) {
      if (stream == (*it)->stream())
        break;
    }
    if (it == local_streams_handlers_.end()) {
      LocalMediaStreamHandler* handler = new LocalMediaStreamHandler(
          stream, provider_);
      local_streams_handlers_.push_back(handler);
    }
  }
};


}  // namespace webrtc
