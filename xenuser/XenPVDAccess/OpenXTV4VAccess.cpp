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

#define OPENXTV4VAPI_EXPORTS
#include "OpenXTV4VAccess.h"

extern "C" {

//
//  v4v C implementation. The interface to the v4v kernel support within the PV Drivers
//  happens in the XenPVDAccess class.
BOOL V4VDriverAccept(void *aV4VContext, void *aNewContextOut,
                     V4V_ACCEPT_VALUES *aAcceptReturnParameters, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->Accept(aNewContextOut, aAcceptReturnParameters, aOverlapped) ? TRUE : FALSE;
}

BOOL V4VDriverBind(void *aV4VContext, v4v_ring_id_t *aRingId, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->Bind(aRingId, aOverlapped) ? TRUE : FALSE;
}
void V4VDriverClose(void *aV4VContext) {
    cOpenXTV4V::DestroyV4VInterface(aV4VContext);
}

BOOL V4VDriverConnect(void *aV4VContext, v4v_addr_t *aRingAddress, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->Connect(aRingAddress, aOverlapped) ? TRUE : FALSE;
}

BOOL V4VDriverConnectWait(void *aV4VContext, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->ConnectWait(aOverlapped) ? TRUE : FALSE;
}

BOOL V4VDriverDisconnect(void *aV4VContext, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->Disconnect(aOverlapped) ? TRUE : FALSE;
}

BOOL V4VDriverDumpRing(void *aV4VContext, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->DumpRing(aOverlapped) ? TRUE : FALSE;
}

BOOL V4VDriverGetInfo(void *aV4VContext, V4V_GETINFO_TYPE aInfoType, V4V_GETINFO_VALUES *aInfoResult, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->GetInfo(aInfoType, aInfoResult, aOverlapped) ? TRUE : FALSE;
}

void *V4VDriverInitialize(unsigned aRingByteCount, OVERLAPPED *aOverlapped) {
    return (void *)cOpenXTV4V::CreateV4VInterface(aRingByteCount, aOverlapped);
}

BOOL V4VDriverListen(void *aV4VContext, ULONG aBacklog, OVERLAPPED *aOverlapped) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;
    return openXTV4V->Listen(aBacklog, aOverlapped) ? TRUE : FALSE;
}
BOOL V4VDriverTestValid(void *aV4VContext) {
    cOpenXTV4V *openXTV4V = (cOpenXTV4V *)aV4VContext;

    return (openXTV4V && openXTV4V->TestValid()) ? TRUE : FALSE;
}};
