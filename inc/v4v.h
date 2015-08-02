/*
 * Copyright (c) 2014 Citrix Systems, Inc.
 */

/******************************************************************************
 * v4v.h
 * 
 * Xen interdomain communications module interface.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __V4V_H__
#define __V4V_H__

/* Compiler specific hacks */
#if !defined(__GNUC__)
#define V4V_PACKED
#define V4V_INLINE __inline
#else /* __GNUC__ */
/* #include  <xen/types.h> */
#define V4V_PACKED __attribute__ ((packed))
#define V4V_INLINE inline
#endif /* __GNUC__ */

/* Get domid_t and DOMID_INVALID defined */
#ifdef __XEN__
#include <xen/types.h>
#include <public/xen.h>
typedef int ssize_t;            //FIXME this needs to be somewhere else
#define V4V_VOLATILE
#else
#if defined(__unix__)
#define V4V_VOLATILE volatile
/* If we're running on unix we can use the Xen headers */
#ifdef __KERNEL__
#include <xen/interface/xen.h>
#else
#include <xen/xen.h>
#endif
#else
#define V4V_VOLATILE volatile
#include "xen.h"
typedef int ssize_t;
#endif
#endif

#ifndef DOMID_INVALID
#define DOMID_INVALID (0x7FF4U)
#endif

#if !defined(__GNUC__)
#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable: 4200)
#endif


#define V4V_PROTO_DGRAM		0x3c2c1db8
#define V4V_PROTO_STREAM 	0x70f6a8e5

/************** Structure definitions **********/

#ifdef __i386__
#define V4V_RING_MAGIC  0xdf6977f231abd910ULL
#define V4V_PFN_LIST_MAGIC  0x91dd6159045b302dULL
#else
#define V4V_RING_MAGIC  0xdf6977f231abd910
#define V4V_PFN_LIST_MAGIC  0x91dd6159045b302d
#endif
#define V4V_DOMID_NONE 	DOMID_INVALID
#define V4V_DOMID_ANY 	DOMID_INVALID
#define V4V_PORT_NONE   0

typedef struct v4v_iov
{
    uint64_t iov_base;
    uint64_t iov_len;
} V4V_PACKED v4v_iov_t;

DEFINE_XEN_GUEST_HANDLE (v4v_iov_t);

typedef struct v4v_addr
{
    uint32_t port;
    domid_t domain;
} V4V_PACKED v4v_addr_t;

DEFINE_XEN_GUEST_HANDLE (v4v_addr_t);

typedef struct v4v_viptables_rule
{
    struct v4v_addr src;
    struct v4v_addr dst;
    uint32_t accept;
} V4V_PACKED v4v_viptables_rule_t;

DEFINE_XEN_GUEST_HANDLE (v4v_viptables_rule_t);

typedef struct v4v_ring_id
{
    struct v4v_addr addr;
    domid_t partner;
} V4V_PACKED v4v_ring_id_t;


typedef uint64_t v4v_pfn_t;
DEFINE_XEN_GUEST_HANDLE (v4v_pfn_t);

typedef struct v4v_pfn_list_t
{
    uint64_t magic;
    uint32_t npage;
    uint32_t pad;
    uint64_t reserved[3];
    v4v_pfn_t pages[0];
} V4V_PACKED v4v_pfn_list_t;

DEFINE_XEN_GUEST_HANDLE (v4v_pfn_list_t);


typedef struct v4v_ring
{
    uint64_t magic;
    struct v4v_ring_id id;      /*Identifies ring_id - xen only looks at this during register/unregister and will fill in id.addr.domain */
    uint32_t len;               /*length of ring[], must be a multiple of 8 */
    V4V_VOLATILE uint32_t rx_ptr; /*rx_ptr - modified by domain */
    V4V_VOLATILE uint32_t tx_ptr; /*tx_ptr - modified by xen */
    uint64_t reserved[4];
    V4V_VOLATILE uint8_t ring[0];
} V4V_PACKED v4v_ring_t;

DEFINE_XEN_GUEST_HANDLE (v4v_ring_t);

#ifdef __i386__
#define V4V_RING_DATA_MAGIC	0x4ce4d30fbc82e92aULL
#else
#define V4V_RING_DATA_MAGIC	0x4ce4d30fbc82e92a
#endif

#define V4V_RING_DATA_F_EMPTY       1U << 0 /*Ring is empty */
#define V4V_RING_DATA_F_EXISTS      1U << 1 /*Ring exists */
#define V4V_RING_DATA_F_PENDING     1U << 2 /*Pending interrupt exists - do not rely on this field - for profiling only */
#define V4V_RING_DATA_F_SUFFICIENT  1U << 3 /*Sufficient space to queue space_required bytes exists */

typedef struct v4v_ring_data_ent
{
    struct v4v_addr ring;
    uint16_t flags;
    uint32_t space_required;
    uint32_t max_message_size;
} V4V_PACKED v4v_ring_data_ent_t;

DEFINE_XEN_GUEST_HANDLE (v4v_ring_data_ent_t);

typedef struct v4v_ring_data
{
    uint64_t magic;
    uint32_t nent;
    uint32_t pad;
    uint64_t reserved[4];
    struct v4v_ring_data_ent data[0];
} V4V_PACKED v4v_ring_data_t;

DEFINE_XEN_GUEST_HANDLE (v4v_ring_data_t);


#define V4V_ROUNDUP(a) (((a) +0xf ) & ~0xf)
/* Messages on the ring are padded to 128 bits */
/* len here refers to the exact length of the data not including the 128 bit header*/
/* the the message uses ((len +0xf) & ~0xf) + sizeof(v4v_ring_message_header) bytes */


#define V4V_SHF_SYN		(1 << 0)
#define V4V_SHF_ACK		(1 << 1)
#define V4V_SHF_RST		(1 << 2)

#define V4V_SHF_PING		(1 << 8)
#define V4V_SHF_PONG		(1 << 9)

struct v4v_stream_header
{
    uint32_t flags;
    uint32_t conid;
} V4V_PACKED;

struct v4v_ring_message_header
{
    uint32_t len;
    struct v4v_addr source;
    uint16_t pad;
    uint32_t protocol;
    uint8_t data[0];

} V4V_PACKED;

/************************** Hyper calls ***************/

/*Prototype of hypercall is */
/*long do_v4v_op(int cmd,XEN_GUEST_HANDLE(void),XEN_GUEST_HANDLE(void),XEN_GUEST_HANDLE(void),uint32_t,uint32_t)*/


#define V4VOP_register_ring 	1
/*int, XEN_GUEST_HANDLE(v4v_ring_t) ring, XEN_GUEST_HANDLE(v4v_pfn_list_t) */

/* Registers a ring with Xen, if a ring with the same v4v_ring_id exists,
 * this ring takes its place, registration will not change tx_ptr 
 * unless it is invalid */

#define V4VOP_unregister_ring 	2
/*int, XEN_GUEST_HANDLE(v4v_ring_t) ring */

#define V4VOP_send 		3
/*int, XEN_GUEST_HANDLE(v4v_addr_t) src,XEN_GUEST_HANDLE(v4v_addr_t) dst,XEN_GUEST_HANDLE(void) buf, UINT32_t len,uint32_t protocol*/

/* Sends len bytes of buf to dst, giving src as the source address (xen will
 * ignore src->domain and put your domain in the actually message), xen
 * first looks for a ring with id.addr==dst and id.partner==sending_domain
 * if that fails it looks for id.addr==dst and id.partner==DOMID_ANY. 
 * protocol is the 32 bit protocol number used from the message
 * most likely V4V_PROTO_DGRAM or STREAM. If insufficient space exists
 * it will return -EAGAIN and xen will twing the V4V_INTERRUPT when
 * sufficient space becomes available */


#define V4VOP_notify 		4
/*int, XEN_GUEST_HANDLE(v4v_ring_data_t) buf*/

/* Asks xen for information about other rings in the system */
/* v4v_ring_data_t contains an array of v4v_ring_data_ent_t
 *
 * ent->ring is the v4v_addr_t of the ring you want information on
 * the same matching rules are used as for V4VOP_send.
 *
 * ent->space_required  if this field is not null xen will check
 * that there is space in the destination ring for this many bytes
 * of payload. If there is it will set the V4V_RING_DATA_F_SUFFICIENT
 * and CANCEL any pending interrupt for that ent->ring, if insufficient
 * space is available it will schedule an interrupt and the flag will
 * not be set.
 *
 * The flags are set by xen when notify replies
 * V4V_RING_DATA_F_EMPTY	ring is empty
 * V4V_RING_DATA_F_PENDING	interrupt is pending - don't rely on this
 * V4V_RING_DATA_F_SUFFICIENT	sufficient space for space_required is there
 * V4V_RING_DATA_F_EXISTS	ring exists
 */


#define V4VOP_sendv		5
/*int, XEN_GUEST_HANDLE(v4v_addr_t) src,XEN_GUEST_HANDLE(v4v_addr_t) dst,XEN_GUEST_HANDLE(v4v_iov_t) , UINT32_t niov,uint32_t protocol*/

/* Identical to V4VOP_send except rather than buf and len it takes 
 * an array of v4v_iov_t and a length of the array */

#define V4VOP_viptables_add     6
#define V4VOP_viptables_del     7
#define V4VOP_viptables_list    8

#if !defined(__GNUC__)
#pragma warning(pop)
#pragma pack(pop)
#endif

/* This structure is used for datagram reads and writes. When sending a
 * datagram, extra space must be reserved at the front of the buffer to
 * format the @addr values in the following structure to indicate the
 * destination address. When receiving data, the receive buffer should also
 * supply the extra head room for the source information that will be
 * returned by V4V. The size of the send/receive should include the extra
 * space for the datagram structure.
 */
#pragma pack(push, 1)
typedef struct _V4V_DATAGRAM {
    v4v_addr_t addr;
    /* data starts here */
} V4V_DATAGRAM, *PV4V_DATAGRAM;
#pragma pack(pop)

/* Typedef for internal stream header structure */
typedef struct v4v_stream_header V4V_STREAM, *PV4V_STREAM;

/* Default internal max backlog length for pending connections */
#define V4V_SOMAXCONN 128

typedef struct _V4V_INIT_VALUES {
    VOID *rxEvent;
    ULONG32 ringLength;
} V4V_INIT_VALUES, *PV4V_INIT_VALUES;

typedef struct _V4V_BIND_VALUES {
    struct v4v_ring_id ringId;
} V4V_BIND_VALUES, *PV4V_BIND_VALUES;

typedef struct _V4V_LISTEN_VALUES {
    ULONG32 backlog;
} V4V_LISTEN_VALUES, *PV4V_LISTEN_VALUES;

typedef union _V4V_ACCEPT_PRIVATE {
    struct {
        ULONG32 a;
        ULONG32 b;
    } d;
    struct {
        ULONG64 a;
    } q;
} V4V_ACCEPT_PRIVATE, *PV4V_ACCEPT_PRIVATE;

typedef struct _V4V_ACCEPT_VALUES {
    VOID *fileHandle;
    VOID *rxEvent;
    struct v4v_addr peerAddr;
    V4V_ACCEPT_PRIVATE priv;
} V4V_ACCEPT_VALUES, *PV4V_ACCEPT_VALUES;

typedef struct _V4V_CONNECT_VALUES {
    V4V_STREAM sh;
    struct v4v_addr ringAddr;
} V4V_CONNECT_VALUES, *PV4V_CONNECT_VALUES;

typedef struct _V4V_WAIT_VALUES {
    V4V_STREAM sh;
} V4V_WAIT_VALUES, *PV4V_WAIT_VALUES;

typedef enum _V4V_GETINFO_TYPE {
    V4vInfoUnset    = 0,
    V4vGetLocalInfo = 1,
    V4vGetPeerInfo  = 2
} V4V_GETINFO_TYPE, *PV4V_GETINFO_TYPE;

typedef struct _V4V_GETINFO_VALUES {
    V4V_GETINFO_TYPE type;
    struct v4v_ring_id ringInfo;
} V4V_GETINFO_VALUES, *PV4V_GETINFO_VALUES;

#if defined(_WIN64)
#define V4V_64BIT 0x800
#else
#define V4V_64BIT 0x000
#endif

/* V4V I/O Control Function Codes */
#define V4V_FUNC_INITIALIZE 0x10
#define V4V_FUNC_BIND       0x11
#define V4V_FUNC_LISTEN     0x12
#define V4V_FUNC_ACCEPT     0x13
#define V4V_FUNC_CONNECT    0x14
#define V4V_FUNC_WAIT       0x15
#define V4V_FUNC_DISCONNECT 0x16
#define V4V_FUNC_GETINFO    0x17
#define V4V_FUNC_DUMPRING   0x18

/* V4V I/O Control Codes */
#if defined(_WIN64)
#define V4V_IOCTL_INITIALIZE CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_INITIALIZE|V4V_64BIT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#else
#define	V4V_IOCTL_INITIALIZE CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_INITIALIZE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#define	V4V_IOCTL_BIND       CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_BIND, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	V4V_IOCTL_LISTEN     CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_LISTEN, METHOD_BUFFERED, FILE_ANY_ACCESS)
#if defined(_WIN64)
#define V4V_IOCTL_ACCEPT     CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_ACCEPT|V4V_64BIT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#else
#define	V4V_IOCTL_ACCEPT     CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_ACCEPT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#define	V4V_IOCTL_CONNECT    CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_CONNECT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	V4V_IOCTL_WAIT       CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_WAIT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	V4V_IOCTL_DISCONNECT CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_DISCONNECT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	V4V_IOCTL_GETINFO    CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_GETINFO, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	V4V_IOCTL_DUMPRING   CTL_CODE(FILE_DEVICE_UNKNOWN, V4V_FUNC_DUMPRING, METHOD_BUFFERED, FILE_ANY_ACCESS)

/************ Internal RING 0/-1 parts **********/
#if !defined(V4V_EXCLUDE_INTERNAL)

#if !defined(__GNUC__)
static __inline void
mb (void)
{
    _mm_mfence ();
    _ReadWriteBarrier ();
}
#endif

/*************** Utility functions **************/

static V4V_INLINE uint32_t
v4v_ring_bytes_to_read (volatile struct v4v_ring *r)
{
    int32_t ret;
    ret = r->tx_ptr - r->rx_ptr;
    if (ret >= 0)
        return ret;
    return (uint32_t) (r->len + ret);
}


/* Copy at most t bytes of the next message in the ring, into the buffer */
/* at _buf, setting from and protocol if they are not NULL, returns */
/* the actual length of the message, or -1 if there is nothing to read */


static V4V_INLINE ssize_t
v4v_copy_out (struct v4v_ring *r, struct v4v_addr *from, uint32_t * protocol,
              void *_buf, size_t t, int consume)
{
    volatile struct v4v_ring_message_header *mh;
    /* unnecessary cast from void * required by MSVC compiler */
    uint8_t *buf = (uint8_t *) _buf; 
    uint32_t btr = v4v_ring_bytes_to_read (r);
    uint32_t rxp = r->rx_ptr;
    uint32_t bte;
    uint32_t len;
    ssize_t ret;


    if (btr < sizeof (*mh))
        return -1;

/*Becuase the message_header is 128 bits long and the ring is 128 bit aligned, we're gaurunteed never to wrap*/
    mh = (volatile struct v4v_ring_message_header *) &r->ring[r->rx_ptr];

    len = mh->len;
    if (btr < len)
        return -1;

#if defined(__GNUC__) 
    if (from)
        *from = mh->source;
#else
	/* MSVC can't do the above */
    if (from)
	memcpy((void *) from, (void *) &(mh->source), sizeof(struct v4v_addr));
#endif

    if (protocol)
        *protocol = mh->protocol;

    rxp += sizeof (*mh);
    if (rxp == r->len)
        rxp = 0;
    len -= sizeof (*mh);
    ret = len;

    bte = r->len - rxp;

    if (bte < len)
      {
          if (t < bte)
            {
                if (buf)
                  {
                      memcpy (buf, (void *) &r->ring[rxp], t);
                      buf += t;
                  }

                rxp = 0;
                len -= bte;
                t = 0;
            }
          else
            {
                if (buf)
                  {
                      memcpy (buf, (void *) &r->ring[rxp], bte);
                      buf += bte;
                  }
                rxp = 0;
                len -= bte;
                t -= bte;
            }
      }

    if (buf && t)
        memcpy (buf, (void *) &r->ring[rxp], (t < len) ? t : len);


    rxp += V4V_ROUNDUP (len);
    if (rxp == r->len)
        rxp = 0;

    mb ();

    if (consume)
        r->rx_ptr = rxp;


    return ret;
}

static V4V_INLINE void
v4v_memcpy_skip (void *_dst, const void *_src, size_t len, size_t *skip)
{
    const uint8_t *src =  (const uint8_t *) _src;
    uint8_t *dst = (uint8_t *) _dst;

    if (!*skip)
      {
          memcpy (dst, src, len);
          return;
      }

    if (*skip >= len)
      {
          *skip -= len;
          return;
      }

    src += *skip;
    dst += *skip;
    len -= *skip;
    *skip = 0;

    memcpy (dst, src, len);
}

/* Copy at most t bytes of the next message in the ring, into the buffer 
 * at _buf, skipping skip bytes, setting from and protocol if they are not 
 * NULL, returns the actual length of the message, or -1 if there is 
 * nothing to read */

static V4V_INLINE ssize_t
v4v_copy_out_offset (struct v4v_ring *r, struct v4v_addr *from,
                     uint32_t * protocol, void *_buf, size_t t, int consume,
                     size_t skip)
{
    volatile struct v4v_ring_message_header *mh;
    /* unnecessary cast from void * required by MSVC compiler */
    uint8_t *buf = (uint8_t *) _buf;
    uint32_t btr = v4v_ring_bytes_to_read (r);
    uint32_t rxp = r->rx_ptr;
    uint32_t bte;
    uint32_t len;
    ssize_t ret;

    buf -= skip;

    if (btr < sizeof (*mh))
        return -1;

/*Becuase the message_header is 128 bits long and the ring is 128 bit aligned, we're gaurunteed never to wrap*/
    mh = (volatile struct v4v_ring_message_header *) &r->ring[r->rx_ptr];

    len = mh->len;
    if (btr < len)
        return -1;

#if defined(__GNUC__) 
    if (from)
        *from = mh->source;
#else
	/* MSVC can't do the above */
    if (from)
	memcpy((void *) from, (void *) &(mh->source), sizeof(struct v4v_addr));
#endif

    if (protocol)
        *protocol = mh->protocol;

    rxp += sizeof (*mh);
    if (rxp == r->len)
        rxp = 0;
    len -= sizeof (*mh);
    ret = len;

    bte = r->len - rxp;

    if (bte < len)
      {
          if (t < bte)
            {
                if (buf)
                  {
                      v4v_memcpy_skip (buf, (void *) &r->ring[rxp], t, &skip);
                      buf += t;
                  }

                rxp = 0;
                len -= bte;
                t = 0;
            }
          else
            {
                if (buf)
                  {
                      v4v_memcpy_skip (buf, (void *) &r->ring[rxp], bte,
                                       &skip);
                      buf += bte;
                  }
                rxp = 0;
                len -= bte;
                t -= bte;
            }
      }

    if (buf && t)
        v4v_memcpy_skip (buf, (void *) &r->ring[rxp], (t < len) ? t : len,
                         &skip);


    rxp += V4V_ROUNDUP (len);
    if (rxp == r->len)
        rxp = 0;

    mb ();

    if (consume)
        r->rx_ptr = rxp;


    return ret;
}

#endif /* V4V_EXCLUDE_INTERNAL */

#endif /* __V4V_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
