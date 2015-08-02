/*
 * Copyright (c) 2011 Citrix Systems, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __V4VAPI_H__
#define __V4VAPI_H__

#include <OpenXTV4VAccess.h>

/* =========================== User Mode API ============================== */

/* The following must be included before this header:
 * #include <windows.h>
 * #include <winioctl.h>
 */

/* V4V for Windows uses the basic file I/O model standard Windows API 
 * functions. All access to the V4V device is accomplished through use of a
 * Windows file handle returned by a call to CreateFile() in V4vOpen().
 * Several other V4V calls can then be made to initialize the V4V file (which
 * represents a particular V4V channel) for the specific operations desired. 
 * Once open and configured, Windows API file functions can be used to 
 * read/write/control V4V file IO. The following are the functions that would
 * be used with V4V:
 *
 * ReadFile()/ReadFileEx()
 * WriteFile()/WriteFileEx()
 * CancelIo()/CancelIoEx()
 *
 * Note that V4V supports for file IO both synchronous blocking mode or
 * asynchronous mode through use of an OVERLAPPED structure or IO completion
 * routines. The caller should not attempt to manipulate the V4V device
 * directly with DeviceIoControl() calls. The proper IOCTLs are sent through
 * the set of functions in the V4V API below. The V4V API also returns an
 * event handle that is signalled when data arrives on a V4V channel. This
 * handle can be used in any Windows API functions that operate on events
 * (e.g. WaitForMultipleObjects() in conjunction with OVERLAPPED IO events).
 *
 * V4V supports both datagram/connectionless and sream/connection types of
 * communication.
 *
 * Datagrams:
 * A V4V channel must simply be bound to send and receive datagrams. When
 * reading datagrams, if the buffer is smaller than next message size the
 * extra bytes will be discarded. When writing, the message cannot exceed
 * the maximum message the V4V channel can accomodate and ERROR_MORE_DATA
 * is returned. If the destination does not exist, ERROR_VC_DISCONNECTED
 * is returned. If the channel is not bound then ERROR_INVALID_FUNCTION
 * will be returned for all IO operations.
 *
 * Streams:
 * V4vListen()/V4vAccept()/V4vConnect()/V4vConnectWait() are used to
 * establish a stream channel. Read operations will read the next chunk
 * of the stream data out of the V4V channel. Note the read length may
 * be less than the supplied buffer. Currently, if stream data chunk is
 * bigger than the supplied buffer, ERROR_MORE_DATA will be returned
 * indicating a bigger buffer should be used. When writing data chunks
 * the call will block or pend until enough room is available for the
 * send. The written chunk cannot exceed the maximum message the V4V 
 * channel can accomodate and ERROR_MORE_DATA is returned. Attempts
 * to read and write after a reset or disconnection will result in
 * ERROR_VC_DISCONNECTED being returned.
 */

/* Define V4V_USE_INLINE_API to specify inline for the V4V API below */
#if defined(V4V_USE_INLINE_API)
#define V4V_INLINE_API __inline
#else
#define V4V_INLINE_API
#endif

/* Default @ringId for V4vBind() to specify no specific binding information */
static const v4v_ring_id_t V4V_DEFAULT_CONNECT_ID = {{V4V_PORT_NONE, V4V_DOMID_NONE}, V4V_DOMID_NONE};

/* This routine opens a V4V file and associated channel. The @context
 * structure is passed in to the routine and if the call is successful, the
 * @OpenXTV4VDevice and @recvEvent handles will be valid and ready for use in
 * further V4V calls to initialize the channel.
 *
 * The @ringSize argument indicates how large the local receive ring for the
 * channel should be in bytes.
 *
 * If the V4V file is being opened with the V4V_FLAG_OVERLAPPED flag set then 
 * the V4vOpen() operation must be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * open is complete.
 * 
 * The new open file handle and receive event are returned event though an
 * a overlapped call may not have yet completed. Until an overlapped call 
 * completes the values in the @context should not be use.
 *
 * Returns TRUE on success or FALSE on error, in which case more
 * information can be obtained by calling GetLastError().
 */
static V4V_INLINE_API BOOL
V4vOpen(void *context, ULONG ringSize, OVERLAPPED *ov)
{
    context = V4VDriverInitialize(ringSize, ov);

    return V4VDriverTestValid(context);
}

/* All users of V4V must call V4vBind() before calling any of the other V4V
 * functions (excpetion V4vClose()) or before performing IO operations. When
 * binding, the @ringId->addr.domain field must be set to V4V_DOMID_NONE or
 * the bind operation will fail. Internally this value will be set to the
 * current domain ID.
 *
 * For V4V channels intended for datagram use, the @ringId->addr.port field
 * can be specified or not. If not specified, a random port number will be
 * assigned internally. The @ringId->partner value can be specified if 
 * datagrams are to only be received from a specific partner domain for the
 * current V4V channel. If V4V_DOMID_ANY is specified, then datagrams from
 * any domain can be recieved. Note that V4V will send datagrams to channels
 * that match a specific domain ID before sending them to one bound with 
 * (@ringId->partner == V4V_DOMID_ANY).
 * 
 * The above rules apply when binding to start a listener though in general
 * one would want to specify a well known port for a listener. When binding
 * to do a connect, V4V_DEFAULT_CONNECT_ID can be used to allow internal
 * values to be selected.
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vBind() operation must be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * bind is complete.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION will
 * be returned if the file is not in the proper state following a call to
 * V4vOpen().
 */
static V4V_INLINE_API BOOL
V4vBind(void *context, v4v_ring_id_t *ringId, OVERLAPPED *ov)
{
    return V4VDriverBind(context, ringId, ov);
}

/* This routine starts a listening V4V channel. For listening channels, the
 * @context->recvEvent will become signaled when a new connection is ready to
 * be accepted with a call to V4vAccept().
 *
 * The @backlog argument specifies the maximum length of pending accepts
 * to maintain. If 0 is specified, the V4V_SOMAXCONN value is used. @backlog
 * cannot be greater than V4V_SOMAXCONN.
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vListen() operation must be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * listen is complete.
 *
 * If the listener @context is closed before all the accepted connections are
 * closed, the existing accepted connections will remain connected. No new
 * connections will be accepted for this rind ID.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION will
 * be returned if the file is not in the bound state following a call to
 * V4vBind().
 */
static V4V_INLINE_API BOOL
V4vListen(void *context, ULONG backlog, OVERLAPPED *ov)
{
    return V4VDriverListen(context, backlog, ov);
}

/* Once a listening channel is established, calls can be made to V4vAccept()
 * to accept new connections. The new connection context is returned in the
 * @newContextOut argument that the caller must allocate. The newly accepted 
 * V4V channel will be a stream connection.
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vAccept() operation must be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until there
 * is an incoming connection. Note the @context->recvEvent for the listening
 * channel will be signaled when incoming connections arrive.
 *
 * The new open file handle and receive event are returned event though an
 * a overlapped call may not have yet completed. Until an overlapped call 
 * completes the values in the @newContextOut should not be use.
 *
 * The caller must suppy the @acceptOut argument. Upon synchronous completion
 * of the call, this structure will have the @acceptOut.peerAddr field filled
 * in (the other fields should be ignored). For overlapped calls, the caller 
 * retain the @acceptOut structure until IO is completed (this is effectively
 * the output buffer for the IOCLT). During GetOverlappedResult() or in the 
 * FileIOCompletionRoutine the @acceptOut.peerAddr value can be fetched and
 * @acceptOut released etc.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION will
 * be returned if the file represented by the @context is not in the listen 
 * state following a call to V4vListen().
 */
static V4V_INLINE_API BOOL
V4vAccept(void *context, void *newContextOut, V4V_ACCEPT_VALUES *acceptOut, OVERLAPPED *ov)
{
    if ((context == NULL)||(newContextOut == NULL)||(acceptOut == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return V4VDriverAccept(context, newContextOut, acceptOut, ov);
}

/* This routine is used to connect V4V channel. The newly connected V4V 
 * channel will be a stream connection. The @ringAddr argument specifies
 * the destination address to connect to.
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vConnect() operation can be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * connection is established.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION will
 * be returned if the file is not in the bound state following a call to
 * V4vBind().
 */
static V4V_INLINE_API BOOL
V4vConnect(void *context, v4v_addr_t *ringAddr, OVERLAPPED *ov)
{
    if ((context == NULL)||(ringAddr == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return V4VDriverConnect(context, ringAddr, ov);
}

/* This function provides an alternate means to make a stream connection.
 * The function will wait for an incoming connect from V4vConnect() and
 * establish a single stream channel. It is different from V4vListen() in
 * that it effectively listens and accepts only once. As the name implies,
 * the call (or IO pended) will block until the peer has connected.
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vListen() operation must be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * listen is complete.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION will
 * be returned if the file is not in the bound state following a call to
 * V4vBind().
 */
static V4V_INLINE_API BOOL
V4vConnectWait(void *context, OVERLAPPED *ov)
{
    if (context == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return V4VDriverConnectWait(context, ov);
}

/* This routine is used to disconnect V4V channel stream. The channel be a 
 * connected or accepted stream connection. The API can be used to perform
 * an orederly shutdown by explicity sending an RST before closing the
 * context with V4vClose(). 
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vConnect() operation can be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * connection is established.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION will
 * be returned if the file is not in the connected/accepted states.
 */
static V4V_INLINE_API BOOL
V4vDisconnect(void *context, OVERLAPPED *ov)
{
    if (context == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return V4VDriverDisconnect(context, ov);
}

/* Information can be gotten about the local or peer address. If 
 * V4vGetLocalInfo is specified, @ringInfoOut will contain the ring
 * information that the channel is locally bound with. If V4vGetPeerInfo
 * is specified then @ringInfoOut->addr will contain the remote peer
 * address information. V4vGetPeerInfo can only be used on V4V channels in the
 * connected or accepted states.
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vGetInfo() operation can be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * get info operation completes.
 *
 * For non-overlapped calls, the @ringInfoOut value will be filled in at the
 * end of the call. For overlapped calls, the caller must fetch the 
 * V4V_GETINFO_VALUES structure during GetOverlappedResult() or in the 
 * FileIOCompletionRoutine.
 *
 * The caller must suppy the @infoOut argument. Upon synchronous completion
 * of the call, this structure will have the @infoOut.ringInfo field filled
 * in (the other fields should be ignored). For overlapped calls, the caller 
 * retain the @infoOut structure until IO is completed (this is effectively
 * the output buffer for the IOCLT). During GetOverlappedResult() or in the 
 * FileIOCompletionRoutine the @infoOut.ringInfo value can be fetched and
 * @acceptOut released etc.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION will
 * be returned if the file is not in the proper state following a call to
 * get the information.
 */
static V4V_INLINE_API BOOL
V4vGetInfo(void *context, V4V_GETINFO_TYPE type, V4V_GETINFO_VALUES *infoOut, OVERLAPPED *ov)
{
    if ((context == NULL)||(infoOut == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return V4VDriverGetInfo(context, type, infoOut, ov);
}

/* This utility routine will dump the current state of the V4V ring to the
 * various driver trace targets (like KD, Xen etc).
 *
 * If the V4V file was opened with the V4V_FLAG_OVERLAPPED flag set then the
 * V4vGetInfo() operation can be done asynchronously using the @ov overlapped
 * value. Otherwise @ov should be NULL and accept call will block until the
 * get info operation completes.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError(). ERROR_INVALID_FUNCTION willg
 * be returned if the file is not in the proper state following a call to
 * dump the ring.
 */
static V4V_INLINE_API BOOL
V4vDumpRing(void *context, OVERLAPPED *ov)
{
    if (context == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return V4VDriverDumpRing(context, ov);
}

/* This routine should be used to close the @context handles returned from a
 * call to V4vOpen(). It can be called at any time to close the file handle
 * and terminate all outstanding IO.
 *
 * Returns TRUE on success or FALSE on error, in which case more information
 * can be obtained by calling GetLastError().
 */
static V4V_INLINE_API BOOL
V4vClose(void *context)
{
    V4VDriverClose(context);
    return TRUE;
}

#endif /* !__V4VAPI_H__ */
