/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * vp8.cc
 *
 * This file contains the WEBRTC VP8 wrapper implementation
 *
 */
#include "vp8.h"
#include "tick_util.h"

#include "vpx/vpx_encoder.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vp8dx.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "module_common_types.h"

enum { kVp8ErrorPropagationTh = 30 };
//#define DEV_PIC_LOSS

namespace webrtc
{

VP8Encoder::VP8Encoder():
    _encodedImage(),
    _encodedCompleteCallback(NULL),
    _width(0),
    _height(0),
    _maxBitRateKbit(0),
    _inited(false),
    _timeStamp(0),
    _pictureID(0),
    _simulcastIdx(0),
    _pictureLossIndicationOn(false),
    _feedbackModeOn(false),
    _nextRefIsGolden(true),
    _lastAcknowledgedIsGolden(true),
    _haveReceivedAcknowledgement(false),
    _pictureIDLastSentRef(0),
    _pictureIDLastAcknowledgedRef(0),
    _cpuSpeed(-6), // default value
    _rcMaxIntraTarget(0),
    _tokenPartitions(VP8_ONE_TOKENPARTITION),
    _encoder(NULL),
    _cfg(NULL),
    _raw(NULL)
{
    srand((WebRtc_UWord32)TickTime::MillisecondTimestamp());
}

VP8Encoder::~VP8Encoder()
{
    Release();
}

WebRtc_Word32
VP8Encoder::VersionStatic(WebRtc_Word8* version, WebRtc_Word32 length)
{
    const char* str = vpx_codec_iface_name(vpx_codec_vp8_cx());
    WebRtc_Word32 verLen = (WebRtc_Word32)strlen(str);
    // Accounting for "\0" and "\n" (to be added a bit later)
    if (verLen + 2 > length)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    strcpy(version, str);
    strcat(version, "\n");
    return (verLen + 2);
}

WebRtc_Word32
VP8Encoder::Version(WebRtc_Word8 *version, WebRtc_Word32 length) const
{
    return VersionStatic(version, length);
}

WebRtc_Word32
VP8Encoder::Release()
{
    if (_encodedImage._buffer != NULL)
    {
        delete [] _encodedImage._buffer;
        _encodedImage._buffer = NULL;
    }
    if (_encoder != NULL)
    {
        if (vpx_codec_destroy(_encoder))
        {
            return WEBRTC_VIDEO_CODEC_MEMORY;
        }
        delete _encoder;
        _encoder = NULL;
    }
    if (_cfg != NULL)
    {
        delete _cfg;
        _cfg = NULL;
    }
    if (_raw != NULL)
    {
        vpx_img_free(_raw);
        delete _raw;
        _raw = NULL;
    }
    _inited = false;

    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Encoder::Reset()
{
    if (!_inited)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (_encoder != NULL)
    {
        if (vpx_codec_destroy(_encoder))
        {
            return WEBRTC_VIDEO_CODEC_MEMORY;
        }
        delete _encoder;
        _encoder = NULL;
    }

    _timeStamp = 0;

    _encoder = new vpx_codec_ctx_t;

    return InitAndSetControlSettings();
}

WebRtc_Word32
VP8Encoder::SetRates(WebRtc_UWord32 newBitRateKbit, WebRtc_UWord32 newFrameRate)
{
    if (!_inited)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (_encoder->err)
    {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    if (newFrameRate < 1)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    // update bit rate
    if (_maxBitRateKbit > 0 &&
        newBitRateKbit > static_cast<WebRtc_UWord32>(_maxBitRateKbit))
    {
        newBitRateKbit = _maxBitRateKbit;
    }

    _cfg->rc_target_bitrate = newBitRateKbit; // in kbit/s
/*  TODO(pwestin) use number of temoral layers config
    int ids[3] = {0,1,2};                                
    _cfg->ts_number_layers     = 3;
    _cfg->ts_periodicity       = 3;
    _cfg->ts_target_bitrate[0] = (newBitRateKbit*2/5);
    _cfg->ts_target_bitrate[1] = (newBitRateKbit*3/5);
    _cfg->ts_target_bitrate[2] = (newBitRateKbit);
    _cfg->ts_rate_decimator[0] = 4;
    _cfg->ts_rate_decimator[1] = 2;
    _cfg->ts_rate_decimator[2] = 1;
    memcpy(_cfg->ts_layer_id, ids, sizeof(ids));
*/

    _maxFrameRate = newFrameRate;

    // update encoder context
    if (vpx_codec_enc_config_set(_encoder, _cfg))
    {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Encoder::InitEncode(const VideoCodec* inst,
                       WebRtc_Word32 numberOfCores,
                       WebRtc_UWord32 /*maxPayloadSize */)
{
    if (inst == NULL)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (inst->maxFramerate < 1)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    // allow zero to represent an unspecified maxBitRate
    if (inst->maxBitrate > 0 && inst->startBitrate > inst->maxBitrate)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (inst->width < 1 || inst->height < 1)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (numberOfCores < 1)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
#ifdef DEV_PIC_LOSS
    // we need to know if we use feedback
    _feedbackModeOn = inst->codecSpecific.VP8.feedbackModeOn;
    _pictureLossIndicationOn = inst->codecSpecific.VP8.pictureLossIndicationOn;
#endif

    WebRtc_Word32 retVal = Release();
    if (retVal < 0)
    {
        return retVal;
    }
    if (_encoder == NULL)
    {
        _encoder = new vpx_codec_ctx_t;
    }
    if (_cfg == NULL)
    {
        _cfg = new vpx_codec_enc_cfg_t;
    }
    if (_raw == NULL)
    {
        _raw = new vpx_image_t;
    }

    _timeStamp = 0;
    _maxBitRateKbit = inst->maxBitrate;
    _maxFrameRate = inst->maxFramerate;
    _width = inst->width;
    _height = inst->height;

    // random start 16 bits is enough
    _pictureID = ((WebRtc_UWord16)rand()) % 0x7FFF;

    // allocate memory for encoded image
    if (_encodedImage._buffer != NULL)
    {
        delete [] _encodedImage._buffer;
    }
    _encodedImage._size = (3 * inst->width * inst->height) >> 1;
    _encodedImage._buffer = new WebRtc_UWord8[_encodedImage._size];
    _encodedImage._completeFrame = true;

    vpx_img_alloc(_raw, IMG_FMT_I420, inst->width, inst->height, 1);
    // populate encoder configuration with default values
    if (vpx_codec_enc_config_default(vpx_codec_vp8_cx(), _cfg, 0))
    {
         return WEBRTC_VIDEO_CODEC_ERROR;
    }

    _cfg->g_w = inst->width;
    _cfg->g_h = inst->height;
    if (_maxBitRateKbit > 0 &&
        inst->startBitrate > static_cast<unsigned int>(_maxBitRateKbit))
    {
        _cfg->rc_target_bitrate = _maxBitRateKbit;
    }
    else
    {
      _cfg->rc_target_bitrate = inst->startBitrate;  // in kbit/s
    }
/*  TODO(pwestin) use number of temoral layers config
    int ids[3] = {0,1,2};                                
    _cfg->ts_number_layers     = 3;
    _cfg->ts_periodicity       = 3;
    _cfg->ts_target_bitrate[0] = (inst->startBitrate*2/5);
    _cfg->ts_target_bitrate[1] = (inst->startBitrate*3/5);
    _cfg->ts_target_bitrate[2] = (inst->startBitrate);
    _cfg->ts_rate_decimator[0] = 4;
    _cfg->ts_rate_decimator[1] = 2;
    _cfg->ts_rate_decimator[2] = 1;
    memcpy(_cfg->ts_layer_id, ids, sizeof(ids));
*/

    // setting the time base of the codec
    _cfg->g_timebase.num = 1;
    _cfg->g_timebase.den = 90000;

#ifdef INDEPENDENT_PARTITIONS
    _cfg->g_error_resilient = VPX_ERROR_RESILIENT_DEFAULT | VPX_ERROR_RESILIENT_PARTITIONS;
#else
    _cfg->g_error_resilient = 1;
#endif
    _cfg->g_lag_in_frames = 0; // 0- no frame lagging

    // Determining number of threads based on the image size

    if (_width * _height > 704 * 576 && numberOfCores > 1)
      // 2 threads when larger than 4CIF
      _cfg->g_threads = 2;
    else
      _cfg->g_threads = 1;

    // rate control settings
    _cfg->rc_dropframe_thresh = 0;
    _cfg->rc_end_usage = VPX_CBR;
    _cfg->g_pass = VPX_RC_ONE_PASS;
    _cfg->rc_resize_allowed = 0;
    _cfg->rc_min_quantizer = 8;
    _cfg->rc_max_quantizer = 56;
    _cfg->rc_undershoot_pct = 100;
    _cfg->rc_overshoot_pct = 15;
    _cfg->rc_buf_initial_sz = 500;
    _cfg->rc_buf_optimal_sz = 600;
    _cfg->rc_buf_sz = 1000;
     // set the maximum target size of any key-frame.
    _rcMaxIntraTarget = MaxIntraTarget(_cfg->rc_buf_optimal_sz);

#ifdef DEV_PIC_LOSS
    // this can only be off if we know we use feedback
    if (_pictureLossIndicationOn)
    {
        // don't generate key frame unless we tell you
        _cfg->kf_mode = VPX_KF_DISABLED;
    }
    else
#endif
    {
        _cfg->kf_mode = VPX_KF_AUTO;
        _cfg->kf_max_dist = 3000;
    }

    switch (inst->codecSpecific.VP8.complexity)
    {
        case kComplexityHigh:
        {
            _cpuSpeed = -5;
            break;
        }
        case kComplexityHigher:
        {
            _cpuSpeed = -4;
            break;
        }
        case kComplexityMax:
        {
            _cpuSpeed = -3;
            break;
        }
        default:
        {
            _cpuSpeed = -6;
            break;
        }
    }

    return InitAndSetControlSettings();


}

WebRtc_Word32
VP8Encoder::InitAndSetControlSettings()
{
    // construct encoder context
    vpx_codec_enc_cfg_t cfg_copy = *_cfg;
    vpx_codec_flags_t flags = 0;
    // TODO(holmer): We should make a smarter decision on the number of
    // partitions. Eight is probably not the optimal number for low resolution
    // video.
    _tokenPartitions = VP8_EIGHT_TOKENPARTITION;
#if WEBRTC_LIBVPX_VERSION >= 971
    flags |= VPX_CODEC_USE_OUTPUT_PARTITION;
#endif
    if (vpx_codec_enc_init(_encoder, vpx_codec_vp8_cx(), _cfg, flags))
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    vpx_codec_control(_encoder, VP8E_SET_STATIC_THRESHOLD, 800);
    vpx_codec_control(_encoder, VP8E_SET_CPUUSED, _cpuSpeed);
    vpx_codec_control(_encoder, VP8E_SET_TOKEN_PARTITIONS,
                      static_cast<vp8e_token_partitions>(_tokenPartitions));
    vpx_codec_control(_encoder, VP8E_SET_NOISE_SENSITIVITY, 2);
#if WEBRTC_LIBVPX_VERSION >= 971
    vpx_codec_control(_encoder, VP8E_SET_MAX_INTRA_BITRATE_PCT,
                      _rcMaxIntraTarget);
#endif
    *_cfg = cfg_copy;

    _inited = true;
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_UWord32
VP8Encoder::MaxIntraTarget(WebRtc_UWord32 optimalBuffersize)
{
    // Set max to the optimal buffer level (normalized by target BR),
    // and scaled by a scalePar.
    // Max target size = scalePar * optimalBufferSize * targetBR[Kbps].
    // This values is presented in percentage of perFrameBw:
    // perFrameBw = targetBR[Kbps] * 1000 / frameRate.
    // The target in % is as follows:

    float scalePar = 0.5;
    WebRtc_UWord32 targetPct = (optimalBuffersize * scalePar) *
                              _maxFrameRate / 10;

    // Don't go below 3 times the per frame bandwidth.
    const WebRtc_UWord32 minIntraTh = 300;
    targetPct = (targetPct < minIntraTh) ? minIntraTh: targetPct;

    return  targetPct;
}

WebRtc_Word32
VP8Encoder::Encode(const RawImage& inputImage,
                   const CodecSpecificInfo* codecSpecificInfo,
                   const VideoFrameType* frameTypes)
{
    if (!_inited)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (inputImage._buffer == NULL)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (_encodedCompleteCallback == NULL)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (codecSpecificInfo) 
    {
        _simulcastIdx = codecSpecificInfo->codecSpecific.VP8.simulcastIdx;
    }
    else
    {
        _simulcastIdx = 0; 
    }
    // image in vpx_image_t format
    _raw->planes[PLANE_Y] =  inputImage._buffer;
    _raw->planes[PLANE_U] =  &inputImage._buffer[_height * _width];
    _raw->planes[PLANE_V] =  &inputImage._buffer[_height * _width * 5 >> 2];

    int flags = 0;
    if (frameTypes && *frameTypes == kKeyFrame)
    {
        flags |= VPX_EFLAG_FORCE_KF; // will update both golden and altref
        _encodedImage._frameType = kKeyFrame;
        _pictureIDLastSentRef = _pictureID;
    }
    else
    {
#ifdef DEV_PIC_LOSS
        if (_feedbackModeOn && codecSpecificInfo)
        {
            const CodecSpecificInfo* info = static_cast<const
                                     CodecSpecificInfo*>(codecSpecificInfo);
            if (info->codecType == kVideoCodecVP8)
            {
                // codecSpecificInfo will contain received RPSI and SLI
                // picture IDs. This will help us decide on when to switch type
                // of reference frame

                // if we receive SLI
                // force using an old golden or altref as a reference

                if (info->codecSpecific.VP8.hasReceivedSLI)
                {
                    // if this is older than my last acked ref we can ignore it
                    // info->codecSpecific.VP8.pictureIdSLI valid 6 bits => 64 frames

                    // since picture id can wrap check if in between our last sent and last acked

                    bool sendRefresh = false;
                    // check for a wrap in picture ID
                    if ((_pictureIDLastAcknowledgedRef & 0x3f) > (_pictureID & 0x3f))
                    {
                        // we have a wrap
                        if ( info->codecSpecific.VP8.pictureIdSLI > (_pictureIDLastAcknowledgedRef&0x3f)||
                            info->codecSpecific.VP8.pictureIdSLI < (_pictureID & 0x3f))
                        {
                            sendRefresh = true;
                        }
                    }
                    else if (info->codecSpecific.VP8.pictureIdSLI > (_pictureIDLastAcknowledgedRef&0x3f)&&
                             info->codecSpecific.VP8.pictureIdSLI < (_pictureID & 0x3f))
                    {
                        sendRefresh = true;
                    }

                    // right now we could also ignore it if it's older than our last sent ref since
                    // last sent ref only refers back to last acked
                    // _pictureIDLastSentRef;
                    if (sendRefresh)
                    {
                        flags |= VP8_EFLAG_NO_REF_LAST; // Don't reference the last frame

                        if (_haveReceivedAcknowledgement)
                        {
                            // we cant set this if we refer to a key frame
                            if (_lastAcknowledgedIsGolden)
                            {
                                flags |= VP8_EFLAG_NO_REF_ARF; // Don't reference the alternate reference frame
                            }
                            else
                            {
                                flags |= VP8_EFLAG_NO_REF_GF; // Don't reference the golden frame
                            }
                        }
                    }
                }
                if (info->codecSpecific.VP8.hasReceivedRPSI)
                {
                    if ((info->codecSpecific.VP8.pictureIdRPSI & 0x3fff) == (_pictureIDLastSentRef & 0x3fff)) // compare 14 bits
                    {
                        // remote peer have received our last reference frame
                        // switch frame type
                        _haveReceivedAcknowledgement = true;
                        _nextRefIsGolden = !_nextRefIsGolden;
                        _pictureIDLastAcknowledgedRef = _pictureIDLastSentRef;
                    }
                }
            }
            const WebRtc_UWord16 periodX = 64; // we need a period X to decide on the distance between golden and altref
            if (_pictureID % periodX == 0)
            {
                // only required if we have had a loss
                // however we don't acknowledge a SLI so if that is lost it's no good
                flags |= VP8_EFLAG_NO_REF_LAST; // Don't reference the last frame

                if (_nextRefIsGolden)
                {
                    flags |= VP8_EFLAG_FORCE_GF; // force a golden
                    flags |= VP8_EFLAG_NO_UPD_ARF; // don't update altref
                    if (_haveReceivedAcknowledgement)
                    {
                        // we can't set this if we refer to a key frame
                        // pw temporary as proof of concept
                        flags |= VP8_EFLAG_NO_REF_GF; // Don't reference the golden frame
                    }
                }
                else
                {
                    flags |= VP8_EFLAG_FORCE_ARF; // force an altref
                    flags |= VP8_EFLAG_NO_UPD_GF; // Don't update golden
                    if (_haveReceivedAcknowledgement)
                    {
                        // we can't set this if we refer to a key frame
                        // pw temporary as proof of concept
                        flags |= VP8_EFLAG_NO_REF_ARF; // Don't reference the alternate reference frame
                    }
                }
                // remember our last reference frame
                _pictureIDLastSentRef = _pictureID;
            }
            else
            {
                flags |= VP8_EFLAG_NO_UPD_GF;  // don't update golden
                flags |= VP8_EFLAG_NO_UPD_ARF; // don't update altref
            }
        }
#endif
        _encodedImage._frameType = kDeltaFrame;
    }

    // TODO(holmer): Ideally the duration should be the timestamp diff of this
    // frame and the next frame to be encoded, which we don't have. Instead we
    // would like to use the duration of the previous frame. Unfortunately the
    // rate control seems to be off with that setup. Using the average input
    // frame rate to calculate an average duration for now.
    assert(_maxFrameRate > 0);
    WebRtc_UWord32 duration = 90000 / _maxFrameRate;
    if (vpx_codec_encode(_encoder, _raw, _timeStamp, duration, flags,
                         VPX_DL_REALTIME))
    {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    _timeStamp += duration;

#if WEBRTC_LIBVPX_VERSION >= 971
    return GetEncodedPartitions(inputImage);
#else
    return GetEncodedFrame(inputImage);
#endif
}

void VP8Encoder::PopulateCodecSpecific(CodecSpecificInfo* codec_specific,
                                       const vpx_codec_cx_pkt& pkt) {
  assert(codec_specific != NULL);
  codec_specific->codecType = kVideoCodecVP8;
  CodecSpecificInfoVP8 *vp8Info = &(codec_specific->codecSpecific.VP8);
  vp8Info->pictureId = _pictureID;
  vp8Info->simulcastIdx = _simulcastIdx;;
  vp8Info->temporalIdx = kNoTemporalIdx; // TODO(pwestin) need to populate this
  vp8Info->nonReference = (pkt.data.frame.flags & VPX_FRAME_IS_DROPPABLE);
  _pictureID = (_pictureID + 1) % 0x7FFF; // prepare next
}

WebRtc_Word32
VP8Encoder::GetEncodedFrame(const RawImage& input_image)
{
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt= vpx_codec_get_cx_data(_encoder, &iter); // no lagging => 1 frame at a time
    if (pkt == NULL && !_encoder->err)
    {
        // dropped frame
        return WEBRTC_VIDEO_CODEC_OK;
    }
    else if (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
    {
        CodecSpecificInfo codecSpecific;
        PopulateCodecSpecific(&codecSpecific, *pkt);

        assert(pkt->data.frame.sz <= _encodedImage._size);
        memcpy(_encodedImage._buffer, pkt->data.frame.buf, pkt->data.frame.sz);
        _encodedImage._length = WebRtc_UWord32(pkt->data.frame.sz);
        _encodedImage._encodedHeight = _raw->h;
        _encodedImage._encodedWidth = _raw->w;

        // check if encoded frame is a key frame
        if (pkt->data.frame.flags & VPX_FRAME_IS_KEY)
        {
            _encodedImage._frameType = kKeyFrame;
        }

        if (_encodedImage._length > 0)
        {
            _encodedImage._timeStamp = input_image._timeStamp;

            // Figure out where partition boundaries are located.
            RTPFragmentationHeader fragInfo;
            fragInfo.VerifyAndAllocateFragmentationHeader(2); // two partitions: 1st and 2nd

            // First partition
            fragInfo.fragmentationOffset[0] = 0;
            WebRtc_UWord8 *firstByte = _encodedImage._buffer;
            WebRtc_UWord32 tmpSize = (firstByte[2] << 16) | (firstByte[1] << 8)
                | firstByte[0];
            fragInfo.fragmentationLength[0] = (tmpSize >> 5) & 0x7FFFF;
            fragInfo.fragmentationPlType[0] = 0; // not known here
            fragInfo.fragmentationTimeDiff[0] = 0;

            // Second partition
            fragInfo.fragmentationOffset[1] = fragInfo.fragmentationLength[0];
            fragInfo.fragmentationLength[1] = _encodedImage._length -
                fragInfo.fragmentationLength[0];
            fragInfo.fragmentationPlType[1] = 0; // not known here
            fragInfo.fragmentationTimeDiff[1] = 0;

            _encodedCompleteCallback->Encoded(_encodedImage, &codecSpecific,
                &fragInfo);
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }
    return WEBRTC_VIDEO_CODEC_ERROR;
}

#if WEBRTC_LIBVPX_VERSION >= 971
WebRtc_Word32
VP8Encoder::GetEncodedPartitions(const RawImage& input_image) {
  vpx_codec_iter_t iter = NULL;
  int part_idx = 0;
  _encodedImage._length = 0;
  RTPFragmentationHeader frag_info;
  frag_info.VerifyAndAllocateFragmentationHeader((1 << _tokenPartitions) + 1);
  CodecSpecificInfo codecSpecific;
  const vpx_codec_cx_pkt_t *pkt = NULL;
  while ((pkt = vpx_codec_get_cx_data(_encoder, &iter)) != NULL) {
    switch(pkt->kind) {
      case VPX_CODEC_CX_FRAME_PKT: {
        memcpy(&_encodedImage._buffer[_encodedImage._length],
               pkt->data.frame.buf,
               pkt->data.frame.sz);
        frag_info.fragmentationOffset[part_idx] = _encodedImage._length;
        frag_info.fragmentationLength[part_idx] =  pkt->data.frame.sz;
        frag_info.fragmentationPlType[part_idx] = 0;  // not known here
        frag_info.fragmentationTimeDiff[part_idx] = 0;
        _encodedImage._length += pkt->data.frame.sz;
        assert(_encodedImage._length <= _encodedImage._size);
        ++part_idx;
        break;
      }
      default: {
        break;
      }
    }
    // End of frame
    if ((pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT) == 0) {
      // check if encoded frame is a key frame
      if (pkt->data.frame.flags & VPX_FRAME_IS_KEY)
      {
          _encodedImage._frameType = kKeyFrame;
      }
      PopulateCodecSpecific(&codecSpecific, *pkt);
      break;
    }
  }
  if (_encodedImage._length == 0)
      return WEBRTC_VIDEO_CODEC_ERROR;
  _encodedImage._timeStamp = input_image._timeStamp;
  _encodedImage._encodedHeight = _raw->h;
  _encodedImage._encodedWidth = _raw->w;
  _encodedCompleteCallback->Encoded(_encodedImage, &codecSpecific,
      &frag_info);
  return WEBRTC_VIDEO_CODEC_OK;
}
#endif

WebRtc_Word32
VP8Encoder::SetPacketLoss(WebRtc_UWord32 packetLoss)
{
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Encoder::RegisterEncodeCompleteCallback(EncodedImageCallback* callback)
{
    _encodedCompleteCallback = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

VP8Decoder::VP8Decoder():
    _decodeCompleteCallback(NULL),
    _inited(false),
    _feedbackModeOn(false),
    _decoder(NULL),
    _inst(NULL),
    _numCores(1),
    _lastKeyFrame(),
    _imageFormat(VPX_IMG_FMT_NONE),
    _refFrame(NULL),
    _propagationCnt(-1)
{
}

VP8Decoder::~VP8Decoder()
{
    _inited = true; // in order to do the actual release
    Release();
}

WebRtc_Word32
VP8Decoder::Reset()
{
    if (!_inited)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (_inst != NULL)
    {
        VideoCodec inst;
        inst = *_inst;
        InitDecode(&inst, _numCores);
    }
    else
    {
        InitDecode(NULL, _numCores);
    }
    _propagationCnt = -1;
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Decoder::InitDecode(const VideoCodec* inst,
                       WebRtc_Word32 numberOfCores)
{
    vp8_postproc_cfg_t  ppcfg;
    WebRtc_Word32 retVal = Release();
    if (retVal < 0 )
    {
        return retVal;
    }
    if (_decoder == NULL)
    {
        _decoder = new vpx_dec_ctx_t;
    }
#ifdef DEV_PIC_LOSS
    if(inst && inst->codecType == kVideoCodecVP8)
    {
        _feedbackModeOn = inst->codecSpecific.VP8.feedbackModeOn;
    }
#endif

    vpx_codec_dec_cfg_t  cfg;
    // Setting number of threads to a constant value (1)
    cfg.threads = 1;
    cfg.h = cfg.w = 0; // set after decode

    vpx_codec_flags_t flags = 0;
#if WEBRTC_LIBVPX_VERSION >= 971
    flags = VPX_CODEC_USE_ERROR_CONCEALMENT;
#ifdef INDEPENDENT_PARTITIONS
    flags |= VPX_CODEC_USE_INPUT_PARTITION;
#endif
#endif

    if (vpx_codec_dec_init(_decoder, vpx_codec_vp8_dx(), NULL, flags))
    {
        return WEBRTC_VIDEO_CODEC_MEMORY;
    }

    // TODO(mikhal): evaluate post-proc settings
    // config post-processing settings for decoder
    ppcfg.post_proc_flag = VP8_DEBLOCK;
    // Strength of deblocking filter. Valid range:[0,16]
    ppcfg.deblocking_level = 5;
    // ppcfg.NoiseLevel     = 1; //Noise intensity. Valid range: [0,7]
    vpx_codec_control(_decoder, VP8_SET_POSTPROC, &ppcfg);

    // Save VideoCodec instance for later; mainly for duplicating the decoder.
    if (inst && inst != _inst)
    {
        if (!_inst)
        {
            _inst = new VideoCodec;
        }
        *_inst = *inst;
    }
    _numCores = numberOfCores;
    _propagationCnt = -1;

    _inited = true;
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Decoder::Decode(const EncodedImage& inputImage,
                   bool missingFrames,
                   const RTPFragmentationHeader* fragmentation,
                   const CodecSpecificInfo* codecSpecificInfo,
                   WebRtc_Word64 /*renderTimeMs*/)
{
    if (!_inited)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (_decodeCompleteCallback == NULL)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (inputImage._completeFrame == false)
    {
        // future improvement
        // we can't decode this frame
        if (_feedbackModeOn)
        {
            return WEBRTC_VIDEO_CODEC_ERR_REQUEST_SLI;
        }
        // otherwise allow for incomplete frames to be decoded.
    }
    if (inputImage._buffer == NULL && inputImage._length > 0)
    {
        // Reset to avoid requesting key frames too often.
        if (_propagationCnt > 0)
            _propagationCnt = 0;
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

#ifdef INDEPENDENT_PARTITIONS
    if (fragmentation == NULL)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
#endif

    // Restrict error propagation
    // Reset on a key frame refresh
    if (inputImage._frameType == kKeyFrame && inputImage._completeFrame)
        _propagationCnt = -1;
    // Start count on first loss
    else if ((!inputImage._completeFrame || missingFrames) &&
              _propagationCnt == -1)
        _propagationCnt = 0;
    if (_propagationCnt >= 0)
        _propagationCnt++;

    vpx_dec_iter_t iter = NULL;
    vpx_image_t* img;
    WebRtc_Word32 ret;

    // check for missing frames
    if (missingFrames)
    {
        // call decoder with zero data length to signal missing frames
        if (vpx_codec_decode(_decoder, NULL, 0, 0, VPX_DL_REALTIME))
        {
            // Reset to avoid requesting key frames too often.
            if (_propagationCnt > 0)
                _propagationCnt = 0;
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        img = vpx_codec_get_frame(_decoder, &iter);
        iter = NULL;
    }

#ifdef INDEPENDENT_PARTITIONS
    if (DecodePartitions(inputImage, fragmentation))
    {
        // Reset to avoid requesting key frames too often.
        if (_propagationCnt > 0)
            _propagationCnt = 0;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
#else
    WebRtc_UWord8* buffer = inputImage._buffer;
    if (inputImage._length == 0)
    {
        buffer = NULL; // Triggers full frame concealment
    }
    if (vpx_codec_decode(_decoder,
                         buffer,
                         inputImage._length,
                         0,
                         VPX_DL_REALTIME))
    {
        // Reset to avoid requesting key frames too often.
        if (_propagationCnt > 0)
            _propagationCnt = 0;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
#endif

    // Store encoded frame if key frame. (Used in Copy method.)
    if (inputImage._frameType == kKeyFrame)
    {
        // Reduce size due to PictureID that we won't copy.
        const WebRtc_UWord32 bytesToCopy = inputImage._length;
        if (_lastKeyFrame._size < bytesToCopy)
        {
            delete [] _lastKeyFrame._buffer;
            _lastKeyFrame._buffer = NULL;
            _lastKeyFrame._size = 0;
        }

        WebRtc_UWord8* tempBuffer = _lastKeyFrame._buffer; // Save buffer ptr.
        WebRtc_UWord32 tempSize = _lastKeyFrame._size; // Save size.
        _lastKeyFrame = inputImage; // Shallow copy.
        _lastKeyFrame._buffer = tempBuffer; // Restore buffer ptr.
        _lastKeyFrame._size = tempSize; // Restore buffer size.
        if (!_lastKeyFrame._buffer)
        {
            // Allocate memory.
            _lastKeyFrame._size = bytesToCopy;
            _lastKeyFrame._buffer = new WebRtc_UWord8[_lastKeyFrame._size];
        }
        // Copy encoded frame.
        memcpy(_lastKeyFrame._buffer, inputImage._buffer, bytesToCopy);
        _lastKeyFrame._length = bytesToCopy;
    }

    int lastRefUpdates = 0;
#ifdef DEV_PIC_LOSS
    if (vpx_codec_control(_decoder, VP8D_GET_LAST_REF_UPDATES, &lastRefUpdates))
    {
        // Reset to avoid requesting key frames too often.
        if (_propagationCnt > 0)
            _propagationCnt = 0;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    int corrupted = 0;
    if (vpx_codec_control(_decoder, VP8D_GET_FRAME_CORRUPTED, &corrupted))
    {
        // Reset to avoid requesting key frames too often.
        if (_propagationCnt > 0)
            _propagationCnt = 0;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
#endif

    img = vpx_codec_get_frame(_decoder, &iter);
    ret = ReturnFrame(img, inputImage._timeStamp);
    if (ret != 0)
    {
        // Reset to avoid requesting key frames too often.
        if (ret < 0 && _propagationCnt > 0)
            _propagationCnt = 0;
        return ret;
    }

    // we need to communicate that we should send a RPSI with a specific picture ID

    // TODO(pw): how do we know it's a golden or alt reference frame? libvpx will
    // provide an API for now I added it temporarily
    WebRtc_Word16 pictureId = -1;
    if (codecSpecificInfo) {
        pictureId = codecSpecificInfo->codecSpecific.VP8.pictureId;
    }
    if (pictureId > -1)
    {
        if ((lastRefUpdates & VP8_GOLD_FRAME)
                || (lastRefUpdates & VP8_ALTR_FRAME))
        {
            if (!missingFrames && (inputImage._completeFrame == true))
            //if (!corrupted) // TODO(pw): Can we engage this line instead of
            // the above?
            {
                _decodeCompleteCallback->ReceivedDecodedReferenceFrame(
                        pictureId);
            }
        }
        _decodeCompleteCallback->ReceivedDecodedFrame(pictureId);
    }

#ifdef DEV_PIC_LOSS
    if (corrupted)
    {
        // we can decode but with artifacts
        return WEBRTC_VIDEO_CODEC_REQUEST_SLI;
    }
#endif

    // Check Vs. threshold
    if (_propagationCnt > kVp8ErrorPropagationTh)
    {
        // Reset to avoid requesting key frames too often.
        _propagationCnt = 0;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Decoder::DecodePartitions(const EncodedImage& input_image,
                             const RTPFragmentationHeader* fragmentation) {
  for (int i = 0; i < fragmentation->fragmentationVectorSize; ++i) {
    const WebRtc_UWord8* partition = input_image._buffer +
        fragmentation->fragmentationOffset[i];
    const WebRtc_UWord32 partition_length =
        fragmentation->fragmentationLength[i];
    if (vpx_codec_decode(_decoder,
                         partition,
                         partition_length,
                         0,
                         VPX_DL_REALTIME)) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }

  // Signal end of frame data. If there was no frame data this will trigger
  // a full frame concealment.
  if (vpx_codec_decode(_decoder, NULL, 0, 0, VPX_DL_REALTIME))
    return WEBRTC_VIDEO_CODEC_ERROR;
  return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Decoder::ReturnFrame(const vpx_image_t* img, WebRtc_UWord32 timeStamp)
{
    if (img == NULL)
    {
        // Decoder OK and NULL image => No show frame
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }

    // Allocate memory for decoded image
    WebRtc_UWord32 requiredSize = (3 * img->d_h * img->d_w) >> 1;
    if (requiredSize > _decodedImage._size)
    {
        delete [] _decodedImage._buffer;
        _decodedImage._buffer = NULL;
    }
    if (_decodedImage._buffer == NULL)
    {
        _decodedImage._size = requiredSize;
        _decodedImage._buffer = new WebRtc_UWord8[_decodedImage._size];
    }

    WebRtc_UWord8* buf;
    WebRtc_UWord32 locCnt = 0;
    WebRtc_UWord32 plane, y;

    for (plane = 0; plane < 3; plane++)
    {
        buf = img->planes[plane];
        WebRtc_UWord32 shiftFactor = plane ? 1 : 0;
        for(y = 0; y < img->d_h >> shiftFactor; y++)
        {
            memcpy(&_decodedImage._buffer[locCnt], buf, img->d_w >> shiftFactor);
            locCnt += img->d_w >> shiftFactor;
            buf += img->stride[plane];
        }
    }

    // Set image parameters
    _decodedImage._height = img->d_h;
    _decodedImage._width = img->d_w;
    _decodedImage._length = (3 * img->d_h * img->d_w) >> 1;
    _decodedImage._timeStamp = timeStamp;
    WebRtc_Word32 ret = _decodeCompleteCallback->Decoded(_decodedImage);
    if (ret != 0)
        return ret;

    // Remember image format for later
    _imageFormat = img->fmt;
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Decoder::RegisterDecodeCompleteCallback(DecodedImageCallback* callback)
{
    _decodeCompleteCallback = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32
VP8Decoder::Release()
{
    if (_decodedImage._buffer != NULL)
    {
        delete [] _decodedImage._buffer;
        _decodedImage._buffer = NULL;
    }
    if (_lastKeyFrame._buffer != NULL)
    {
        delete [] _lastKeyFrame._buffer;
        _lastKeyFrame._buffer = NULL;
    }
    if (_decoder != NULL)
    {
        if(vpx_codec_destroy(_decoder))
        {
            return WEBRTC_VIDEO_CODEC_MEMORY;
        }
        delete _decoder;
        _decoder = NULL;
    }
    if (_inst != NULL)
    {
        delete _inst;
        _inst = NULL;
    }
    if (_refFrame != NULL)
    {
        vpx_img_free(&_refFrame->img);
        delete _refFrame;
        _refFrame = NULL;
    }

    _inited = false;
    return WEBRTC_VIDEO_CODEC_OK;
}

VideoDecoder*
VP8Decoder::Copy()
{
    // Sanity checks.
    if (!_inited)
    {
        // Not initialized.
        assert(false);
        return NULL;
    }
    if (_decodedImage._buffer == NULL)
    {
        // Nothing has been decoded before; cannot clone.
        return NULL;
    }
    if (_lastKeyFrame._buffer == NULL)
    {
        // Cannot clone if we have no key frame to start with.
        return NULL;
    }

    // Create a new VideoDecoder object
    VP8Decoder *copyTo = new VP8Decoder;

    // Initialize the new decoder
    if (copyTo->InitDecode(_inst, _numCores) != WEBRTC_VIDEO_CODEC_OK)
    {
        delete copyTo;
        return NULL;
    }

    // Inject last key frame into new decoder.
    if (vpx_codec_decode(copyTo->_decoder, _lastKeyFrame._buffer,
        _lastKeyFrame._length, NULL, VPX_DL_REALTIME))
    {
        delete copyTo;
        return NULL;
    }

    // Allocate memory for reference image copy
    assert(_decodedImage._width > 0);
    assert(_decodedImage._height > 0);
    assert(_imageFormat > VPX_IMG_FMT_NONE);
    // Check if frame format has changed.
    if (_refFrame &&
        (_decodedImage._width != _refFrame->img.d_w ||
            _decodedImage._height != _refFrame->img.d_h ||
            _imageFormat != _refFrame->img.fmt))
    {
        vpx_img_free(&_refFrame->img);
        delete _refFrame;
        _refFrame = NULL;
    }


    if (!_refFrame)
    {
        _refFrame = new vpx_ref_frame_t;

        if(!vpx_img_alloc(&_refFrame->img,
            static_cast<vpx_img_fmt_t>(_imageFormat),
            _decodedImage._width, _decodedImage._height, 1))
        {
            assert(false);
            delete copyTo;
            return NULL;
        }
    }

    const vpx_ref_frame_type_t typeVec[] = { VP8_LAST_FRAME, VP8_GOLD_FRAME,
                                             VP8_ALTR_FRAME };
    for (WebRtc_UWord32 ix = 0;
         ix < sizeof(typeVec) / sizeof(vpx_ref_frame_type_t); ++ix)
    {
        _refFrame->frame_type = typeVec[ix];
        if (CopyReference(copyTo) < 0)
        {
            delete copyTo;
            return NULL;
        }
    }

    // Copy all member variables (that are not set in initialization).
    copyTo->_feedbackModeOn = _feedbackModeOn;
    copyTo->_imageFormat = _imageFormat;
    copyTo->_lastKeyFrame = _lastKeyFrame; // Shallow copy.
    // Allocate memory. (Discard copied _buffer pointer.)
    copyTo->_lastKeyFrame._buffer = new WebRtc_UWord8[_lastKeyFrame._size];
    memcpy(copyTo->_lastKeyFrame._buffer, _lastKeyFrame._buffer,
           _lastKeyFrame._length);

    // Initialize _decodedImage.
    copyTo->_decodedImage = _decodedImage;  // Shallow copy
    copyTo->_decodedImage._buffer = NULL;
    if (_decodedImage._size)
    {
        copyTo->_decodedImage._buffer = new WebRtc_UWord8[_decodedImage._size];
    }

    return static_cast<VideoDecoder*>(copyTo);
}

int VP8Decoder::CopyReference(VP8Decoder* copyTo)
{
    // The type of frame to copy should be set in _refFrame->frame_type
    // before the call to this function.
    if (vpx_codec_control(_decoder, VP8_COPY_REFERENCE, _refFrame)
        != VPX_CODEC_OK)
    {
        return -1;
    }
    if (vpx_codec_control(copyTo->_decoder, VP8_SET_REFERENCE, _refFrame)
        != VPX_CODEC_OK)
    {
        return -1;
    }
    return 0;
}


} // namespace webrtc
