/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "voe_errors.h"
#include "voe_base.h"
#include "voe_codec.h"
#include "voe_volume_control.h"
#include "voe_dtmf.h"
#include "voe_rtp_rtcp.h"
#include "voe_audio_processing.h"
#include "voe_file.h"
#include "voe_video_sync.h"
#include "voe_encryption.h"
#include "voe_hardware.h"
#include "voe_external_media.h"
#include "voe_network.h"
#include "voe_neteq_stats.h"
#include "engine_configurations.h"
#ifdef DYLIB
#include "dlfcn.h"
#endif

//#define DEBUG

//#define EXTERNAL_TRANSPORT

using namespace webrtc;

#define VALIDATE                                                        \
    if (res != 0)                                                       \
    {                                                                   \
        printf("*** Error at position %i / line %i \n", cnt, __LINE__); \
        printf("*** Error code = %i \n", base1->LastError());    \
        error = 1;                                                      \
    }                                                                   \
    cnt++;

VoiceEngine* m_voe = NULL;
VoEBase* base1 = NULL;
VoECodec* codec = NULL;
VoEVolumeControl* volume = NULL;
VoEDtmf* dtmf = NULL;
VoERTP_RTCP* rtp_rtcp = NULL;
VoEAudioProcessing* apm = NULL;
VoENetwork* netw = NULL;
VoEFile* file = NULL;
VoEVideoSync* vsync = NULL;
VoEEncryption* encr = NULL;
VoEHardware* hardware = NULL;
VoEExternalMedia* xmedia = NULL;
VoENetEqStats* neteqst = NULL;

void loopBack(int);

#ifdef EXTERNAL_TRANSPORT

class my_transportation : public Transport
{
    int SendPacket(int channel,const void *data,int len);
    int SendRTCPPacket(int channel, const void *data, int len);
};

int my_transportation::SendPacket(int channel,const void *data,int len)
{
    netw->ReceivedRTPPacket(channel, data, len);
    return 0;
}

int my_transportation::SendRTCPPacket(int channel, const void *data, int len)
{
    netw->ReceivedRTCPPacket(channel, data, len);
    return 0;
}

my_transportation my_transport;

#endif

int main(int /*argc*/, char* /*argv[]*/*)
{
    int res = 0;
    int cnt = 0;
    int error = 0;

    printf("Test started \n");

    m_voe = VoiceEngine::Create();
    base1 = VoEBase::GetInterface(m_voe);
    codec = VoECodec::GetInterface(m_voe);
    apm = VoEAudioProcessing::GetInterface(m_voe);
    volume = VoEVolumeControl::GetInterface(m_voe);
    dtmf = VoEDtmf::GetInterface(m_voe);
    rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    netw = VoENetwork::GetInterface(m_voe);
    file = VoEFile::GetInterface(m_voe);
    vsync = VoEVideoSync::GetInterface(m_voe);
    encr = VoEEncryption::GetInterface(m_voe);
    hardware = VoEHardware::GetInterface(m_voe);
    xmedia = VoEExternalMedia::GetInterface(m_voe);
    neteqst = VoENetEqStats::GetInterface(m_voe);

    int year = 0, month = 0, day = 0; // Set correct expiry values here when testing
    printf("Set trace filenames (enable trace)\n");
    //VoiceEngine::SetTraceFilter(kTraceAll);
    // VoiceEngine::SetTraceFilter(kTraceAll);
    // VoiceEngine::SetTraceFilter(kTraceStream |
    //  kTraceStateInfo | kTraceWarning | kTraceError |
    //  kTraceCritical | kTraceApiCall | kTraceModuleCall |
    // kTraceMemory | kTraceDebug | kTraceInfo);
    //VoiceEngine::SetTraceFilter(kTraceStateInfo | kTraceWarning |
    // kTraceError | kTraceCritical | kTraceApiCall |
    // kTraceModuleCall | kTraceMemory | kTraceInfo);

    res = VoiceEngine::SetTraceFile("4_X_trace.txt");
    VALIDATE;
    //res = VoiceEngine::SetTraceCallback(NULL);
    //VALIDATE;

    int select = 1;
#ifdef WEBRTC_LINUX
    printf("Enter 0 for LINUX_AUDIO_ALSA\nEnter 1 for LINUX_AUDIO_PULSE\n");
    int dummy(0);
    dummy = scanf("%d", &select);
#endif

    printf("Init\n");
    res = base1->Init();
    if (res != 0)
    {
        printf("\nError calling Init: %d\n", base1->LastError());
        fflush(NULL);
        exit(0);
    }

    cnt++;
    printf("Version\n");
    char tmp[1024];
    res = base1->GetVersion(tmp);
    VALIDATE;
    cnt++;
    printf("%s\n", tmp);
    loopBack(select);

    printf("Terminate \n");

    res = base1->Terminate();
    VALIDATE;

    if (base1)
        base1->Release();

    if (codec)
        codec->Release();

    if (volume)
        volume->Release();

    if (dtmf)
        dtmf->Release();

    if (rtp_rtcp)
        rtp_rtcp->Release();

    if (apm)
        apm->Release();

    if (netw)
        netw->Release();

    if (file)
        file->Release();

    if (vsync)
        vsync->Release();

    if (encr)
        encr->Release();

    if (hardware)
        hardware->Release();

    if (xmedia)
        xmedia->Release();

    if (neteqst)
        neteqst->Release();

    VoiceEngine::Delete(m_voe);

    return 0;
}

void loopBack(int)
{
    int chan, cnt, error, res;
    CodecInst cinst;
    cnt = 0;
    int i;
    int dummy(0);
    int codecinput;
    bool AEC = false;
    bool AGC = true;
    bool AGC1 = false;
    bool VAD = false;
    bool NS = false;
    bool NS1 = false;

    chan = base1->CreateChannel();
    if (chan < 0)
    {
        printf("Error at position %i\n", cnt);
        printf("************ Error code = %i\n", base1->LastError());
        fflush(NULL);
        error = 1;
    }
    cnt++;

#ifdef EXTERNAL_TRANSPORT
    my_transportation ch0transport;
    printf("Enabling external transport \n");
    netw->SetExternalTransport(0, true, &ch0transport);
#else
    char ip[64];
#ifdef DEBUG
    strcpy(ip, "127.0.0.1");
#else
    char localip[64];
    netw->GetLocalIP(localip, 64);
    printf("local IP:%s\n", localip);

    printf("1. 127.0.0.1 \n");
    printf("2. Specify IP \n");
    dummy = scanf("%i", &i);
    if (1 == i)
        strcpy(ip, "127.0.0.1");
    else
    {
        printf("Specify remote IP: ");
        dummy = scanf("%s", ip);
    }
#endif

    int colons(0), j(0);
    while (ip[j] != '\0' && j < 64 && !(colons = (ip[j++] == ':')))
        ;
    if (colons)
    {
        printf("Enabling IPv6\n");
        res = netw->EnableIPv6(0);
        VALIDATE;
    }

    int rPort;
#ifdef DEBUG
    rPort=8500;
#else
    printf("Specify remote port (1=1234): ");
    dummy = scanf("%i", &rPort);
    if (1 == rPort)
        rPort = 1234;
    printf("Set Send port \n");
#endif

    printf("Set Send IP \n");
    res = base1->SetSendDestination(chan, rPort, ip);
    VALIDATE

    int lPort;
#ifdef DEBUG
    lPort=8500;
#else
    printf("Specify local port (1=1234): ");
    dummy = scanf("%i", &lPort);
    if (1 == lPort)
        lPort = 1234;
    printf("Set Rec Port \n");
#endif
    res = base1->SetLocalReceiver(chan, lPort);
    VALIDATE
#endif

    printf("\n");
    for (i = 0; i < codec->NumOfCodecs(); i++)
    {
        res = codec->GetCodec(i, cinst);
        VALIDATE
        if (strncmp(cinst.plname, "ISAC", 4) == 0 && cinst.plfreq == 32000)
        {
            printf("%i. ISAC-swb pltype:%i plfreqi:%i\n", i, cinst.pltype,
                   cinst.plfreq);
        } else
        {
            printf("%i. %s pltype:%i plfreq:%i\n", i, cinst.plname,
                   cinst.pltype, cinst.plfreq);
        }
    }
#ifdef DEBUG
    codecinput=0;
#else
    printf("Select send codec: ");
    dummy = scanf("%i", &codecinput);
#endif
    codec->GetCodec(codecinput, cinst);

    printf("Set primary codec\n");
    res = codec->SetSendCodec(chan, cinst);
    VALIDATE

    // Call loop
    bool newcall = true;
    while (newcall)
    {

#ifdef WEBRTC_LINUX
        // ALSA

        // NOTE: hw:0,0, is NOT the default device, it's a raw kernel device mapping.
        //       "default" is the default device, which is used if no call is made
        //        to SetRecordingDevice / SetPlayoutDevice.
        //        This is what we shall use for normal testing.
        int rd(-1), pd(-1);
        res = hardware->GetNumOfRecordingDevices(rd);
        VALIDATE;
        res = hardware->GetNumOfPlayoutDevices(pd);
        VALIDATE;

        char dn[64] =
        {   0};
        char guid[128] =
        {   0};
        printf("\nPlayout devices (%d): \n", pd);
        for (j=0; j<pd; ++j)
        {
            res = hardware->GetPlayoutDeviceName(j, dn, guid);
            VALIDATE;
            printf("  %d: %s \n", j, dn);
        }

        printf("Recording devices (%d): \n", rd);
        for (j=0; j<rd; ++j)
        {
            res = hardware->GetRecordingDeviceName(j, dn, guid);
            VALIDATE;
            printf("  %d: %s \n", j, dn);
        }

        printf("Select playout device: ");
        dummy = scanf("%d", &pd);
        res = hardware->SetPlayoutDevice(pd); // Will use plughw for hardware devices
        VALIDATE;
        printf("Select recording device: ");
        dummy = scanf("%d", &rd);
        printf("Setting sound devices \n");
        res = hardware->SetRecordingDevice(rd); // Will use plughw for hardware devices
        VALIDATE;

#endif // LINUX
        res = codec->SetVADStatus(0, VAD);
        VALIDATE

        res = apm->SetAgcStatus(AGC);
        VALIDATE

        res = apm->SetEcStatus(AEC);
        VALIDATE

        res = apm->SetNsStatus(NS);
        VALIDATE

#ifdef DEBUG
        i = 1;
#else
        printf("\n1. Send, listen and playout \n");
        printf("2. Send only \n");
        printf("3. Listen and playout only \n");
        printf("Select transfer mode: ");
        dummy = scanf("%i", &i);
#endif
        const bool send = !(3 == i);
        const bool receive = !(2 == i);

        if (receive)
        {
#ifndef EXTERNAL_TRANSPORT
            printf("Start Listen \n");
            res = base1->StartReceive(chan);
            VALIDATE;
#endif

            printf("Start Playout \n");
            res = base1->StartPlayout(chan);
            VALIDATE;
        }

        if (send)
        {
            // GQoS is not supported on Linux and the only
            // way to set the TOS value is using this function
            // the useSetSockop param is ignored as the only
            // way to set the value is with setsockopt  
            //res=netw->SetSendTOS(chan, 12, false);
            //VALIDATE;

            printf("Start Send \n");
            res = base1->StartSend(chan);
            VALIDATE;
        }

        //        res=file->StartRecordingMicrophone("MicRecording.pcm");


        printf("Getting mic volume \n");
        unsigned int vol = 999;
        res = volume->GetMicVolume(vol);
        VALIDATE
        if ((vol > 255) || (vol < 1))
        {
            printf("\n****ERROR in GetMicVolume");

        }
        int forever = 1;
        while (forever)
        {
            printf("\nActions\n");

            printf("Codec Changes\n");
            for (i = 0; i < codec->NumOfCodecs(); i++)
            {
                res = codec->GetCodec(i, cinst);
                VALIDATE
                if (strncmp(cinst.plname, "ISAC", 4) == 0 && cinst.plfreq
                    == 32000)
                {
                    printf("\t%i. ISAC-swb pltype:%i plfreq:%i\n", i,
                           cinst.pltype, cinst.plfreq);
                } else
                {
                    printf("\t%i. %s pltype:%i plfreq:%i\n", i, cinst.plname,
                           cinst.pltype, cinst.plfreq);
                }
            }
            printf("Other\n");
            const int noCodecs = i - 1;
            printf("\t%i. Toggle VAD\n", i);
            i++;
            printf("\t%i. Toggle AGC\n", i);
            i++;
            printf("\t%i. Toggle NS\n", i);
            i++;
            printf("\t%i. Toggle echo control\n", i);
            i++;
            printf("\t%i. Select AEC\n", i);
            i++;
            printf("\t%i. Select AECM\n", i);
            i++;
            printf("\t%i. Get speaker volume\n", i);
            i++;
            printf("\t%i. Set speaker volume\n", i);
            i++;
            printf("\t%i. Get microphone volume\n", i);
            i++;
            printf("\t%i. Set microphone volume\n", i);
            i++;
            printf("\t%i. Stereo \n", i);
            i++;
            printf("\t%i. Play local file \n", i);
            i++;
            printf("\t%i. Change Playout Device \n", i);
            i++;
            printf("\t%i. Change Recording Device \n", i);
            i++;
            printf("\t%i. Toggle Remote AGC \n", i);
            i++;
            printf("\t%i. Toggle Remote NS \n", i);
            i++;
            printf("\t%i. agc status \n", i);
            i++;
            printf("\t%i. Stop call \n", i);

            printf("Select action or %i to stop the call: ", i);
            dummy = scanf("%i", &codecinput);

            if (codecinput < codec->NumOfCodecs())
            {
                res = codec->GetCodec(codecinput, cinst);
                VALIDATE

                // Test "Super Wideband" SWB
                //if (!strcmp(cinst.plname, "G7221"))
                //{
                //    cinst.plfreq = 32000;
                //    cinst.pacsize = 640;
                //    cinst.rate = 24000; 
                //    cinst.rate = 32000; 
                //    cinst.rate = 48000; 
                //    base1->StopPlayout(0);
                //    base1->StopReceive(0);
                //    codec->SetRecPayloadType(0, cinst);
                //    base1->StartReceive(0);
                //    base1->StartPlayout(0);
                //}

                printf("Set primary codec\n");
                res = codec->SetSendCodec(chan, cinst);
                VALIDATE
            } else if (codecinput == (noCodecs + 1))
            {
                VAD = !VAD;
                res = codec->SetVADStatus(0, VAD);
                VALIDATE
                if (VAD)
                    printf("\n VAD is now on! \n");
                else
                    printf("\n VAD is now off! \n");
            } else if (codecinput == (noCodecs + 2))
            {
                AGC = !AGC;

                res = apm->SetAgcStatus(AGC);
                VALIDATE
                if (AGC)
                    printf("\n AGC is now on! \n");
                else
                    printf("\n AGC is now off! \n");
            } else if (codecinput == (noCodecs + 3))
            {
                NS = !NS;
                res = apm->SetNsStatus(NS);
                VALIDATE
                if (NS)
                    printf("\n NS is now on! \n");
                else
                    printf("\n NS is now off! \n");
            } else if (codecinput == (noCodecs + 4))
            {
                AEC = !AEC;
                res = apm->SetEcStatus(AEC, kEcUnchanged);
                VALIDATE
                if (AEC)
                    printf("\n Echo control is now on! \n");
                else
                    printf("\n Echo control is now off! \n");
            } else if (codecinput == (noCodecs + 5))
            {
                res = apm->SetEcStatus(AEC, kEcAec);
                VALIDATE
                printf("\n AEC selected! \n");
                if (AEC)
                    printf(" (Echo control is on)\n");
                else
                    printf(" (Echo control is off)\n");
            } else if (codecinput == (noCodecs + 6))
            {
                res = apm->SetEcStatus(AEC, kEcAecm);
                VALIDATE
                printf("\n AECM selected! \n");
                if (AEC)
                    printf(" (Echo control is on)\n");
                else
                    printf(" (Echo control is off)\n");
            } else if (codecinput == (noCodecs + 7))
            {
                unsigned vol(0);
                res = volume->GetSpeakerVolume(vol);
                VALIDATE;
                printf("\n Speaker Volume is %d \n", vol);
            } else if (codecinput == (noCodecs + 8))
            {
                printf("Level: ");
                dummy = scanf("%i", &i);
                res = volume->SetSpeakerVolume(i);
                VALIDATE;
            } else if (codecinput == (noCodecs + 9))
            {
                unsigned vol(0);
                res = volume->GetMicVolume(vol);
                VALIDATE;
                printf("\n Microphone Volume is %d \n", vol);
            } else if (codecinput == (noCodecs + 10))
            {
                printf("Level: ");
                dummy = scanf("%i", &i);
                res = volume->SetMicVolume(i);
                VALIDATE;
            } else if (codecinput == (noCodecs + 11))
            {
                //use different IP address for remote to test this. non-loopback
                //use rtp to send to port 1234
                //Here we are using channel 0 to test stereo
                //Refere to windows code stub for more info
                CodecInst c;
                base1->DeleteChannel(chan);
                res = base1->CreateChannel();
                VALIDATE
                base1->StopPlayout(chan);
                base1->StopReceive(chan);
                //G729 
                //c.channels = 2; c.pacsize = 160; c.plfreq = 8000;
                //strcpy(c.plname, "G729"); c.pltype = 18; c.rate = 8000;
                //G722_1_C - 48k rate
                //c.channels = 2; c.pacsize = 640; c.plfreq = 32000;
                //strcpy(c.plname, "G7221"); c.pltype = 125; c.rate = 48000;
                //L16 at 16Khz
                //c.channels = 2; c.pacsize = 160; c.plfreq = 16000;
                //strcpy(c.plname, "L16"); c.pltype = 125; c.rate = 256000;
                //PCMU
                c.channels = 2;
                c.pacsize = 160;
                c.plfreq = 8000;
                strcpy(c.plname, "PCMU");
                c.pltype = 125;
                c.rate = 64000;
                res = codec->SetRecPayloadType(chan, c);
                VALIDATE
                res = base1->SetLocalReceiver(chan, lPort);
                VALIDATE
                base1->StartReceive(chan);
                res = base1->SetSendDestination(chan, lPort, ip);
                VALIDATE
                res = base1->StartSend(chan);
                VALIDATE;
                base1->StartPlayout(chan);
            } else if (codecinput == (noCodecs + 12))
            {
                file->StartPlayingFileLocally(0, "singleUserDemo.pcm");
            } else if (codecinput == (noCodecs + 13))
            {
                // change the playout device with current call
                int num_pd(-1);
                res = hardware->GetNumOfPlayoutDevices(num_pd);
                VALIDATE;

                char dn[64] = { 0 };
                char guid[128] = { 0 };

                printf("\nPlayout devices (%d): \n", num_pd);
                for (j = 0; j < num_pd; ++j)
                {
                    res = hardware->GetPlayoutDeviceName(j, dn, guid);
                    VALIDATE;
                    printf("  %d: %s \n", j, dn);
                }
                printf("Select playout device: ");
                dummy = scanf("%d", &num_pd);
                // Will use plughw for hardware devices
                res = hardware->SetPlayoutDevice(num_pd);
                VALIDATE;

            } else if (codecinput == (noCodecs + 14))
            {
                // change the recording device with current call
                int num_rd(-1);

                res = hardware->GetNumOfRecordingDevices(num_rd);
                VALIDATE;

                char dn[64] = { 0 };
                char guid[128] = { 0 };

                printf("Recording devices (%d): \n", num_rd);
                for (j = 0; j < num_rd; ++j)
                {
                    res = hardware->GetRecordingDeviceName(j, dn, guid);
                    VALIDATE;
                    printf("  %d: %s \n", j, dn);
                }

                printf("Select recording device: ");
                dummy = scanf("%d", &num_rd);
                printf("Setting sound devices \n");
                // Will use plughw for hardware devices
                res = hardware->SetRecordingDevice(num_rd);
                VALIDATE;
            } else if (codecinput == (noCodecs + 15))
            {
                // Remote AGC
                AGC1 = !AGC1;
                res = apm->SetRxAgcStatus(chan, AGC1);
                VALIDATE
                if (AGC1)
                    printf("\n Remote AGC is now on! \n");
                else
                    printf("\n Remote AGC is now off! \n");
            } else if (codecinput == (noCodecs + 16))
            {
                // Remote NS
                NS1 = !NS1;
                res = apm->SetRxNsStatus(chan, NS);
                VALIDATE
                if (NS1)
                    printf("\n Remote NS is now on! \n");
                else
                    printf("\n Remote NS is now off! \n");
            } else if (codecinput == (noCodecs + 17))
            {
                AgcModes agcmode;
                bool enable;
                res = apm->GetAgcStatus(enable, agcmode);
                VALIDATE
                printf("\n AGC enale is %d , mode is %d \n", enable, agcmode);
            } else
                break;
        }

        if (send)
        {
            printf("Stop Send \n");
            res = base1->StopSend(chan);
            VALIDATE;
        }

        if (receive)
        {
            printf("Stop Playout \n");
            res = base1->StopPlayout(chan);
            VALIDATE;

#ifndef EXTERNAL_TRANSPORT
            printf("Stop Listen \n");
            res = base1->StopReceive(chan);
            VALIDATE;
#endif
        }

        printf("\n1. New call \n");
        printf("2. Quit \n");
        printf("Select action: ");
        dummy = scanf("%i", &i);
        newcall = (1 == i);

        // Call loop
    }

    printf("Delete Channel \n");
    res = base1->DeleteChannel(chan);
    VALIDATE
}
