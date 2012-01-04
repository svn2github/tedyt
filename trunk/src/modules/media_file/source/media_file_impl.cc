/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#include "critical_section_wrapper.h"
#include "file_wrapper.h"
#include "media_file_impl.h"
#include "tick_util.h"
#include "trace.h"

#if (defined(WIN32) || defined(WINCE))
    #define STR_CASE_CMP _stricmp
    #define STR_NCASE_CMP _strnicmp
#else
    #define STR_CASE_CMP strcasecmp
    #define STR_NCASE_CMP strncasecmp
#endif

namespace webrtc {
MediaFile* MediaFile::CreateMediaFile(const WebRtc_Word32 id)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, id, "CreateMediaFile()");
    return new MediaFileImpl(id);
}

void MediaFile::DestroyMediaFile(MediaFile* module)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, -1, "DestroyMediaFile()");
    delete static_cast<MediaFileImpl*>(module);
}

MediaFileImpl::MediaFileImpl(const WebRtc_Word32 id)
    : _id(id),
      _crit(CriticalSectionWrapper::CreateCriticalSection()),
      _callbackCrit(CriticalSectionWrapper::CreateCriticalSection()),
      _ptrFileUtilityObj(NULL),
      codec_info_(),
      _ptrInStream(NULL),
      _ptrOutStream(NULL),
      _fileFormat((FileFormats)-1),
      _recordDurationMs(0),
      _playoutPositionMs(0),
      _notificationMs(0),
      _playingActive(false),
      _recordingActive(false),
      _isStereo(false),
      _openFile(false),
      _fileName(),
      _ptrCallback(NULL)
{
    WEBRTC_TRACE(kTraceMemory, kTraceFile, id, "Created");

    codec_info_.plname[0] = '\0';
    _fileName[0] = '\0';
}


MediaFileImpl::~MediaFileImpl()
{
    WEBRTC_TRACE(kTraceMemory, kTraceFile, _id, "~MediaFileImpl()");
    {
        CriticalSectionScoped lock(_crit);

        if(_playingActive)
        {
            StopPlaying();
        }

        if(_recordingActive)
        {
            StopRecording();
        }

        delete _ptrFileUtilityObj;

        if(_openFile)
        {
            delete _ptrInStream;
            _ptrInStream = NULL;
            delete _ptrOutStream;
            _ptrOutStream = NULL;
        }
    }

    delete _crit;
    delete _callbackCrit;
}

WebRtc_Word32 MediaFileImpl::ChangeUniqueId(const WebRtc_Word32 id)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id, "ChangeUniqueId(new id:%d)",
                 id);
    _id = id;
    return 0;
}

WebRtc_Word32 MediaFileImpl::TimeUntilNextProcess()
{
    WEBRTC_TRACE(
        kTraceWarning,
        kTraceFile,
        _id,
        "TimeUntilNextProcess: This method is not used by MediaFile class.");
    return -1;
}

WebRtc_Word32 MediaFileImpl::Process()
{
    WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                 "Process: This method is not used by MediaFile class.");
    return -1;
}

WebRtc_Word32 MediaFileImpl::PlayoutAVIVideoData(
    WebRtc_Word8* buffer,
    WebRtc_UWord32& dataLengthInBytes)
{
    return PlayoutData( buffer, dataLengthInBytes, true);
}

WebRtc_Word32 MediaFileImpl::PlayoutAudioData(WebRtc_Word8* buffer,
                                WebRtc_UWord32& dataLengthInBytes)
{
    return PlayoutData( buffer, dataLengthInBytes, false);
}

WebRtc_Word32 MediaFileImpl::PlayoutData(WebRtc_Word8* buffer,
                                         WebRtc_UWord32& dataLengthInBytes,
                                         bool video)
{
    WEBRTC_TRACE(kTraceStream, kTraceFile, _id,
               "MediaFileImpl::PlayoutData(buffer= 0x%x, bufLen= %ld)",
                 buffer, dataLengthInBytes);

    const WebRtc_UWord32 bufferLengthInBytes = dataLengthInBytes;
    dataLengthInBytes = 0;

    if(buffer == NULL || bufferLengthInBytes == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "Buffer pointer or length is NULL!");
        return -1;
    }

    WebRtc_Word32 bytesRead = 0;
    {
        CriticalSectionScoped lock(_crit);

        if(!_playingActive)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                         "Not currently playing!");
            return -1;
        }

        if(!_ptrFileUtilityObj)
        {
            WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                         "Playing, but no FileUtility object!");
            StopPlaying();
            return -1;
        }

        switch(_fileFormat)
        {
            case kFileFormatPcm32kHzFile:
            case kFileFormatPcm16kHzFile:
            case kFileFormatPcm8kHzFile:
                bytesRead = _ptrFileUtilityObj->ReadPCMData(
                    *_ptrInStream,
                    buffer,
                    bufferLengthInBytes);
                break;
            case kFileFormatCompressedFile:
                bytesRead = _ptrFileUtilityObj->ReadCompressedData(
                    *_ptrInStream,
                    buffer,
                    bufferLengthInBytes);
                break;
            case kFileFormatWavFile:
                bytesRead = _ptrFileUtilityObj->ReadWavDataAsMono(
                    *_ptrInStream,
                    buffer,
                    bufferLengthInBytes);
                break;
            case kFileFormatPreencodedFile:
                bytesRead = _ptrFileUtilityObj->ReadPreEncodedData(
                    *_ptrInStream,
                    buffer,
                    bufferLengthInBytes);
                if(bytesRead > 0)
                {
                    dataLengthInBytes = bytesRead;
                    return 0;
                }
                break;
#ifdef WEBRTC_MODULE_UTILITY_VIDEO
            case kFileFormatAviFile:
            {
                if(video)
                {
                    bytesRead = _ptrFileUtilityObj->ReadAviVideoData(
                        buffer,
                        bufferLengthInBytes);
                }
                else
                {
                    bytesRead = _ptrFileUtilityObj->ReadAviAudioData(
                        buffer,
                        bufferLengthInBytes);
                }
                break;
            }
#endif
            default:
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Playing file, but file format invalid!");
                assert(false);
                break;
        }

        if( bytesRead > 0)
        {
            dataLengthInBytes =(WebRtc_UWord32) bytesRead;
        }
    }
    HandlePlayCallbacks(bytesRead);
    return 0;
}

void MediaFileImpl::HandlePlayCallbacks(WebRtc_Word32 bytesRead)
{
    bool playEnded = false;
    WebRtc_UWord32 callbackNotifyMs = 0;

    if(bytesRead > 0)
    {
        // Check if it's time for PlayNotification(..).
        _playoutPositionMs = _ptrFileUtilityObj->PlayoutPositionMs();
        if(_notificationMs)
        {
            if(_playoutPositionMs >= _notificationMs)
            {
                _notificationMs = 0;
                callbackNotifyMs = _playoutPositionMs;
            }
        }
    }
    else
    {
        // If no bytes were read assume end of file.
        StopPlaying();
        playEnded = true;
    }

    // Only _callbackCrit may and should be taken when making callbacks.
    CriticalSectionScoped lock(_callbackCrit);
    if(_ptrCallback)
    {
        if(callbackNotifyMs)
        {
            _ptrCallback->PlayNotification(_id, callbackNotifyMs);
        }
        if(playEnded)
        {
            _ptrCallback->PlayFileEnded(_id);
        }
    }
}

WebRtc_Word32 MediaFileImpl::PlayoutStereoData(
    WebRtc_Word8* bufferLeft,
    WebRtc_Word8* bufferRight,
    WebRtc_UWord32& dataLengthInBytes)
{
    WEBRTC_TRACE(kTraceStream, kTraceFile, _id,
                 "MediaFileImpl::PlayoutStereoData(Left = 0x%x, Right = 0x%x,\
 Len= %ld)",
                 bufferLeft,
                 bufferRight,
                 dataLengthInBytes);

    const WebRtc_UWord32 bufferLengthInBytes = dataLengthInBytes;
    dataLengthInBytes = 0;

    if(bufferLeft == NULL || bufferRight == NULL || bufferLengthInBytes == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "A buffer pointer or the length is NULL!");
        return -1;
    }

    bool playEnded = false;
    WebRtc_UWord32 callbackNotifyMs = 0;
    {
        CriticalSectionScoped lock(_crit);

        if(!_playingActive || !_isStereo)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                         "Not currently playing stereo!");
            return -1;
        }

        if(!_ptrFileUtilityObj)
        {
            WEBRTC_TRACE(
                kTraceError,
                kTraceFile,
                _id,
                "Playing stereo, but the FileUtility objects is NULL!");
            StopPlaying();
            return -1;
        }

        // Stereo playout only supported for WAV files.
        WebRtc_Word32 bytesRead = 0;
        switch(_fileFormat)
        {
            case kFileFormatWavFile:
                    bytesRead = _ptrFileUtilityObj->ReadWavDataAsStereo(
                        *_ptrInStream,
                        bufferLeft,
                        bufferRight,
                        bufferLengthInBytes);
                    break;
            default:
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Trying to read non-WAV as stereo audio\
 (not supported)");
                break;
        }

        if(bytesRead > 0)
        {
            dataLengthInBytes = bytesRead;

            // Check if it's time for PlayNotification(..).
            _playoutPositionMs = _ptrFileUtilityObj->PlayoutPositionMs();
            if(_notificationMs)
            {
                if(_playoutPositionMs >= _notificationMs)
                {
                    _notificationMs = 0;
                    callbackNotifyMs = _playoutPositionMs;
                }
            }
        }
        else
        {
            // If no bytes were read assume end of file.
            StopPlaying();
            playEnded = true;
        }
    }

    CriticalSectionScoped lock(_callbackCrit);
    if(_ptrCallback)
    {
        if(callbackNotifyMs)
        {
            _ptrCallback->PlayNotification(_id, callbackNotifyMs);
        }
        if(playEnded)
        {
            _ptrCallback->PlayFileEnded(_id);
        }
    }
    return 0;
}

WebRtc_Word32 MediaFileImpl::StartPlayingAudioFile(
    const WebRtc_Word8* fileName,
    const WebRtc_UWord32 notificationTimeMs,
    const bool loop,
    const FileFormats format,
    const CodecInst* codecInst,
    const WebRtc_UWord32 startPointMs,
    const WebRtc_UWord32 stopPointMs)
{
    const bool videoOnly = false;
    return StartPlayingFile(fileName, notificationTimeMs, loop, videoOnly,
                            format, codecInst, startPointMs, stopPointMs);
}


WebRtc_Word32 MediaFileImpl::StartPlayingVideoFile(const WebRtc_Word8* fileName,
                                                   const bool loop,
                                                   bool videoOnly,
                                                   const FileFormats format)
{

    const WebRtc_UWord32 notificationTimeMs = 0;
    const WebRtc_UWord32 startPointMs       = 0;
    const WebRtc_UWord32 stopPointMs        = 0;
    return StartPlayingFile(fileName, notificationTimeMs, loop, videoOnly,
                            format, 0, startPointMs, stopPointMs);
}

WebRtc_Word32 MediaFileImpl::StartPlayingFile(
    const WebRtc_Word8* fileName,
    const WebRtc_UWord32 notificationTimeMs,
    const bool loop,
    bool videoOnly,
    const FileFormats format,
    const CodecInst* codecInst,
    const WebRtc_UWord32 startPointMs,
    const WebRtc_UWord32 stopPointMs)
{
    WEBRTC_TRACE(
        kTraceModuleCall,
        kTraceFile,
        _id,
        "MediaFileImpl::StartPlayingFile: fileName= %s, notify= %d, loop= %d,\
 format= %d, codecInst=%s, start= %d, stop= %d",
        (fileName == NULL) ? "NULL" : fileName,
        notificationTimeMs,
        loop,
        format,(codecInst == NULL) ? "NULL" : codecInst->plname,
        startPointMs,
        stopPointMs);

    if(!ValidFileName(fileName))
    {
        return -1;
    }
    if(!ValidFileFormat(format,codecInst))
    {
        return -1;
    }
    if(!ValidFilePositions(startPointMs,stopPointMs))
    {
        return -1;
    }

    // Check that the file will play longer than notificationTimeMs ms.
    if((startPointMs && stopPointMs && !loop) &&
       (notificationTimeMs > (stopPointMs - startPointMs)))
    {
        WEBRTC_TRACE(
            kTraceError,
            kTraceFile,
            _id,
            "specified notification time is longer than amount of ms that will\
 be played");
        return -1;
    }

    FileWrapper* inputStream = FileWrapper::Create();
    if(inputStream == NULL)
    {
       WEBRTC_TRACE(kTraceMemory, kTraceFile, _id,
                    "Failed to allocate input stream for file %s", fileName);
        return -1;
    }

    // TODO (hellner): make all formats support reading from stream.
    bool useStream = (format != kFileFormatAviFile);
    if( useStream)
    {
        if(inputStream->OpenFile(fileName, true, loop) != 0)
        {
            delete inputStream;
            WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                         "Could not open input file %s", fileName);
            return -1;
        }
    }

    if(StartPlayingStream(*inputStream, fileName, loop, notificationTimeMs,
                          format, codecInst, startPointMs, stopPointMs,
                          videoOnly) == -1)
    {
        if( useStream)
        {
            inputStream->CloseFile();
        }
        delete inputStream;
        return -1;
    }

    CriticalSectionScoped lock(_crit);
    _openFile = true;
    strncpy(_fileName, fileName, sizeof(_fileName));
    _fileName[sizeof(_fileName) - 1] = '\0';
    return 0;
}

WebRtc_Word32 MediaFileImpl::StartPlayingAudioStream(
    InStream& stream,
    const WebRtc_UWord32 notificationTimeMs,
    const FileFormats format,
    const CodecInst* codecInst,
    const WebRtc_UWord32 startPointMs,
    const WebRtc_UWord32 stopPointMs)
{
    return StartPlayingStream(stream, 0, false, notificationTimeMs, format,
                              codecInst, startPointMs, stopPointMs);
}

WebRtc_Word32 MediaFileImpl::StartPlayingStream(
    InStream& stream,
    const WebRtc_Word8* filename,
    bool loop,
    const WebRtc_UWord32 notificationTimeMs,
    const FileFormats format,
    const CodecInst*  codecInst,
    const WebRtc_UWord32 startPointMs,
    const WebRtc_UWord32 stopPointMs,
    bool videoOnly)
{
    WEBRTC_TRACE(
        kTraceModuleCall,
        kTraceFile,
        _id,
        "MediaFileImpl::StartPlayingStream(stream=0x%x, notify=%ld, format=%d,\
 codecInst=%s, start=%ld, stop=%ld",
        &stream,
        notificationTimeMs,
        format,
        (codecInst == NULL) ? "NULL" : codecInst->plname,
        startPointMs,
        stopPointMs);

    if(!ValidFileFormat(format,codecInst))
    {
        return -1;
    }

    if(!ValidFilePositions(startPointMs,stopPointMs))
    {
        return -1;
    }

    CriticalSectionScoped lock(_crit);
    if(_playingActive || _recordingActive)
    {
        WEBRTC_TRACE(
            kTraceError,
            kTraceFile,
            _id,
            "StartPlaying called, but already playing or recording file %s",
            (_fileName == NULL) ? "NULL" : _fileName);
        return -1;
    }

    if(_ptrFileUtilityObj != NULL)
    {
        WEBRTC_TRACE(kTraceError,
                     kTraceFile,
                     _id,
                     "StartPlaying called, but FileUtilityObj already exists!");
        StopPlaying();
        return -1;
    }

    _ptrFileUtilityObj = new ModuleFileUtility(_id);
    if(_ptrFileUtilityObj == NULL)
    {
        WEBRTC_TRACE(kTraceMemory, kTraceFile, _id,
                     "Failed to create FileUtilityObj!");
        return -1;
    }

    switch(format)
    {
        case kFileFormatWavFile:
        {
            if(_ptrFileUtilityObj->InitWavReading(stream, startPointMs,
                                                  stopPointMs) == -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Not a valid WAV file!");
                StopPlaying();
                return -1;
            }
            _fileFormat = kFileFormatWavFile;
            break;
        }
        case kFileFormatCompressedFile:
        {
            if(_ptrFileUtilityObj->InitCompressedReading(stream, startPointMs,
                                                         stopPointMs) == -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Not a valid Compressed file!");
                StopPlaying();
                return -1;
            }
            _fileFormat = kFileFormatCompressedFile;
            break;
        }
        case kFileFormatPcm8kHzFile:
        case kFileFormatPcm16kHzFile:
        case kFileFormatPcm32kHzFile:
        {
            if(!ValidFrequency(codecInst->plfreq) ||
               _ptrFileUtilityObj->InitPCMReading(stream, startPointMs,
                                                  stopPointMs,
                                                  codecInst->plfreq) == -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Not a valid raw 8 or 16 KHz PCM file!");
                StopPlaying();
                return -1;
            }

            _fileFormat = format;
            break;
        }
        case kFileFormatPreencodedFile:
        {
            if(_ptrFileUtilityObj->InitPreEncodedReading(stream, *codecInst) ==
               -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Not a valid PreEncoded file!");
                StopPlaying();
                return -1;
            }

            _fileFormat = kFileFormatPreencodedFile;
            break;
        }
#ifdef WEBRTC_MODULE_UTILITY_VIDEO
        case kFileFormatAviFile:
        {
            if(_ptrFileUtilityObj->InitAviReading( filename, videoOnly, loop))
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Not a valid AVI file!");
                StopPlaying();

                return -1;
            }

            _ptrFileUtilityObj->codec_info(codec_info_);

            _fileFormat = kFileFormatAviFile;
            break;
        }
#endif
        default:
        {
            WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                         "Invalid file format specified!");
            StopPlaying();
            return -1;
        }
    }
    if(_ptrFileUtilityObj->codec_info(codec_info_) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "Failed to retrieve codec info!");
        StopPlaying();
        return -1;
    }

    _isStereo = (codec_info_.channels == 2);
    if(_isStereo && (_fileFormat != kFileFormatWavFile))
    {
        WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                     "Stereo is only allowed for WAV files");
        StopPlaying();
        return -1;
    }
    _playingActive = true;
    _playoutPositionMs = _ptrFileUtilityObj->PlayoutPositionMs();
    _ptrInStream = &stream;
    _notificationMs = notificationTimeMs;

    return 0;
}

WebRtc_Word32 MediaFileImpl::StopPlaying()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id,
                 "MediaFileImpl::StopPlaying()");

    CriticalSectionScoped lock(_crit);
    _isStereo = false;
    if(_ptrFileUtilityObj)
    {
        delete _ptrFileUtilityObj;
        _ptrFileUtilityObj = NULL;
    }
    if(_ptrInStream)
    {
        // If MediaFileImpl opened the InStream it must be reclaimed here.
        if(_openFile)
        {
            delete _ptrInStream;
            _openFile = false;
        }
        _ptrInStream = NULL;
    }

    codec_info_.pltype = 0;
    codec_info_.plname[0] = '\0';

    if(!_playingActive)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                     "playing is not active!");
        return -1;
    }

    _playingActive = false;
    return 0;
}

bool MediaFileImpl::IsPlaying()
{
    WEBRTC_TRACE(kTraceStream, kTraceFile, _id, "MediaFileImpl::IsPlaying()");
    CriticalSectionScoped lock(_crit);
    return _playingActive;
}

WebRtc_Word32 MediaFileImpl::IncomingAudioData(
    const WebRtc_Word8*  buffer,
    const WebRtc_UWord32 bufferLengthInBytes)
{
    return IncomingAudioVideoData( buffer, bufferLengthInBytes, false);
}

WebRtc_Word32 MediaFileImpl::IncomingAVIVideoData(
    const WebRtc_Word8*  buffer,
    const WebRtc_UWord32 bufferLengthInBytes)
{
    return IncomingAudioVideoData( buffer, bufferLengthInBytes, true);
}

WebRtc_Word32 MediaFileImpl::IncomingAudioVideoData(
    const WebRtc_Word8*  buffer,
    const WebRtc_UWord32 bufferLengthInBytes,
    const bool video)
{
    WEBRTC_TRACE(kTraceStream, kTraceFile, _id,
                 "MediaFile::IncomingData(buffer= 0x%x, bufLen= %hd",
                 buffer, bufferLengthInBytes);

    if(buffer == NULL || bufferLengthInBytes == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "Buffer pointer or length is NULL!");
        return -1;
    }

    bool recordingEnded = false;
    WebRtc_UWord32 callbackNotifyMs = 0;
    {
        CriticalSectionScoped lock(_crit);

        if(!_recordingActive)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                         "Not currently recording!");
            return -1;
        }
        if(_ptrOutStream == NULL)
        {
            WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                         "Recording is active, but output stream is NULL!");
            assert(false);
            return -1;
        }

        WebRtc_Word32 bytesWritten = 0;
        WebRtc_UWord32 samplesWritten = codec_info_.pacsize;
        if(_ptrFileUtilityObj)
        {
            switch(_fileFormat)
            {
                case kFileFormatPcm8kHzFile:
                case kFileFormatPcm16kHzFile:
                case kFileFormatPcm32kHzFile:
                    bytesWritten = _ptrFileUtilityObj->WritePCMData(
                        *_ptrOutStream,
                        buffer,
                        bufferLengthInBytes);

                    // Sample size is 2 bytes.
                    if(bytesWritten > 0)
                    {
                        samplesWritten = bytesWritten/sizeof(WebRtc_Word16);
                    }
                    break;
                case kFileFormatCompressedFile:
                    bytesWritten = _ptrFileUtilityObj->WriteCompressedData(
                        *_ptrOutStream, buffer, bufferLengthInBytes);
                    break;
                case kFileFormatWavFile:
                    bytesWritten = _ptrFileUtilityObj->WriteWavData(
                        *_ptrOutStream,
                        buffer,
                        bufferLengthInBytes);
                    if(bytesWritten > 0 && STR_NCASE_CMP(codec_info_.plname,
                                                         "L16", 4) == 0)
                    {
                        // Sample size is 2 bytes.
                        samplesWritten = bytesWritten/sizeof(WebRtc_Word16);
                    }
                    break;
                case kFileFormatPreencodedFile:
                    bytesWritten = _ptrFileUtilityObj->WritePreEncodedData(
                        *_ptrOutStream, buffer, bufferLengthInBytes);
                    break;
#ifdef WEBRTC_MODULE_UTILITY_VIDEO
                case kFileFormatAviFile:
                    if(video)
                    {
                        bytesWritten = _ptrFileUtilityObj->WriteAviVideoData(
                            buffer, bufferLengthInBytes);
                    }else
                    {
                        bytesWritten = _ptrFileUtilityObj->WriteAviAudioData(
                            buffer, bufferLengthInBytes);
                    }
                    break;
#endif
                default:
                    WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                                 "recording active, but file format invalid!");
                    break;
            }
        } else {
            // TODO (hellner): quick look at the code makes me think that this
            //                 code is never executed. Remove?
            if(_ptrOutStream)
            {
                if(_ptrOutStream->Write(buffer, bufferLengthInBytes))
                {
                    bytesWritten = bufferLengthInBytes;
                }
            }
        }

        if(!video)
        {
            _recordDurationMs += samplesWritten / (codec_info_.plfreq / 1000);
        }

        // Check if it's time for RecordNotification(..).
        if(_notificationMs)
        {
            if(_recordDurationMs  >= _notificationMs)
            {
                _notificationMs = 0;
                callbackNotifyMs = _recordDurationMs;
            }
        }
        if(bytesWritten < (WebRtc_Word32)bufferLengthInBytes)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                         "Failed to write all requested bytes!");
            StopRecording();
            recordingEnded = true;
        }
    }

    // Only _callbackCrit may and should be taken when making callbacks.
    CriticalSectionScoped lock(_callbackCrit);
    if(_ptrCallback)
    {
        if(callbackNotifyMs)
        {
            _ptrCallback->RecordNotification(_id, callbackNotifyMs);
        }
        if(recordingEnded)
        {
            _ptrCallback->RecordFileEnded(_id);
            return -1;
        }
    }
    return 0;
}

WebRtc_Word32 MediaFileImpl::StartRecordingAudioFile(
    const WebRtc_Word8* fileName,
    const FileFormats format,
    const CodecInst& codecInst,
    const WebRtc_UWord32 notificationTimeMs,
    const WebRtc_UWord32 maxSizeBytes)
{
    VideoCodec dummyCodecInst;
    return StartRecordingFile(fileName, format, codecInst, dummyCodecInst,
                              notificationTimeMs, maxSizeBytes);
}


WebRtc_Word32 MediaFileImpl::StartRecordingVideoFile(
    const WebRtc_Word8* fileName,
    const FileFormats format,
    const CodecInst& codecInst,
    const VideoCodec& videoCodecInst,
    bool videoOnly)
{
    const WebRtc_UWord32 notificationTimeMs = 0;
    const WebRtc_UWord32 maxSizeBytes       = 0;

    return StartRecordingFile(fileName, format, codecInst, videoCodecInst,
                              notificationTimeMs, maxSizeBytes, videoOnly);
}

WebRtc_Word32 MediaFileImpl::StartRecordingFile(
    const WebRtc_Word8* fileName,
    const FileFormats format,
    const CodecInst& codecInst,
    const VideoCodec& videoCodecInst,
    const WebRtc_UWord32 notificationTimeMs,
    const WebRtc_UWord32 maxSizeBytes,
    bool videoOnly)
{
    WEBRTC_TRACE(kTraceModuleCall,
                 kTraceFile,
                 _id,
                 "MediaFileImpl::StartRecordingFile(fileName= %s, format= %d,\
 codecInst= %s, notificationMs= %d, maxSize= %d",
                 (fileName == NULL) ? "NULL" : fileName,
                 format,
                 (codecInst.plname[0] == '\0') ? "NULL" : codecInst.plname,
                 notificationTimeMs,
                 maxSizeBytes);

    if(!ValidFileName(fileName))
    {
        return -1;
    }
    if(!ValidFileFormat(format,&codecInst))
    {
        return -1;
    }

    FileWrapper* outputStream = FileWrapper::Create();
    if(outputStream == NULL)
    {
        WEBRTC_TRACE(kTraceMemory, kTraceFile, _id,
                     "Failed to allocate memory for output stream");
        return -1;
    }

    // TODO (hellner): make all formats support writing to stream.
    const bool useStream = ( format != kFileFormatAviFile);
    if( useStream)
    {
        if(outputStream->OpenFile(fileName, false) != 0)
        {
            delete outputStream;
            WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                         "Could not open output file '%s' for writing!",
                         fileName);
            return -1;
        }
    }
    if(maxSizeBytes)
    {
        outputStream->SetMaxFileSize(maxSizeBytes);
    }

    if(StartRecordingStream(*outputStream, fileName, format, codecInst,
                            videoCodecInst, notificationTimeMs,
                            videoOnly) == -1)
    {
        if( useStream)
        {
            outputStream->CloseFile();
        }
        delete outputStream;
        return -1;
    }

    CriticalSectionScoped lock(_crit);
    _openFile = true;
    strncpy(_fileName, fileName, sizeof(_fileName));
    _fileName[sizeof(_fileName) - 1] = '\0';
    return 0;
}

WebRtc_Word32 MediaFileImpl::StartRecordingAudioStream(
    OutStream& stream,
    const FileFormats format,
    const CodecInst& codecInst,
    const WebRtc_UWord32 notificationTimeMs)
{
    VideoCodec dummyCodecInst;
    return StartRecordingStream(stream, 0, format, codecInst, dummyCodecInst,
                                notificationTimeMs);
}

WebRtc_Word32 MediaFileImpl::StartRecordingStream(
    OutStream& stream,
    const WebRtc_Word8* fileName,
    const FileFormats format,
    const CodecInst& codecInst,
    const VideoCodec& videoCodecInst,
    const WebRtc_UWord32 notificationTimeMs,
    bool videoOnly)
{
    WEBRTC_TRACE(
        kTraceModuleCall,
        kTraceFile,
        _id,
        "MediaFileImpl::StartRecordingStream: format= %d, codec= %s,\
 notify= %d",
        format,(codecInst.plname[0] == '\0') ? "NULL" : codecInst.plname,
        notificationTimeMs);

    // Check codec info
    if(!ValidFileFormat(format,&codecInst))
    {
        return -1;
    }

    CriticalSectionScoped lock(_crit);
    if(_recordingActive || _playingActive)
    {
        WEBRTC_TRACE(
            kTraceError,
            kTraceFile,
            _id,
            "StartRecording called, but already recording or playing file %s!",
                   _fileName);
        return -1;
    }

    if(_ptrFileUtilityObj != NULL)
    {
        WEBRTC_TRACE(
            kTraceError,
            kTraceFile,
            _id,
            "StartRecording called, but fileUtilityObj already exists!");
        StopRecording();
        return -1;
    }

    _ptrFileUtilityObj = new ModuleFileUtility(_id);
    if(_ptrFileUtilityObj == NULL)
    {
        WEBRTC_TRACE(kTraceMemory, kTraceFile, _id,
                     "Cannot allocate fileUtilityObj!");
        return -1;
    }

    CodecInst tmpAudioCodec;
    memcpy(&tmpAudioCodec, &codecInst, sizeof(CodecInst));
    switch(format)
    {
        case kFileFormatWavFile:
        {
            if(_ptrFileUtilityObj->InitWavWriting(stream, codecInst) == -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Failed to initialize WAV file!");
                delete _ptrFileUtilityObj;
                _ptrFileUtilityObj = NULL;
                return -1;
            }
            _fileFormat = kFileFormatWavFile;
            break;
        }
        case kFileFormatCompressedFile:
        {
            // Write compression codec name at beginning of file
            if(_ptrFileUtilityObj->InitCompressedWriting(stream, codecInst) ==
               -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Failed to initialize Compressed file!");
                delete _ptrFileUtilityObj;
                _ptrFileUtilityObj = NULL;
                return -1;
            }
            _fileFormat = kFileFormatCompressedFile;
            break;
        }
        case kFileFormatPcm8kHzFile:
        case kFileFormatPcm16kHzFile:
        {
            if(!ValidFrequency(codecInst.plfreq) ||
               _ptrFileUtilityObj->InitPCMWriting(stream, codecInst.plfreq) ==
               -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Failed to initialize 8 or 16KHz PCM file!");
                delete _ptrFileUtilityObj;
                _ptrFileUtilityObj = NULL;
                return -1;
            }
            _fileFormat = format;
            break;
        }
        case kFileFormatPreencodedFile:
        {
            if(_ptrFileUtilityObj->InitPreEncodedWriting(stream, codecInst) ==
               -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Failed to initialize Pre-Encoded file!");
                delete _ptrFileUtilityObj;
                _ptrFileUtilityObj = NULL;
                return -1;
            }

            _fileFormat = kFileFormatPreencodedFile;
            break;
        }
#ifdef WEBRTC_MODULE_UTILITY_VIDEO
        case kFileFormatAviFile:
        {
            if( (_ptrFileUtilityObj->InitAviWriting(
                    fileName,
                    codecInst,
                    videoCodecInst,videoOnly) == -1) ||
                    (_ptrFileUtilityObj->codec_info(tmpAudioCodec) != 0))
            {
                WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                             "Failed to initialize AVI file!");
                delete _ptrFileUtilityObj;
                _ptrFileUtilityObj = NULL;
                return -1;
            }
            _fileFormat = kFileFormatAviFile;
            break;
        }
#endif
        default:
        {
            WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                         "Invalid file format %d specified!", format);
            delete _ptrFileUtilityObj;
            _ptrFileUtilityObj = NULL;
            return -1;
        }
    }
    _isStereo = (tmpAudioCodec.channels == 2);
    if(_isStereo)
    {
        if(_fileFormat != kFileFormatWavFile)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                         "Stereo is only allowed for WAV files");
            StopRecording();
            return -1;
        }
        if((STR_NCASE_CMP(codec_info_.plname, "L16", 4) != 0) &&
           (STR_NCASE_CMP(codec_info_.plname, "PCMU", 5) != 0) &&
           (STR_NCASE_CMP(codec_info_.plname, "PCMA", 5) != 0))
        {
            WEBRTC_TRACE(
                kTraceWarning,
                kTraceFile,
                _id,
                "Stereo is only allowed for codec PCMU, PCMA and L16 ");
            StopRecording();
            return -1;
        }
    }
    memcpy(&codec_info_, &tmpAudioCodec, sizeof(CodecInst));
    _recordingActive = true;
    _ptrOutStream = &stream;
    _notificationMs = notificationTimeMs;
    _recordDurationMs = 0;
    return 0;
}

WebRtc_Word32 MediaFileImpl::StopRecording()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id,
                 "MediaFileImpl::StopRecording()");

    CriticalSectionScoped lock(_crit);
    if(!_recordingActive)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceFile, _id,
                     "recording is not active!");
        return -1;
    }

    _isStereo = false;

    if(_ptrFileUtilityObj != NULL)
    {
        // Both AVI and WAV header has to be updated before closing the stream
        // because they contain size information.
        if((_fileFormat == kFileFormatWavFile) &&
            (_ptrOutStream != NULL))
        {
            _ptrFileUtilityObj->UpdateWavHeader(*_ptrOutStream);
        }
#ifdef WEBRTC_MODULE_UTILITY_VIDEO
        else if( _fileFormat == kFileFormatAviFile)
        {
            _ptrFileUtilityObj->CloseAviFile( );
        }
#endif
        delete _ptrFileUtilityObj;
        _ptrFileUtilityObj = NULL;
    }

    if(_ptrOutStream != NULL)
    {
        // If MediaFileImpl opened the OutStream it must be reclaimed here.
        if(_openFile)
        {
            delete _ptrOutStream;
            _openFile = false;
        }
        _ptrOutStream = NULL;
    }

    _recordingActive = false;
    codec_info_.pltype = 0;
    codec_info_.plname[0] = '\0';

    return 0;
}

bool MediaFileImpl::IsRecording()
{
    WEBRTC_TRACE(kTraceStream, kTraceFile, _id, "MediaFileImpl::IsRecording()");
    CriticalSectionScoped lock(_crit);
    return _recordingActive;
}

WebRtc_Word32 MediaFileImpl::RecordDurationMs(WebRtc_UWord32& durationMs)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id,
               "MediaFileImpl::RecordDurationMs()");

    CriticalSectionScoped lock(_crit);
    if(!_recordingActive)
    {
        durationMs = 0;
        return -1;
    }
    durationMs = _recordDurationMs;
    return 0;
}

bool MediaFileImpl::IsStereo()
{
    WEBRTC_TRACE(kTraceStream, kTraceFile, _id, "MediaFileImpl::IsStereo()");
    CriticalSectionScoped lock(_crit);
    return _isStereo;
}

WebRtc_Word32 MediaFileImpl::SetModuleFileCallback(FileCallback* callback)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id,
                 "MediaFileImpl::SetModuleFileCallback(callback= 0x%x)",
                 &callback);

    CriticalSectionScoped lock(_callbackCrit);

    _ptrCallback = callback;
    return 0;
}

WebRtc_Word32 MediaFileImpl::FileDurationMs(const WebRtc_Word8* fileName,
                                            WebRtc_UWord32& durationMs,
                                            const FileFormats format,
                                            const WebRtc_UWord32 freqInHz)
{
    WEBRTC_TRACE(
        kTraceModuleCall,
        kTraceFile,
        _id,
        "MediaFileImpl::FileDurationMs(%s, -, format= %d, freqInHz= %d)",
        (fileName == NULL) ? "NULL" : fileName,
        format,
        freqInHz);

    if(!ValidFileName(fileName))
    {
        return -1;
    }
    if(!ValidFrequency(freqInHz))
    {
        return -1;
    }

    ModuleFileUtility* utilityObj = new ModuleFileUtility(_id);
    if(utilityObj == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "failed to allocate utility object!");
        return -1;
    }

    const WebRtc_Word32 duration = utilityObj->FileDurationMs(fileName, format,
                                                              freqInHz);
    delete utilityObj;
    if(duration == -1)
    {
        durationMs = 0;
        return -1;
    }

    durationMs = duration;
    return 0;
}

WebRtc_Word32 MediaFileImpl::PlayoutPositionMs(WebRtc_UWord32& positionMs) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id,
                 "MediaFileImpl::PlayoutPositionMS(?)");
    CriticalSectionScoped lock(_crit);
    if(!_playingActive)
    {
        positionMs = 0;
        return -1;
    }
    positionMs = _playoutPositionMs;
    return 0;
}

WebRtc_Word32 MediaFileImpl::codec_info(CodecInst& codecInst) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id,
               "MediaFileImpl::codec_info(CodecInst= 0x%x)", &codecInst);
    CriticalSectionScoped lock(_crit);
    if(!_playingActive && !_recordingActive)
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "Neither playout nor recording has been initialized!");
        return -1;
    }
    if (codec_info_.pltype == 0 && codec_info_.plname[0] == '\0')
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "The CodecInst for %s is unknown!",
            _playingActive ? "Playback" : "Recording");
        return -1;
    }
    memcpy(&codecInst,&codec_info_,sizeof(CodecInst));
    return 0;
}

WebRtc_Word32 MediaFileImpl::VideoCodecInst(VideoCodec& codecInst) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceFile, _id,
               "MediaFileImpl::VideoCodecInst(CodecInst= 0x%x)", &codecInst);
    CriticalSectionScoped lock(_crit);
    if(!_playingActive && !_recordingActive)
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, _id,
                     "Neither playout nor recording has been initialized!");
        return -1;
    }
    if( _ptrFileUtilityObj == NULL)
    {
        return -1;
    }
#ifdef WEBRTC_MODULE_UTILITY_VIDEO
    VideoCodec videoCodec;
    if( _ptrFileUtilityObj->VideoCodecInst( videoCodec) != 0)
    {
        return -1;
    }
    memcpy(&codecInst,&videoCodec,sizeof(VideoCodec));
    return 0;
#else
    return -1;
#endif
}

bool MediaFileImpl::ValidFileFormat(const FileFormats format,
                                    const CodecInst*  codecInst)
{
    if(codecInst == NULL)
    {
        if(format == kFileFormatPreencodedFile ||
           format == kFileFormatPcm8kHzFile    ||
           format == kFileFormatPcm16kHzFile   ||
           format == kFileFormatPcm32kHzFile)
        {
            WEBRTC_TRACE(kTraceError, kTraceFile, -1,
                         "Codec info required for file format specified!");
            return false;
        }
    }
    return true;
}

bool MediaFileImpl::ValidFileName(const WebRtc_Word8* fileName)
{
    if((fileName == NULL) ||(fileName[0] == '\0'))
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, -1, "FileName not specified!");
        return false;
    }
    return true;
}


bool MediaFileImpl::ValidFilePositions(const WebRtc_UWord32 startPointMs,
                                       const WebRtc_UWord32 stopPointMs)
{
    if(startPointMs == 0 && stopPointMs == 0) // Default values
    {
        return true;
    }
    if(stopPointMs &&(startPointMs >= stopPointMs))
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, -1,
                     "startPointMs must be less than stopPointMs!");
        return false;
    }
    if(stopPointMs &&((stopPointMs - startPointMs) < 20))
    {
        WEBRTC_TRACE(kTraceError, kTraceFile, -1,
                     "minimum play duration for files is 20 ms!");
        return false;
    }
    return true;
}

bool MediaFileImpl::ValidFrequency(const WebRtc_UWord32 frequency)
{
    if((frequency == 8000) || (frequency == 16000)|| (frequency == 32000))
    {
        return true;
    }
    WEBRTC_TRACE(kTraceError, kTraceFile, -1,
                 "Frequency should be 8000, 16000 or 32000 (Hz)");
    return false;
}
} // namespace webrtc
