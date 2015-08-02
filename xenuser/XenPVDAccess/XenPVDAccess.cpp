/*
 * Copyright (c) 2010 Citrix Systems, Inc.
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

#include <initguid.h>
#define XSPVDRIVERAPI_EXPORTS
#include "XenPVDAccess.h"

#include <xs_private.h>

int cXenPVDAccess::EvtchnBindUnboundPort(unsigned short remoteDomain,
                                         HANDLE event,
                                         bool mask,
                                         unsigned *localPort) {
    dCreateSentinel(sentinel);
    EVTCHN_BIND_UNBOUND_PORT_IN in;
    in.RemoteDomain = remoteDomain;
    in.Event = event;
    in.Mask = !!mask;

    DWORD returned;
    EVTCHN_BIND_UNBOUND_PORT_OUT out;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_EVTCHN_BIND_UNBOUND_PORT,
                             &in, sizeof(in),
                             &out, sizeof(out),
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    *localPort = out.LocalPort;
    return ERROR_SUCCESS;
}

int cXenPVDAccess::EvtchnBindInterdomain(unsigned short remoteDomain,
                                         unsigned remotePort,
                                         HANDLE event,
                                         bool mask,
                                         unsigned *localPort) {
    dCreateSentinel(sentinel);
    EVTCHN_BIND_INTERDOMAIN_IN in;
    in.RemoteDomain = remoteDomain;
    in.RemotePort = remotePort;
    in.Event = event;
    in.Mask = !!mask;

    EVTCHN_BIND_INTERDOMAIN_OUT out;
    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_EVTCHN_BIND_INTERDOMAIN,
                             &in, sizeof(in),
                             &out, sizeof(out),
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    *localPort = out.LocalPort;
    return ERROR_SUCCESS;
}

int cXenPVDAccess::EvtchnClose(unsigned localPort) {
    dCreateSentinel(sentinel);
    EVTCHN_CLOSE_IN in;
    in.LocalPort = localPort;

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_EVTCHN_CLOSE,
                             &in, sizeof(in),
                             NULL, 0,
                             &returned,
                             NULL))     {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;
}

int cXenPVDAccess::EvtchnNotify(unsigned localPort) {
    dCreateSentinel(sentinel);
    EVTCHN_NOTIFY_IN in;
    in.LocalPort = localPort;

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_EVTCHN_NOTIFY,
                             &in, sizeof(in),
                             NULL, 0,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;
}

int cXenPVDAccess::EvtchnUnmask(unsigned localPort) {
    dCreateSentinel(sentinel);
    EVTCHN_UNMASK_IN in;
    in.LocalPort = localPort;

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_EVTCHN_UNMASK,
                             &in, sizeof(in),
                             NULL, 0,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;
}

int cXenPVDAccess::EvtchnStatus(unsigned localPort,
                                unsigned *status) {
    dCreateSentinel(sentinel);

    EVTCHN_STATUS_IN in;
    in.LocalPort = localPort;

    DWORD returned;
    EVTCHN_STATUS_OUT out;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_EVTCHN_STATUS,
                             &in, sizeof(in),
                             &out, sizeof(out),
                             &returned,
                             NULL)) {

        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    *status = out.Status;

    return ERROR_SUCCESS;
}

int cXenPVDAccess::GnttabGrantPages(unsigned short remoteDomain,
                                    unsigned numberPages,
                                    unsigned notifyOffset,
                                    unsigned notifyPort,
                                    GNTTAB_GRANT_PAGES_FLAGS flags,
                                    void **handle,
                                    void **address,
                                    unsigned *references) {
    dCreateSentinel(sentinel);
    GNTTAB_GRANT_PAGES_IN in;
    in.RemoteDomain = remoteDomain;
    in.NumberPages = numberPages;
    in.NotifyOffset = notifyOffset;
    in.NotifyPort = notifyPort;
    in.Flags = flags;

    const int size = sizeof(GNTTAB_GRANT_PAGES_OUT) + numberPages * sizeof(unsigned);
    GNTTAB_GRANT_PAGES_OUT *out = (GNTTAB_GRANT_PAGES_OUT *)new char[size];
    if (!out) {
        SetLastError(ERROR_OUTOFMEMORY);
        sentinel.PostMessage("Out of memory. File: %s, Line #: %d", __FILE__, __LINE__);
        return ERROR_OUTOFMEMORY;
    }

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_GNTTAB_GRANT_PAGES,
                             &in, sizeof(in),
                             out, size,
                             &returned,
                             NULL)) {
        delete[] out;
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    *address = out->Address;
    *handle = out->Context;
    memcpy(references, &out->References, numberPages * sizeof(unsigned));

    delete[] out;
    return ERROR_SUCCESS;
}

int cXenPVDAccess::GnttabUngrantPages(void *handle) {
    dCreateSentinel(sentinel);
    GNTTAB_UNGRANT_PAGES_IN in;
    in.Context = handle;

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_GNTTAB_UNGRANT_PAGES,
                             &in, sizeof(in),
                             NULL, 0,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;
}

int cXenPVDAccess::GnttabMapForeignPages(unsigned short remoteDomain,
                                         unsigned numberPages,
                                         unsigned *references,
                                         unsigned notifyOffset,
                                         unsigned notifyPort,
                                         GNTTAB_GRANT_PAGES_FLAGS flags,
                                         void **handle,
                                         void **address) {
    dCreateSentinel(sentinel);

    int size = sizeof(GNTTAB_MAP_FOREIGN_PAGES_IN) + numberPages * sizeof(unsigned);
    GNTTAB_MAP_FOREIGN_PAGES_IN *in = (GNTTAB_MAP_FOREIGN_PAGES_IN *)new char[size];

    if (!in) {
        SetLastError(ERROR_OUTOFMEMORY);
        sentinel.PostMessage("Out of memory. File: %s, Line #: %d", __FILE__, __LINE__);
        return ERROR_OUTOFMEMORY;
    }

    in->RemoteDomain = remoteDomain;
    in->NumberPages = numberPages;
    in->NotifyOffset = notifyOffset;
    in->NotifyPort = notifyPort;
    in->Flags = flags;
    memcpy(&in->References, references, numberPages * sizeof(unsigned));

    DWORD returned;
    GNTTAB_MAP_FOREIGN_PAGES_OUT out;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_GNTTAB_MAP_FOREIGN_PAGES,
                             in, size,
                             &out, sizeof(out),
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    *address = out.Address;
    *handle = out.Context;

    delete[] in;
    return ERROR_SUCCESS;
}

int cXenPVDAccess::GnttabUnmapForeignPages(void *handle) {
    dCreateSentinel(sentinel);
    GNTTAB_UNMAP_FOREIGN_PAGES_IN in;
    in.Context = handle;

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_GNTTAB_UNMAP_FOREIGN_PAGES,
                             &in, sizeof(in),
                             NULL, 0,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }
    return ERROR_SUCCESS;
}

int cXenPVDAccess::StoreRead(const char *path,
                             int cbOutput,
                             char *output) {
    dCreateSentinel(sentinel);
    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_STORE_READ,
                             (LPVOID)path, (DWORD)strlen(path) + 1,
                             output, cbOutput,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }
    return ERROR_SUCCESS;
}

int cXenPVDAccess::StoreWrite(const char *path,
                              const char *value) {
    dCreateSentinel(sentinel);
    const int bufferByteCount = (int)(strlen(path) + 1 + strlen(value) + 1 + 1);
    char *buffer = new char[bufferByteCount];
    if (!buffer) {
        SetLastError(ERROR_OUTOFMEMORY);
        sentinel.PostMessage("Out of memory. File: %s, Line #: %d", __FILE__, __LINE__);
        return ERROR_OUTOFMEMORY;
    }

    ZeroMemory(buffer, bufferByteCount);
    memcpy(buffer, path, strlen(path));
    memcpy(buffer + strlen(path) + 1, value, strlen(value));

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_STORE_WRITE,
                             buffer, bufferByteCount,
                             NULL, 0,
                             &returned,
                             NULL)) {
        delete[] buffer;
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    delete[] buffer;
    return ERROR_SUCCESS;
}

int cXenPVDAccess::StoreDirectory(const char *path,
                                  int cbOutput,
                                  char *output) {
    dCreateSentinel(sentinel);
    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_STORE_DIRECTORY,
                             (LPVOID)path, (DWORD)strlen(path) + 1,
                             output, cbOutput,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;

}

int cXenPVDAccess::StoreRemove(const char *path) {
    dCreateSentinel(sentinel);
    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_STORE_REMOVE,
                             (LPVOID)path, (DWORD)strlen(path) + 1,
                             NULL, 0,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;
}

int cXenPVDAccess::StoreSetPermissions(const char *path,
                                       unsigned count,
                                       PXENBUS_STORE_PERMISSION permissions) {
    dCreateSentinel(sentinel);
    int size = sizeof(STORE_SET_PERMISSIONS_IN) + count * sizeof(XENBUS_STORE_PERMISSION);

    STORE_SET_PERMISSIONS_IN *in = (STORE_SET_PERMISSIONS_IN *)new char[size];
    if (!in) {
        SetLastError(ERROR_OUTOFMEMORY);
        sentinel.PostMessage("Out of memory. File: %s, Line #: %d", __FILE__, __LINE__);
        return ERROR_OUTOFMEMORY;
    }

    in->Path = (PCHAR)path;
    in->PathLength = (DWORD)strlen(in->Path) + 1;
    in->NumberPermissions = count;
    memcpy(&in->Permissions, permissions, count * sizeof(XENBUS_STORE_PERMISSION));

    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_STORE_SET_PERMISSIONS,
                             in, size,
                             NULL, 0,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    delete[] in;
    return ERROR_SUCCESS;
}

int cXenPVDAccess::StoreAddWatch(const char *path,
                                 HANDLE event,
                                 void **handle) {
    dCreateSentinel(sentinel);
    STORE_ADD_WATCH_IN in;
    in.Path = (PCHAR)path;
    in.PathLength = (DWORD)strlen(path) + 1;
    in.Event = event;

    STORE_ADD_WATCH_OUT out;
    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_STORE_ADD_WATCH,
                             &in, sizeof(in),
                             &out, sizeof(out),
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    *handle = out.Context;

    return ERROR_SUCCESS;
}


int cXenPVDAccess::StoreRemoveWatch(void *handle) {
    dCreateSentinel(sentinel);
    STORE_REMOVE_WATCH_IN in;
    in.Context = handle;
    DWORD returned;
    if (0 == DeviceIoControl(PVDriverDevice,
                             IOCTL_XENIFACE_STORE_REMOVE_WATCH,
                             &in, sizeof(in),
                             NULL, 0,
                             &returned,
                             NULL)) {
        int errorCode = GetLastError();
        sentinel.PostMessage("DeviceIOControl Returned Error: %d", errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;
}


extern "C" {

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
void *XSPVDriver_open(void) {
    return cXenPVDAccess::CreateXenPVDAccess();
}

/* Close a handle to the xenstore interface.  The handle passed in
 * should have been previously returned by XSPVDriver_open().  After this
 * function has been called, the handle should not be used by any
 * other functions.
 *
 * If there is an open transaction, it will be aborted.  Any
 * outstanding watches will be cancelled.
 */
void XSPVDriver_close(void *handle) {
    cXenPVDAccess::DestroyXenPVDAccess(handle);
}

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
BOOL XSPVDriver_write(void *handle, const char *path,
                      const char *data) {
    return ((cXenPVDAccess *)handle)->StoreWrite(path, data) ? 1 : 0;
}

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
int WINAPI XSPVDriver_directory(void *handle,
                                const char *path,
                                size_t bufferByteCount,
                                char *returnBuffer) {
    return ((cXenPVDAccess *)handle)->StoreDirectory(path, (int)bufferByteCount, returnBuffer);
}

/* Read the contents of a node @path in the store, using open handle
 * @handle.
 *
 */
BOOL WINAPI XSPVDriver_read(void *handle, const char *path, size_t len, char *returnBuffer) {
    return ((cXenPVDAccess *)handle)->StoreRead(path, (int)len, returnBuffer);
}

/* Remove the node @path from the store, using open handle @handle.
 *
 * If a transaction is currently open against the handle, the remove
 * operation is bundled into that transaction.
 *
 * Returns TRUE on success, or FALSE on error.  On error,
 * GetLastError() will return the error code.
 */
BOOL XSPVDriver_remove(void *handle, const char *path) {
    return ((cXenPVDAccess *)handle)->StoreRemove(path) ? 1 : 0;
}

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
void *XSPVDriver_watch(void *handle,
                       const char *path,
                       HANDLE event) {
    cXenPVDAccess *xenPVDAccess = (cXenPVDAccess *)handle;
    cDriverWatch *driverWatch = new cDriverWatch(xenPVDAccess, path, event);
    return driverWatch;
}

/* Release the watch @watch which was previously created with
 * Watch().  This always succeeds (provided @watch is valid).
 */
void XSPVDriver_unwatch(void *watch) {
    cDriverWatch *driverWatch = (cDriverWatch *)watch;
    delete driverWatch;
}

/* Release memory allocated by the XSPVDriver library.  The semantics are
 * similar to the C standard library free() function: the memory at
 * @mem must have been previously allocated, and not already freed,
 * and once FreeSpace() has been called any access to @mem is invalid.
 */
void XSPVDriver_free(const void *mem) {
    (void)mem;
}

BOOL XSPVDriver_transaction_start(void *handle) {
    (void)handle;
    return TRUE;
}

void XSPVDriver_transaction_abort(void *handle) {
    (void)handle;
    return;
}

BOOL XSPVDriver_transaction_commit(void *handle) {
    (void)handle;
    return TRUE;
}

WRITE_ON_CLOSE_HANDLE
XSPVDriver_write_on_close(void *xih,
                          const char *path,
                          const void *data,
                          size_t data_size)
{
    (void)xih, path, data, data_size;
    WRITE_ON_CLOSE_HANDLE handle;
    handle.__h = 0;
    return handle;
}

void
XSPVDriver_cancel_write_on_close(void *xih,
                                 WRITE_ON_CLOSE_HANDLE handle)
{
    (void) handle, xih;
    return;
}
};
