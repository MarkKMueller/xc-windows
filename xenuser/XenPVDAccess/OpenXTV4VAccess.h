/*
 * Copyright (c).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHWARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef OPENXTV4VACCESS
#define OPENXTV4VACCESS

#include "XenPVDAccess.h"

#include <OpenXTV4V.h>

#ifdef OPENXTV4VAPI_EXPORTS
#define OPENXTV4VAPI __declspec(dllexport)
#else
#define OPENXTV4VAPI __declspec(dllimport)
#endif


#if defined (__cplusplus)
extern "C" {
#endif
OPENXTV4VAPI BOOL V4VDriverAccept(void *aV4Vontext, void *aNewContextOut,
                                  V4V_ACCEPT_VALUES *aAcceptReturnParameters, OVERLAPPED *aOverlapped);
OPENXTV4VAPI BOOL V4VDriverBind(void *aV4VContext, v4v_ring_id_t *aRingId, OVERLAPPED *aOverlapped);
OPENXTV4VAPI void V4VDriverClose(void *aV4VContext);
OPENXTV4VAPI BOOL V4VDriverConnect(void *aV4VContext, v4v_addr_t *aRingAddress, OVERLAPPED *aOverlapped);
OPENXTV4VAPI BOOL V4VDriverConnectWait(void *aV4VContext, OVERLAPPED *aOverlapped);
OPENXTV4VAPI BOOL V4VDriverDisconnect(void *aV4VContext, OVERLAPPED *aOverlapped);
OPENXTV4VAPI BOOL V4VDriverDumpRing(void *aV4VContext, OVERLAPPED *aOverlapped);
OPENXTV4VAPI BOOL V4VDriverGetInfo(void *aV4VContext, V4V_GETINFO_TYPE aInfoType, V4V_GETINFO_VALUES *aInfoResult, OVERLAPPED *aOverlapped);
OPENXTV4VAPI void *V4VDriverInitialize(unsigned aRingByteCount, OVERLAPPED *aOverlapped);
OPENXTV4VAPI BOOL V4VDriverListen(void *aV4VContext, ULONG aBacklog, OVERLAPPED *aOverlapped);
OPENXTV4VAPI BOOL V4VDriverTestValid(void *aV4VContext);
#if defined (__cplusplus)
};

class cOpenXTV4V {
public:
    static cOpenXTV4V *CreateV4VInterface(unsigned aRingByteCount, OVERLAPPED *aOverlapped = 0) {
        cOpenXTV4V *openXTV4V = new cOpenXTV4V(aRingByteCount, aOverlapped);

        return openXTV4V;
    }
    static void DestroyV4VInterface(void *aOpenXTV4V) {
        delete (cOpenXTV4V *)aOpenXTV4V;
    }

    cOpenXTV4V(unsigned aRingByteCount, OVERLAPPED *aOverlapped = 0) :
        XenPVDAccess(aOverlapped ? FILE_FLAG_OVERLAPPED : 0),
        RingByteCount(aRingByteCount),
        ValidInterface(false) {
        dCreateSentinel(sentinel);
        EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (0 == EventHandle) {
            int errorCode = GetLastError();
            sentinel.PostMessage("CreateEventFailed: %d", errorCode);
            SetLastError(errorCode);
            return;
        }

        V4V_INIT_VALUES initParameters = {0};
        initParameters.ringLength = aRingByteCount;
        initParameters.rxEvent = EventHandle;

        if (SendIOCTL(V4V_IOCTL_INITIALIZE, &initParameters,
                      sizeof(initParameters), aOverlapped, sentinel)) {
            ValidInterface = true;
            return;
        }

        const int errorCode = GetLastError();
        CloseHandle(EventHandle);
        EventHandle = 0;
        SetLastError(errorCode);
    }

    ~cOpenXTV4V() {
        dCreateSentinel(sentinel);
        if (EventHandle) {
            CloseHandle(EventHandle);
        }
    }

    bool Accept(void *aNewContextOut, V4V_ACCEPT_VALUES *aAcceptReturnParameters, OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        cOpenXTV4V *openXTV4V = new cOpenXTV4V(RingByteCount, aOverlapped);
        if (false == openXTV4V->TestValid()) {
            return false;
        }
        V4V_ACCEPT_VALUES acceptParameters = {0};
        acceptParameters.fileHandle = (void *)openXTV4V;
        acceptParameters.rxEvent = openXTV4V->EventHandle;

        ZeroMemory(aAcceptReturnParameters, sizeof(*aAcceptReturnParameters));

        if (SendIOCTL(V4V_IOCTL_ACCEPT, &acceptParameters,
                      sizeof(acceptParameters), aOverlapped, sentinel,
                      aAcceptReturnParameters, sizeof(*aAcceptReturnParameters))) {
            aNewContextOut = (void *)acceptParameters.fileHandle;
            return true;
        }
        return false;
    }

    bool Bind(v4v_ring_id_t *aRingId, OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        V4V_BIND_VALUES bindParameters = {0};
        memcpy(&bindParameters.ringId, aRingId, sizeof(v4v_ring_id_t));

        return SendIOCTL(V4V_IOCTL_BIND, &bindParameters,
                         sizeof(bindParameters), aOverlapped, sentinel);
    }

    bool Connect(v4v_addr_t *aRingAddress, OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        V4V_CONNECT_VALUES connectParameters = {0};
        memcpy(&connectParameters.ringAddr, aRingAddress, sizeof(v4v_addr_t));

        return SendIOCTL(V4V_IOCTL_CONNECT, &connectParameters,
                         sizeof(connectParameters), aOverlapped, sentinel);
    }

    bool ConnectWait(OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        V4V_WAIT_VALUES waitParameters = {0};
        return SendIOCTL(V4V_IOCTL_WAIT, &waitParameters,
                         sizeof(waitParameters), aOverlapped, sentinel);
    }

    bool Disconnect(OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        return SendIOCTL(V4V_IOCTL_DISCONNECT, 0, 0, aOverlapped, sentinel);
    }

    bool DumpRing(OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        return SendIOCTL(V4V_IOCTL_DUMPRING, 0, 0, aOverlapped, sentinel);
    }

    HANDLE GetDriverHandle() {
        return XenPVDAccess.GetHandle();
    }

    HANDLE GetEventHandle() {
        return EventHandle;
    }

    bool GetInfo(V4V_GETINFO_TYPE aInfoType, V4V_GETINFO_VALUES *aInfoResult, OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        V4V_GETINFO_VALUES infoParameters = {V4vInfoUnset, {{V4V_PORT_NONE, V4V_DOMID_NONE}, V4V_DOMID_NONE}};
        infoParameters.type = aInfoType;

        ZeroMemory(aInfoResult, sizeof(V4V_GETINFO_VALUES));
        aInfoResult->type = V4vInfoUnset;

        return SendIOCTL(V4V_IOCTL_GETINFO, &infoParameters,
                         sizeof(infoParameters), aOverlapped, sentinel, aInfoResult, sizeof(*aInfoResult));
    }

    bool Listen(unsigned aBacklog, OVERLAPPED *aOverlapped = 0) {
        dCreateSentinel(sentinel);
        V4V_LISTEN_VALUES listenParameters = {0};
        listenParameters.backlog = aBacklog;

        return SendIOCTL(V4V_IOCTL_LISTEN, &listenParameters,
                         sizeof(listenParameters), aOverlapped, sentinel);
    }

    bool TestValid() {
        return ValidInterface;
    }

protected:
    bool SendIOCTL(int aIOCTLCode, void *aParameters, DWORD aParameterSize,
                   OVERLAPPED *aOverlapped, cFunctionSentinel &aSentinel,
                   void *aReturnParameters = 0, DWORD aReturnParameterSize = 0) {
        DWORD bytesReturned;
        const int errorReturn = DeviceIoControl(XenPVDAccess.GetHandle(), aIOCTLCode, aParameters,
                                                aParameterSize, aReturnParameters, aReturnParameterSize,
                                                &bytesReturned, aOverlapped);
        if (aOverlapped) {
            const int errorCode = GetLastError();
            if ((ERROR_SUCCESS != errorCode) && (ERROR_IO_PENDING != errorCode)) {
                aSentinel.PostMessage("DeviceIoControl Failed Overlapped IO: %d", errorCode);
                SetLastError(errorCode);
                return false;
            }
        }
        else if (0 == errorReturn) {
            const int errorCode = GetLastError();
            aSentinel.PostMessage("DeviceIoControl Failed Non-Overlapped IO: %d", errorCode);
            SetLastError(errorCode);
            return false;
        }
        return true;
    }

    HANDLE EventHandle;
    unsigned RingByteCount;
    cXenPVDAccess XenPVDAccess;

    bool ValidInterface;
};

#endif  // _cplusplus

#endif // OPENXTV4VACCESS

