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

#ifndef V2V_PRIVATE_H__
#define V2V_PRIVATE_H__

struct XSPVDriver_handle;

void *ymalloc(size_t size);
char *ystrdup(const char *str);
void yfree(const void *ptr);

BOOL xenstore_readv_string(struct XSPVDriver_handle *XSPVDriver, size_t bufferByteCount, char *resultBuffer, ...);
struct XSPVDriver_watch *xenstore_watchv(struct XSPVDriver_handle *XSPVDriver, HANDLE event, ...);

enum xenstore_scatter_type {
    xenstore_scatter_type_bad,
    xenstore_scatter_type_grant_ref = 0xbeef,
    xenstore_scatter_type_evtchn_port,
    xenstore_scatter_type_string,
    xenstore_scatter_type_int
};
BOOL xenstore_scatter(struct XSPVDriver_handle *XSPVDriver, const char *prefix, ...);

enum xenstore_gather_type {
    xenstore_gather_type_bad,
    xenstore_gather_type_alien_grant_ref = 0xfeeb,
    xenstore_gather_type_alien_evtchn_port,
    xenstore_gather_type_int
};
BOOL xenstore_gather(struct XSPVDriver_handle *XSPVDriver, const char *prefix, ...);

BOOL xenstore_printfv(struct XSPVDriver_handle *XSPVDriver, ...);

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#endif /* !V2V_PRIVATE_H__ */
