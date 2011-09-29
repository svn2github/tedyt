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

#include "talk/app/webrtc_dev/webrtcjson.h"

#include <stdio.h>
#include <string>

#include "talk/base/json.h"
#include "talk/base/logging.h"
#include "talk/base/stringutils.h"
#include "talk/session/phone/codec.h"
#include "talk/session/phone/mediasessionclient.h"

namespace webrtc {
static const int kIceComponent = 1;
static const int kIceFoundation = 1;

static std::vector<Json::Value> ReadValues(const Json::Value& value,
                                           const std::string& key);

static bool BuildContent(
    const cricket::SessionDescription* sdp,
    const cricket::ContentInfo& content_info,
    const std::vector<cricket::Candidate>& candidates,
    bool video,
    Json::Value* content);

static bool BuildCandidate(const std::vector<cricket::Candidate>& candidates,
                           bool video,
                           std::vector<Json::Value>* jcandidates);

static bool BuildRtpMapParams(const cricket::ContentInfo& audio_offer,
                              bool video,
                              std::vector<Json::Value>* rtpmap);

static bool BuildTrack(const cricket::SessionDescription* sdp,
                       bool video,
                       std::vector<Json::Value>* track);

static std::string Serialize(const Json::Value& value);

static bool Deserialize(const std::string& message, Json::Value& value);

static bool ParseRtcpMux(const Json::Value& value);
static bool ParseAudioCodec(const Json::Value& value,
                            cricket::AudioContentDescription* content);
static bool ParseVideoCodec(const Json::Value& value,
                            cricket::VideoContentDescription* content);
static bool ParseIceCandidates(const Json::Value& value,
                               std::vector<cricket::Candidate>* candidates);
static Json::Value ReadValue(const Json::Value& value, const std::string& key);
static std::string ReadString(const Json::Value& value, const std::string& key);
static double ReadDouble(const Json::Value& value, const std::string& key);
static uint32 ReadUInt(const Json::Value& value, const std::string& key);

static void Append(Json::Value* object, const std::string& key, bool value);
static void Append(Json::Value* object, const std::string& key,
                   const char* value);
static void Append(Json::Value* object, const std::string& key, int value);
static void Append(Json::Value* object, const std::string& key,
                   const std::string& value);
static void Append(Json::Value* object, const std::string& key, uint32 value);
static void Append(Json::Value* object, const std::string& key,
                   const Json::Value& value);
static void Append(Json::Value* object,
                   const std::string& key,
                   const std::vector<Json::Value>& values);

bool JsonSerialize(
    const cricket::SessionDescription* sdp,
    const std::vector<cricket::Candidate>& candidates,
    std::string* signaling_message) {
  const cricket::ContentInfo* audio_content = GetFirstAudioContent(sdp);
  const cricket::ContentInfo* video_content = GetFirstVideoContent(sdp);

  Json::Value media;
  std::vector<Json::Value> together;
  together.push_back("audio");
  together.push_back("video");

  std::vector<Json::Value> contents;

  if (audio_content) {
    Json::Value content;
    BuildContent(sdp, *audio_content, candidates, false, &content);
    contents.push_back(content);
  }

  if (video_content) {
    Json::Value content;
    BuildContent(sdp, *video_content, candidates, true, &content);
    contents.push_back(content);
  }

  Append(&media, "content", contents);
  Append(&media, "TOGETHER", together);

  // Now serialize.
  *signaling_message = Serialize(media);

  return true;
}

bool BuildContent(
    const cricket::SessionDescription* sdp,
    const cricket::ContentInfo& content_info,
    const std::vector<cricket::Candidate>& candidates,
    bool video,
    Json::Value* content) {
  std::string label("media");
  // TODO(ronghuawu): Use enum instead of bool video to prepare for other
  // media types such as the data media stream.
  if (video) {
    Append(content, label, "video");
  } else {
    Append(content, label, "audio");
  }

  const cricket::MediaContentDescription* media_info =
      static_cast<const cricket::MediaContentDescription*> (
          content_info.description);
  if (media_info->rtcp_mux()) {
    Append(content, "rtcp_mux", true);
  }

  // rtpmap
  std::vector<Json::Value> rtpmap;
  BuildRtpMapParams(content_info, video, &rtpmap);
  Append(content, "rtpmap", rtpmap);

  // crypto
  Json::Value crypto;
  // TODO(ronghuawu): BuildCrypto
  Append(content, "crypto", crypto);

  // candidate
  std::vector<Json::Value> jcandidates;
  BuildCandidate(candidates, video, &jcandidates);
  Append(content, "candidate", jcandidates);

  // track
  std::vector<Json::Value> track;
  BuildTrack(sdp, video, &track);
  Append(content, "track", track);

  return true;
}

bool BuildRtpMapParams(const cricket::ContentInfo& content_info,
                       bool video,
                       std::vector<Json::Value>* rtpmap) {
  if (!video) {
    const cricket::AudioContentDescription* audio_offer =
        static_cast<const cricket::AudioContentDescription*>(
            content_info.description);

    std::vector<cricket::AudioCodec>::const_iterator iter =
        audio_offer->codecs().begin();
    std::vector<cricket::AudioCodec>::const_iterator iter_end =
        audio_offer->codecs().end();
    for (; iter != iter_end; ++iter) {
      Json::Value codec;
      std::string codec_str(std::string("audio/").append(iter->name));
      // adding clockrate
      Append(&codec, "clockrate", iter->clockrate);
      Append(&codec, "codec", codec_str);
      Json::Value codec_id;
      Append(&codec_id, talk_base::ToString(iter->id), codec);
      rtpmap->push_back(codec_id);
    }
  } else {
    const cricket::VideoContentDescription* video_offer =
        static_cast<const cricket::VideoContentDescription*>(
            content_info.description);

    std::vector<cricket::VideoCodec>::const_iterator iter =
        video_offer->codecs().begin();
    std::vector<cricket::VideoCodec>::const_iterator iter_end =
        video_offer->codecs().end();
    for (; iter != iter_end; ++iter) {
      Json::Value codec;
      std::string codec_str(std::string("video/").append(iter->name));
      Append(&codec, "codec", codec_str);
      Json::Value codec_id;
      Append(&codec_id, talk_base::ToString(iter->id), codec);
      rtpmap->push_back(codec_id);
    }
  }
  return true;
}

bool BuildCandidate(const std::vector<cricket::Candidate>& candidates,
                    bool video,
                    std::vector<Json::Value>* jcandidates) {
  std::vector<cricket::Candidate>::const_iterator iter =
      candidates.begin();
  std::vector<cricket::Candidate>::const_iterator iter_end =
      candidates.end();
  for (; iter != iter_end; ++iter) {
    if ((video && (!iter->name().compare("video_rtcp") ||
                  (!iter->name().compare("video_rtp")))) ||
        (!video && (!iter->name().compare("rtp") ||
                   (!iter->name().compare("rtcp"))))) {
      Json::Value jcandidate;
      Append(&jcandidate, "component", kIceComponent);
      Append(&jcandidate, "foundation", kIceFoundation);
      Append(&jcandidate, "generation", iter->generation());
      Append(&jcandidate, "proto", iter->protocol());
      Append(&jcandidate, "priority", iter->preference_str());
      Append(&jcandidate, "ip", iter->address().IPAsString());
      Append(&jcandidate, "port", iter->address().PortAsString());
      Append(&jcandidate, "type", iter->type());
      Append(&jcandidate, "name", iter->name());
      Append(&jcandidate, "network_name", iter->network_name());
      Append(&jcandidate, "username", iter->username());
      Append(&jcandidate, "password", iter->password());
      jcandidates->push_back(jcandidate);
    }
  }
  return true;
}

bool BuildTrack(const cricket::SessionDescription* sdp,
                bool video,
                std::vector<Json::Value>* tracks) {
  const cricket::ContentInfo* content;
  if (video)
    content = GetFirstVideoContent(sdp);
  else
    content = GetFirstAudioContent(sdp);

  if (!content)
    return false;

  const cricket::MediaContentDescription* desc =
      static_cast<const cricket::MediaContentDescription*>(
          content->description);
  for (cricket::Sources::const_iterator it = desc->sources().begin();
       it != desc->sources().end();
       ++it) {
    Json::Value track;
    Append(&track, "ssrc", it->ssrc);
    Append(&track, "cname", it->cname);
    tracks->push_back(track);
  }
  return true;
}

std::string Serialize(const Json::Value& value) {
  Json::StyledWriter writer;
  return writer.write(value);
}

bool Deserialize(const std::string& message, Json::Value* value) {
  Json::Reader reader;
  return reader.parse(message, *value);
}

bool JsonDeserialize(const std::string& signaling_message,
                     cricket::SessionDescription** sdp,
                     std::vector<cricket::Candidate>* candidates) {
  ASSERT(!(*sdp));  // expect this to be NULL
  // first deserialize message
  Json::Value value;
  if (!Deserialize(signaling_message, &value)) {
    return false;
  }

  // get media objects
  std::vector<Json::Value> mlines = ReadValues(value, "media");
  if (mlines.empty()) {
    // no m-lines found
    return false;
  }

  *sdp = new cricket::SessionDescription();

  // get codec information
  for (size_t i = 0; i < mlines.size(); ++i) {
    if (mlines[i]["label"].asInt() == 1) {
      cricket::AudioContentDescription* audio_content =
          new cricket::AudioContentDescription();
      ParseAudioCodec(mlines[i], audio_content);
      audio_content->set_rtcp_mux(ParseRtcpMux(mlines[i]));
      audio_content->SortCodecs();
      (*sdp)->AddContent(cricket::CN_AUDIO,
                         cricket::NS_JINGLE_RTP, audio_content);
      ParseIceCandidates(mlines[i], candidates);
    } else {
      cricket::VideoContentDescription* video_content =
          new cricket::VideoContentDescription();
      ParseVideoCodec(mlines[i], video_content);

      video_content->set_rtcp_mux(ParseRtcpMux(mlines[i]));
      video_content->SortCodecs();
      (*sdp)->AddContent(cricket::CN_VIDEO,
                         cricket::NS_JINGLE_RTP, video_content);
      ParseIceCandidates(mlines[i], candidates);
    }
  }
  return true;
}

bool ParseRtcpMux(const Json::Value& value) {
  Json::Value rtcp_mux(ReadValue(value, "rtcp_mux"));
  if (!rtcp_mux.empty()) {
    if (rtcp_mux.asBool()) {
      return true;
    }
  }
  return false;
}

bool ParseAudioCodec(const Json::Value& value,
                     cricket::AudioContentDescription* content) {
  std::vector<Json::Value> rtpmap(ReadValues(value, "rtpmap"));
  if (rtpmap.empty())
    return false;

  std::vector<Json::Value>::const_iterator iter =
      rtpmap.begin();
  std::vector<Json::Value>::const_iterator iter_end =
      rtpmap.end();
  for (; iter != iter_end; ++iter) {
    cricket::AudioCodec codec;
    std::string pltype(iter->begin().memberName());
    talk_base::FromString(pltype, &codec.id);
    Json::Value codec_info((*iter)[pltype]);
    std::string codec_name(ReadString(codec_info, "codec"));
    std::vector<std::string> tokens;
    talk_base::split(codec_name, '/', &tokens);
    codec.name = tokens[1];
    codec.clockrate = ReadUInt(codec_info, "clockrate");
    content->AddCodec(codec);
  }

  return true;
}

bool ParseVideoCodec(const Json::Value& value,
                     cricket::VideoContentDescription* content) {
  std::vector<Json::Value> rtpmap(ReadValues(value, "rtpmap"));
  if (rtpmap.empty())
    return false;

  std::vector<Json::Value>::const_iterator iter =
      rtpmap.begin();
  std::vector<Json::Value>::const_iterator iter_end =
      rtpmap.end();
  for (; iter != iter_end; ++iter) {
    cricket::VideoCodec codec;
    std::string pltype(iter->begin().memberName());
    talk_base::FromString(pltype, &codec.id);
    Json::Value codec_info((*iter)[pltype]);
    std::vector<std::string> tokens;
    talk_base::split(codec_info["codec"].asString(), '/', &tokens);
    codec.name = tokens[1];
    content->AddCodec(codec);
  }
  return true;
}

bool ParseIceCandidates(const Json::Value& value,
                        std::vector<cricket::Candidate>* candidates) {
  Json::Value attributes(ReadValue(value, "attributes"));
  std::string ice_pwd(ReadString(attributes, "ice-pwd"));
  std::string ice_ufrag(ReadString(attributes, "ice-ufrag"));

  std::vector<Json::Value> jcandidates(ReadValues(attributes, "candidate"));

  std::vector<Json::Value>::const_iterator iter =
      jcandidates.begin();
  std::vector<Json::Value>::const_iterator iter_end =
      jcandidates.end();
  for (; iter != iter_end; ++iter) {
    cricket::Candidate cand;

    unsigned int generation;
    if (!GetUIntFromJsonObject(*iter, "generation", &generation))
      return false;
    cand.set_generation_str(talk_base::ToString(generation));

    std::string proto;
    if (!GetStringFromJsonObject(*iter, "proto", &proto))
      return false;
    cand.set_protocol(proto);

    std::string priority;
    if (!GetStringFromJsonObject(*iter, "priority", &priority))
      return false;
    cand.set_preference_str(priority);

    std::string str;
    talk_base::SocketAddress addr;
    if (!GetStringFromJsonObject(*iter, "ip", &str))
      return false;
    addr.SetIP(str);
    if (!GetStringFromJsonObject(*iter, "port", &str))
      return false;
    int port;
    if (!talk_base::FromString(str, &port))
      return false;
    addr.SetPort(port);
    cand.set_address(addr);

    if (!GetStringFromJsonObject(*iter, "type", &str))
      return false;
    cand.set_type(str);

    if (!GetStringFromJsonObject(*iter, "name", &str))
      return false;
    cand.set_name(str);

    if (!GetStringFromJsonObject(*iter, "network_name", &str))
      return false;
    cand.set_network_name(str);

    if (!GetStringFromJsonObject(*iter, "username", &str))
      return false;
    cand.set_username(str);

    if (!GetStringFromJsonObject(*iter, "password", &str))
      return false;
    cand.set_password(str);

    candidates->push_back(cand);
  }
  return true;
}

std::vector<Json::Value> ReadValues(
    const Json::Value& value, const std::string& key) {
  std::vector<Json::Value> objects;
  for (Json::Value::ArrayIndex i = 0; i < value[key].size(); ++i) {
    objects.push_back(value[key][i]);
  }
  return objects;
}

Json::Value ReadValue(const Json::Value& value, const std::string& key) {
  return value[key];
}

std::string ReadString(const Json::Value& value, const std::string& key) {
  return value[key].asString();
}

uint32 ReadUInt(const Json::Value& value, const std::string& key) {
  return value[key].asUInt();
}

double ReadDouble(const Json::Value& value, const std::string& key) {
  return value[key].asDouble();
}

void Append(Json::Value* object, const std::string& key, bool value) {
  (*object)[key] = Json::Value(value);
}

void Append(Json::Value* object, const std::string& key, const char* value) {
  (*object)[key] = Json::Value(value);
}

void Append(Json::Value* object, const std::string& key, int value) {
  (*object)[key] = Json::Value(value);
}

void Append(Json::Value* object, const std::string& key,
            const std::string& value) {
  (*object)[key] = Json::Value(value);
}

void Append(Json::Value* object, const std::string& key, uint32 value) {
  (*object)[key] = Json::Value(value);
}

void Append(Json::Value* object, const std::string& key,
            const Json::Value& value) {
  (*object)[key] = value;
}

void Append(Json::Value* object,
            const std::string & key,
            const std::vector<Json::Value>& values) {
  for (std::vector<Json::Value>::const_iterator iter = values.begin();
      iter != values.end(); ++iter) {
    (*object)[key].append(*iter);
  }
}

}  // namespace webrtc
