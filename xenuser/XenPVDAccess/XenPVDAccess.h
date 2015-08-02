/*
 * Copyright (c) 2009 Citrix Systems, Inc.
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

#ifndef _XENPVDACCESS_H_
#define _XENPVDACCESS_H_

#include <windows.h>
#include <stdlib.h>
#include <setupapi.h>

#include <winioctl.h>
#include "xeniface_ioctls.h"

#ifdef XSPVDRIVERAPI_EXPORTS
#define XSPVDRIVER_API __declspec(dllexport)
#else
#define XSPVDRIVER_API __declspec(dllimport)
#endif

#if defined (__cplusplus)
extern "C" {
#endif
/* Open the xenstore interface.  The returned handle should be
 * released with XSPVDriver_close() when it is no longer needed.  It is
 * automatically closed when the application exits.
 *
 * Any given handle should only be used from the thread which
 * initially created it.
 *
 * Returns a new XSPVDriver_handle on success, or NULL on error.  On error,
 * the error code can be retrieved with GetLastError().
 */
XSPVDRIVER_API void *WINAPI XSPVDriver_open(void);

/* Close a handle to the xenstore interface.  The handle passed in
 * should have been previously returned by XSPVDriver_open().  After this
 * function has been called, the handle should not be used by any
 * other functions.
 *
 * If there is an open transaction, it will be aborted.  Any
 * outstanding watches will be cancelled.
 */
XSPVDRIVER_API void WINAPI XSPVDriver_close(void *handle);

/* Start a transaction.  @handle is the xenstore handle (previously
 * returned by XSPVDriver_open()) on which to start the transaction.  Only
 * one transaction can be open on any given handle at any time.
 *
 * Returns a hint as to whether the transaction is likely to succeed
 * or fail.  If this function returns FALSE, the transaction is likely
 * to fail.  GetLastError() can be called to give an indication of why
 * the transaction is likely to fail.
 *
 * One of XSPVDriver_transaction_commit() and XSPVDriver_transaction_abort() must
 * always be called to end the transaction, regardless of the return
 * value of this function.
 *
 * Transactions apply to all data operations (read, write, remove,
 * etc.), but not to non-data operations like watch and unwatch.
 */
XSPVDRIVER_API BOOL WINAPI XSPVDriver_transaction_start(void *handle);

/* Abort a transaction.  All operations made in the transaction will
 * be discarded.  There must be a transaction open against the handle
 * when this is called, and the transaction will be closed.
 */
XSPVDRIVER_API void WINAPI XSPVDriver_transaction_abort(void *handle);

/* Commit a transaction.  All operations made in the transaction will
 * be made visible to other VMs.  There must be a transaction open
 * against the handle when this is called, and the transaction will be
 * closed (even if there's an error committing it).
 *
 * This can fail due to conflicts with updates made by other VMs.  In
 * that case, this function will return FALSE and GetLastError() will
 * return ERROR_RETRY.  The caller should check that whatever it was
 * trying to do is still valid and retry.
 *
 * Returns FALSE on error and TRUE on success.  On error,
 * GetLastError() can be used to obtain the error code.
 *
 * If any operations in the transaction encountered an error, the
 * transaction will fail to commit, and GetLastError() will report the
 * error which was encountered.
 */
XSPVDRIVER_API BOOL WINAPI XSPVDriver_transaction_commit(void *handle);

/* Write a nul-terminated string @data to the store at location @path.
 * @handle should be an XSPVDriver_handle which was previously openned with
 * XSPVDriver_open().
 *
 * If a transaction is currently open against the handle, the write
 * operation is bundled into that transaction.
 *
 * Returns TRUE on success, or FALSE on error.  On error, the error
 * code can be retrieved by calling GetLastError().
 */
XSPVDRIVER_API BOOL WINAPI XSPVDriver_write(void *handle, const char *path,
                                     const char *data);

/* Get the contents of a directory @path in the store, using the XSPVDriver
 * handle @handle.
 *
 * @num may be NULL.  If it is not NULL, *@num is set to the number of
 * entries in the directory, or 0 on error.
 *
 * Returns a newly-allocated NULL-terminated array of pointers to
 * newly-allocated nul-terminated strings.  These values can be
 * released with FreeSpace().  The entries must be released
 * independently of the main table.  The NULL terminator in the main
 * array is not counted towards the count of directory entries in
 * *@num.
 *
 * If a transaction is currently open against the handle, the
 * directory operation is bundled into that transaction.
 *
 * Returns NULL on error.  In that case, a more specific error code
 * can be retrieved with GetLastError().
 */
XSPVDRIVER_API int WINAPI XSPVDriver_directory(void *handle,
                                        const char *path,
                                        size_t bufferByteCount,
                                        char *returnBuffer);

/* Read the contents of a node @path in the store, using open handle
 * @handle.
 *
 * Returns a newly-allocated buffer containing the contents of the
 * node.  A nul-terminator is automatically added to the end of the
 * returned buffer; this is not included in the size of the value.
 *
 * @len may be NULL.  If it is not NULL, it *@len is set to the size
 * of the returned string, or 0 on error.
 *
 * If a transaction is currently open against the handle, the read
 * operation is bundled into that transaction.
 *
 * Returns NULL on error.  GetLastError() will return the error code.
 */
XSPVDRIVER_API BOOL WINAPI XSPVDriver_read(void *handle, const char *path, size_t len, char *returnBuffer);

/* Remove the node @path from the store, using open handle @handle.
 *
 * If a transaction is currently open against the handle, the remove
 * operation is bundled into that transaction.
 *
 * Returns TRUE on success, or FALSE on error.  On error,
 * GetLastError() will return the error code.
 */
XSPVDRIVER_API BOOL WINAPI XSPVDriver_remove(void *handle, const char *path);
/* Create a watch on a xenstore node @path using open handle @handle.
 * The event @event will be notified shortly after the contents of the
 * node changes.  There are no other guarantees on the behaviour of
 * this function.  In particular:
 *
 * -- The event may be notified when the path hasn't actually changed
 * -- The event may be notified several times for a single change
 * -- The event may only be notified once if the node changes several
 *    times in rapid succession.
 *
 * The returned watch handler should be released with UnWatch()
 * when it is no longer needed.
 *
 * Returns a new watch structure on success, or NULL on error.  On
 * error, GetLastError() will return the error code.
 *
 * If the handle is closed before the watch is released, the watch
 * will no longer function, but it must still be released with
 * UnWatch().
 */
XSPVDRIVER_API void *WINAPI XSPVDriver_watch(void *handle,
                                      const char *path,
                                      HANDLE event);

/* Release the watch @watch which was previously created with
 * Watch().  This always succeeds (provided @watch is valid).
 */
XSPVDRIVER_API void WINAPI XSPVDriver_unwatch(void *watch);

/* Release memory allocated by the XSPVDriver library.  The semantics are
 * similar to the C standard library free() function: the memory at
 * @mem must have been previously allocated, and not already freed,
 * and once FreeSpace() has been called any access to @mem is invalid.
 */
XSPVDRIVER_API void WINAPI XSPVDriver_free(const void *mem);

#if defined (__cplusplus)
};

#include <stdarg.h>
#include <stdio.h>

class cFunctionSentinel {
public:
    cFunctionSentinel(const char *aFunctionName)
        : FunctionName(aFunctionName) {
        OutputDebugStringA(FunctionName);
        OutputDebugStringA(": ENTRY\n");
    }

    ~cFunctionSentinel() {
        OutputDebugStringA(FunctionName);
        OutputDebugStringA(": EXIT\n");
    }

    void PostMessage(const char *aFormatString, ...) {
        OutputDebugStringA(FunctionName);
        OutputDebugStringA(": ");
        char outputBuffer[1024];
        outputBuffer[0] = 0;

        va_list args;
        va_start(args, aFormatString);
        if (0 < _vsnprintf_s(outputBuffer, sizeof(outputBuffer), sizeof(outputBuffer), aFormatString, args)) {
            OutputDebugStringA((PSTR)outputBuffer);
        }
    }

protected:
    const char *FunctionName;
};

#define dCreateSentinel(aName) cFunctionSentinel aName(__FUNCTION__); (void)aName

class cDriverWatch;

class cXenPVDAccess {
public:
    static cXenPVDAccess *CreateXenPVDAccess() {
        return new cXenPVDAccess;
    }
    static void DestroyXenPVDAccess(void *aAccess) {
        delete (cXenPVDAccess *)aAccess;
    }

    cXenPVDAccess(int aAttributeFlags = 0) :
        PVDriverDevice(INVALID_HANDLE_VALUE),
        ValidIOCTLInterface(false) {
        dCreateSentinel(sentinel);

        // Setup the IOCTL interface.
        HDEVINFO devInfo = SetupDiGetClassDevs(&GUID_INTERFACE_XENIFACE, 0, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (INVALID_HANDLE_VALUE == devInfo) {
            int errorCode = GetLastError();
            sentinel.PostMessage("SetupDiGetClassDevs Returned an invalid handle. Error: %d", errorCode);
            SetLastError(errorCode);
            return;
        }

        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
        deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
        if (!SetupDiEnumDeviceInterfaces(devInfo, NULL, &GUID_INTERFACE_XENIFACE, 0, &deviceInterfaceData)) {
            int errorCode = GetLastError();
            sentinel.PostMessage("SetupDiEnumDeviceInterfaces Returned Error: %d", errorCode);
            SetLastError(errorCode);
            return;
        }

        // Follow MS defined proceedure to find the object size for the device interface detail.
        DWORD dataByteCount;
        SetupDiGetDeviceInterfaceDetail(devInfo, &deviceInterfaceData, NULL, 0, &dataByteCount, NULL);
        const unsigned errorCode = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != errorCode) {
            sentinel.PostMessage("SetupDiGetDeviceInterfaceDetail Returned unexpected Error: %d", errorCode);
            SetLastError(errorCode);
            return;
        }

        SP_DEVICE_INTERFACE_DETAIL_DATA *deviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA *)new char[dataByteCount];
        if (!deviceInterfaceDetailData) {
            sentinel.PostMessage("Out of memory. File: %s, Line #: %d", __FILE__, __LINE__);
            SetLastError(ERROR_OUTOFMEMORY);
            return;
        }

        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(devInfo, &deviceInterfaceData, deviceInterfaceDetailData, dataByteCount, NULL, NULL)) {
            delete[] deviceInterfaceDetailData;
            int errorCode = GetLastError();
            sentinel.PostMessage("SetupDiGetDeviceInterfaceDetail Returned Error: %d", errorCode);
            SetLastError(errorCode);
            return;
        }

        PVDriverDevice = CreateFile(deviceInterfaceDetailData->DevicePath,
                                    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | aAttributeFlags,
                                    NULL);

        delete[] deviceInterfaceDetailData;
        if (PVDriverDevice == INVALID_HANDLE_VALUE) {
            int errorCode = GetLastError();
            sentinel.PostMessage("CreateFile Returned Error: %d", errorCode);
            SetLastError(errorCode);
            return;
        }

        ValidIOCTLInterface = true;
    }

    ~cXenPVDAccess() {
        dCreateSentinel(sentinel);
        if (INVALID_HANDLE_VALUE != PVDriverDevice) {
            CloseHandle(PVDriverDevice);
        }
    }


    HANDLE GetHandle() {
        return PVDriverDevice;
    }

    int EvtchnBindUnboundPort(unsigned short remoteDomain,
                              HANDLE event,
                              bool mask,
                              unsigned *localPort);

    int EvtchnBindInterdomain(unsigned short remoteDomain,
                              unsigned remotePort,
                              HANDLE event,
                              bool mask,
                              unsigned *localPort);

    int EvtchnClose(unsigned localPort);

    int EvtchnNotify(unsigned localPort);

    int EvtchnUnmask(unsigned localPort);

    int EvtchnStatus(unsigned localPort,
                     unsigned *status);

    int GnttabGrantPages(unsigned short remoteDomain,
                         unsigned numberPages,
                         unsigned notifyOffset,
                         unsigned notifyPort,
                         GNTTAB_GRANT_PAGES_FLAGS flags,
                         void **handle,
                         void **address,
                         unsigned *references);

    int GnttabUngrantPages(void *handle);

    int GnttabMapForeignPages(unsigned short remoteDomain,
                              unsigned numberPages,
                              unsigned *references,
                              unsigned notifyOffset,
                              unsigned notifyPort,
                              GNTTAB_GRANT_PAGES_FLAGS flags,
                              void **handle,
                              void **address);

    int GnttabUnmapForeignPages(void *handle);

    int StoreRead(const char *path,
                  int cbOutput,
                  char *output);

    int StoreWrite(const char *path,
                   const char *value);

    int StoreDirectory(const char *path,
                       int cbOutput,
                       char *output);

    int StoreRemove(const char *path);

    int StoreSetPermissions(const char *path,
                            unsigned count,
                            PXENBUS_STORE_PERMISSION permissions);

    int StoreAddWatch(const char *path,
                      HANDLE event,
                      void **handle);

    int StoreRemoveWatch(void *handle);

private:
    //    void DropHandleReference();
    //    int ListenSuspend(HANDLE event);
    //    bool UnListenSuspend();
    //    bool PVDriverIOCTL(int dwCtrlCode, const void *pInBuffer,
    //                       int InBufferSize, void *pOutBuffer,
    //                       int *pOutBufferSize);
    //    bool TransactionEnd(bool fAbort);

    //    unsigned RefCount;
    HANDLE PVDriverDevice;
    //    bool HaveTransaction;
    bool ValidIOCTLInterface;

    int TransactionStatus;
};

class cDriverWatch {
public:
    cDriverWatch(cXenPVDAccess *aXenPVDAccess, const char *path, HANDLE event) :
        XenPVDriverAccess(aXenPVDAccess),
        WatchHandle(0) {
        XenPVDriverAccess->StoreAddWatch(path, event, &WatchHandle);
    }

    ~cDriverWatch() {
        XenPVDriverAccess->StoreRemoveWatch(WatchHandle);
    }

protected:
    cXenPVDAccess *XenPVDriverAccess;
    void *WatchHandle;
};

#endif  // _cplusplus

#endif // _XENPVDACCESS_H_
