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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma warning(push, 3)
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <malloc.h>
#pragma warning(pop)

#define XSAPI_EXPORTS
#include "xs.h"
#include "xs_private.h"

/* Don't include XenPVDAccessor.h.  We're not allowed to directly call any
   functions which it exports, because of the stupid Windows linking
   rules. */

struct XSPVDriver_handle;

struct xs_watch {
    struct xs_watch *next, *prev;
    int xs_handle;
    struct XSPVDriver_watch *XSPVDriver_watch;
};

struct XSPVDriver_interface {
    HMODULE module;
    struct XSPVDriver_handle *(WINAPI *XSPVDriver_open)(void);
    void (WINAPI *XSPVDriver_close)(struct XSPVDriver_handle *h);
    BOOL (WINAPI *XSPVDriver_transaction_start)(struct XSPVDriver_handle *h);
    void (WINAPI *XSPVDriver_transaction_abort)(struct XSPVDriver_handle *handle);
    BOOL (WINAPI *XSPVDriver_transaction_commit)(struct XSPVDriver_handle *handle);
    void *(WINAPI *XSPVDriver_read)(HANDLE hXS, const char *path, size_t *len);
    BOOL (WINAPI *XSPVDriver_write)(struct XSPVDriver_handle *handle, const char *path,
                      const char *data);
    BOOL (WINAPI *XSPVDriver_write_bin)(struct XSPVDriver_handle *handle, const char *path,
                                  const void *data, size_t size);
    char **(WINAPI *XSPVDriver_directory)(struct XSPVDriver_handle *handle,
                                    const char *path,
                                    unsigned int *num);
    struct XSPVDriver_watch *(WINAPI *XSPVDriver_watch)(struct XSPVDriver_handle *handle,
                                           const char *path,
                                           HANDLE event);
    void (WINAPI *XSPVDriver_unwatch)(struct XSPVDriver_watch *watch);
    BOOL (WINAPI *XSPVDriver_remove)(HANDLE hXS, const char *path);
    void (WINAPI *XSPVDriver_free)(const void *mem);
};

static HANDLE mainLock;
static int nextWatchHandle;
static struct xs_watch *head_watch;
static struct XSPVDriver_interface *volatile XSPVDriver_interface;

static struct XSPVDriver_interface *get_XSPVDriver_interface(void)
{
    struct XSPVDriver_interface *work;
    LONG r;
    HKEY reg;
    DWORD type;
    DWORD sz;
    unsigned char *path;
    HMODULE module;
    DWORD err;

    if (XSPVDriver_interface)
        return XSPVDriver_interface;
    r = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "SOFTWARE\\Citrix\\XenTools",
                     0,
                     KEY_QUERY_VALUE,
                     &reg);
    if (r != ERROR_SUCCESS) {
        SetLastError(r);
        return NULL;
    }
    sz = 0;
    r = RegQueryValueEx(reg, "XSPVDriver.dll", NULL, &type, NULL, &sz);
    if (r != ERROR_SUCCESS) {
        RegCloseKey(reg);
        SetLastError(r);
        return NULL;
    }
    if (type != REG_SZ) {
        RegCloseKey(reg);
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    path = malloc(sz + 1);
    if (!path) {
        RegCloseKey(reg);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    r = RegQueryValueEx(reg, "XSPVDriver.dll", NULL, &type, path, &sz);
    if (r != ERROR_SUCCESS) {
        RegCloseKey(reg);
        free(path);
        SetLastError(r);
        return NULL;
    }
    path[sz] = 0;
    RegCloseKey(reg);

    module = LoadLibrary((char *)path);
    if (!module) {
        free(path);
        return NULL;
    }

    work = malloc(sizeof(*work));
    if (!work) {
        FreeLibrary(module);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    work->module = module;
#pragma warning(push)
#pragma warning(disable:4054) /* typecast FARPROC to void */
#pragma warning(disable:4152) /* reinterpret void * as arbitrary
                               * function pointer */
#ifdef AMD64
#define DECORATE(n, s) #n
#else
#define DECORATE(n, s) "_" #n "@" #s
#endif
    work->XSPVDriver_open = (void *)GetProcAddress(module, DECORATE(XSPVDriver_open, 0));
    if (!work->XSPVDriver_open) {
    err:
        err = GetLastError();
        FreeLibrary(module);
        free(work);
        SetLastError(err);
        return NULL;
    }
#define f(name, s)                                                       \
    work->name = (void *)GetProcAddress(module, DECORATE(name, s) );     \
    if (!work->name) {                                                   \
        goto err;                                                        \
    }
    f(XSPVDriver_close, 4);
    f(XSPVDriver_transaction_start, 4);
    f(XSPVDriver_transaction_abort, 4);
    f(XSPVDriver_transaction_commit, 4);
    f(XSPVDriver_read, 12);
    f(XSPVDriver_write, 12);
    f(XSPVDriver_write_bin, 16);
    f(XSPVDriver_directory, 12);
    f(XSPVDriver_watch, 12);
    f(XSPVDriver_unwatch, 4);
    f(XSPVDriver_remove, 8);
    f(XSPVDriver_free, 4);
#undef f
#undef DECORATE
#pragma warning(pop)

    if (InterlockedCompareExchangePointer(&XSPVDriver_interface, work, NULL)) {
        FreeLibrary(module);
        free(work);
        return XSPVDriver_interface;
    } else {
        return work;
    }
}

HANDLE xs_domain_open()
{
    struct XSPVDriver_interface *i = get_XSPVDriver_interface();
    if (!i)
        return NULL;
    else
        return i->XSPVDriver_open();
}

void xs_daemon_close(HANDLE _xih)
{
    XSPVDriver_interface->XSPVDriver_close(_xih);
}

BOOL xs_transaction_start(HANDLE _xih)
{
    return XSPVDriver_interface->XSPVDriver_transaction_start(_xih);
}

BOOL xs_transaction_end(HANDLE _xih, BOOL fAbort)
{
    if (fAbort) {
        XSPVDriver_interface->XSPVDriver_transaction_abort(_xih);
        return TRUE;
    } else {
        return XSPVDriver_interface->XSPVDriver_transaction_commit(_xih);
    }
}

/* Some of these look a bit funny, because we need to move things from
   the XSPVDriver heap into the CRT heap. */
void *xs_read(HANDLE _xih, const char *path, size_t *len)
{
    void *res;
    void *res2;
    size_t len2;

    if (len)
        *len = 0;
    res = XSPVDriver_interface->XSPVDriver_read(_xih, path, &len2);
    if (res) {
        res2 = malloc(len2 + 1);
        if (!res2) {
            XSPVDriver_interface->XSPVDriver_free(res);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        memcpy(res2, res, len2);
        ((char *)res2)[len2] = 0;
        XSPVDriver_interface->XSPVDriver_free(res);
        if (len)
            *len = len2;
        return res2;
    } else {
        return NULL;
    }
}

BOOL xs_remove( HANDLE _xih, const char *path)
{
    return XSPVDriver_interface->XSPVDriver_remove(_xih, path);
}

BOOL xs_write_bin(HANDLE _xih, const char *path, const void *data,
                  size_t data_len)
{
    return XSPVDriver_interface->XSPVDriver_write_bin(_xih, path, data, data_len);
}

BOOL xs_write(HANDLE xih, const char *path, const char *data )
{
    return XSPVDriver_interface->XSPVDriver_write(xih, path, data);
}

char **xs_directory(HANDLE _xih, const char *path, unsigned int *num)
{
    char **XSPVDriver_res;
    char **crt_res;
    unsigned XSPVDriver_num;
    unsigned x;

    if (num)
        *num = 0;

    XSPVDriver_res = XSPVDriver_interface->XSPVDriver_directory(_xih, path, &XSPVDriver_num);
    if (!XSPVDriver_res)
        return NULL;

    crt_res = calloc(XSPVDriver_num, sizeof(crt_res[0]));
    if (!crt_res)
        goto no_memory;
    for (x = 0; x < XSPVDriver_num; x++) {
        crt_res[x] = malloc(strlen(XSPVDriver_res[x]) + 1);
        if (!crt_res[x])
            goto no_memory;
        strcpy(crt_res[x], XSPVDriver_res[x]);
        XSPVDriver_interface->XSPVDriver_free(XSPVDriver_res[x]);
        XSPVDriver_res[x] = NULL;
    }
    XSPVDriver_interface->XSPVDriver_free(XSPVDriver_res);
    if (num)
        *num = XSPVDriver_num;
    return crt_res;

no_memory:
    for (x = 0; x < XSPVDriver_num; x++) {
        xs_free(crt_res[x]);
        XSPVDriver_interface->XSPVDriver_free(XSPVDriver_res[x]);
    }
    xs_free(crt_res);
    XSPVDriver_interface->XSPVDriver_free(XSPVDriver_res);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
}

int xs_watch(HANDLE _xih, const char *path, HANDLE event)
{
    struct xs_watch *work;
    struct xs_watch *other_watch;
    DWORD err;
    int candidateId;
    BOOL foundId;

    work = calloc(sizeof(*work), 1);
    if (work == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    work->XSPVDriver_watch = XSPVDriver_interface->XSPVDriver_watch(_xih, path, event);
    if (work->XSPVDriver_watch == NULL) {
        err = GetLastError();
        free(work);
        SetLastError(err);
        return -1;
    }

    WaitForSingleObject(mainLock, INFINITE);

    /* Find an ID for the watch.  Handling the case where watch ids
       wrap over 2 billion is really paranoia, because no realistic
       program is going to create that many watches over its life, but
       handle it anyway. */
    candidateId = nextWatchHandle;
    foundId = FALSE;
    while (!foundId) {
        foundId = TRUE;
        for (other_watch = head_watch;
             other_watch != NULL;
             other_watch = other_watch->next) {
            if (other_watch->xs_handle == candidateId) {
                foundId = FALSE;
                candidateId++;
                if (candidateId < 0)
                    candidateId = 0;
                if (candidateId == nextWatchHandle) {
                    /* Wow... we've not only created 2 billion over
                       the application's lifetime, we've got 2 billion
                       outstanding *at the same time*. */
                    ReleaseMutex(mainLock);
                    XSPVDriver_interface->XSPVDriver_unwatch(work->XSPVDriver_watch);
                    free(work);
                    /* For want of any better error codes. */
                    SetLastError(ERROR_TOO_MANY_OPEN_FILES);
                    return -1;
                }
                break;
            }
        }
    }

    work->xs_handle = candidateId;
    work->next = head_watch;
    work->prev = NULL;
    if (work->next)
        work->next->prev = work;
    head_watch = work;

    nextWatchHandle = candidateId + 1;
    if (nextWatchHandle < 0)
        nextWatchHandle = 0;
    ReleaseMutex(mainLock);

    return work->xs_handle;
}

BOOL xs_unwatch(HANDLE _xih, int handle)
{
    struct xs_watch *watch;
    BOOL res;

    UNREFERENCED_PARAMETER(_xih);

    if (WaitForSingleObject(mainLock, INFINITE) != WAIT_OBJECT_0)
        return FALSE;
    for (watch = head_watch;
         watch != NULL && watch->xs_handle != handle;
         watch = watch->next)
        ;
    if (watch != NULL) {
        if (watch->prev)
            watch->prev->next = watch->next;
        else
            head_watch = watch->next;
        if (watch->next)
            watch->next->prev = watch->prev;
    }
    ReleaseMutex(mainLock);
    if (watch) {
        XSPVDriver_interface->XSPVDriver_unwatch(watch->XSPVDriver_watch);
        free(watch);
        res = TRUE;
    } else {
        SetLastError(ERROR_INVALID_PARAMETER);
        res = FALSE;
    }
    return res;
}

XS_API VOID xs_free(void *data)
{
    free(data);
}

BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(hinst);
    UNREFERENCED_PARAMETER(reserved);

    if (reason == DLL_PROCESS_ATTACH) {
        mainLock = CreateMutex(NULL, FALSE, NULL);
        if (mainLock == NULL)
            return FALSE;
    } else if (reason == DLL_PROCESS_DETACH) {
        if (mainLock != NULL) {
            CloseHandle(mainLock);
            mainLock = NULL;
        }
    }
    return TRUE;
}
