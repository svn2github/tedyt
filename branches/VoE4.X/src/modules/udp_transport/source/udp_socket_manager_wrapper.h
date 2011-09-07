/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_MANAGER_WRAPPER_H_
#define WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_MANAGER_WRAPPER_H_

#include "typedefs.h"

namespace webrtc {
class UdpSocketWrapper;

enum UdpSocketManagerCount
{
    kUdpSocketManagerDec = 0,
    kUdpSocketManagerInc = 1
};

enum UdpSocketManagerState
{
    kUdpSocketManagerExist   = 0,
    kUdpSocketManagerCreate  = 1,
    kUdpSocketManagerDestroy = 2
};

class UdpSocketManager
{
public:
    static UdpSocketManager* Create(const WebRtc_Word32 id,
                                    WebRtc_UWord8& numOfWorkThreads);
    static void Return();

    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id) = 0;

    // Start listening to sockets that have been registered via the
    // AddSocket(..) API.
    virtual bool Start() = 0;
    // Stop listening to sockets.
    virtual bool Stop() = 0;

    virtual WebRtc_UWord8 WorkThreads() const;

    // Register a socket with the socket manager.
    virtual bool AddSocket(UdpSocketWrapper* s) = 0;
    // Unregister a socket from the manager.
    virtual bool RemoveSocket(UdpSocketWrapper* s) = 0;

protected:
    UdpSocketManager(const WebRtc_Word32 /*id*/,
                     WebRtc_UWord8& numOfWorkThreads);

    virtual ~UdpSocketManager() {}

private:
    const WebRtc_UWord8 _numOfWorkThreads;

    // Factory method.
    static UdpSocketManager* CreateSocketManager(
        const WebRtc_Word32 id,
        WebRtc_UWord8& numOfWorkThreads);

    static UdpSocketManager* StaticInstance(const UdpSocketManagerCount inc,
                                            const WebRtc_Word32 id,
                                            WebRtc_UWord8& numOfWorkThreads);
};
} // namespace webrtc

#endif // WEBRTC_MODULES_UDP_TRANSPORT_SOURCE_UDP_SOCKET_MANAGER_WRAPPER_H_
