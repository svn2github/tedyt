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
#include "talk/app/webrtc_dev/mediastreamimpl.h"

namespace webrtc {

scoped_refptr<LocalMediaStream> CreateLocalMediaStream(
    const std::string& label) {
  return MediaStreamImpl::Create(label);
}

scoped_refptr<MediaStreamImpl> MediaStreamImpl::Create(
    const std::string& label) {
  RefCountImpl<MediaStreamImpl>* stream =
      new RefCountImpl<MediaStreamImpl>(label);
  return stream;
}

MediaStreamImpl::MediaStreamImpl(const std::string& label)
    : label_(label),
      ready_state_(MediaStream::kInitializing),
      track_list_(new RefCountImpl<MediaStreamTrackListImpl>()) {
}

const std::string& MediaStreamImpl::label() {
  return label_;
}

scoped_refptr<MediaStreamTrackList> MediaStreamImpl::tracks() {
  return track_list_;
}

MediaStream::ReadyState MediaStreamImpl::ready_state() {
  return ready_state_;
}

void MediaStreamImpl::set_ready_state(MediaStream::ReadyState new_state) {
  ready_state_ = new_state;
  NotifierImpl<LocalMediaStream>::FireOnChanged();
}

bool MediaStreamImpl::AddTrack(MediaStreamTrack* track) {
  if (ready_state() != kInitializing)
    return false;

  track_list_->AddTrack(track);
  return true;
}

void MediaStreamImpl::MediaStreamTrackListImpl::AddTrack(
    MediaStreamTrack* track) {
  tracks_.push_back(track);
  NotifierImpl<MediaStreamTrackList>::FireOnChanged();
}

size_t MediaStreamImpl::MediaStreamTrackListImpl::count() {
  return tracks_.size();
}

scoped_refptr<MediaStreamTrack> MediaStreamImpl::MediaStreamTrackListImpl::at(
    size_t index) {
  return tracks_.at(index);
}

}  // namespace webrtc
