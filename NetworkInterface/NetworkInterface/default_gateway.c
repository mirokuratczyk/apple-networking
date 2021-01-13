/*
 * Copyright (c) 2008-2017 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1983, 1988, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Modifications made by Psiphon Inc. on January 11th, 2021.
 */
/*
 * Copyright (c) 2020, Psiphon Inc.
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * This file comprises a stripped down version of the netstat source
 * code which finds and prints the interface associated with the
 * default gateway in the routing table. That is the interface
 * listed as `default` under the destination column when using the
 * command `netstat -nr -f inet`. E.g.:
 *
 *  Destination        Gateway            Flags        Netif Expire
 *  default            192.168.1.5        UGSc           en8
 *  default            192.168.9.1        UGScI          en0
 *
 * This is accomplished by searching for the route which handles
 * destination 0.0.0.0 and then printing the associated interface.
 *
 * NOTE: the definition of each flag can be found in the man entry
 * for netstat: `man netstat`.
 *
 * NOTE: this functionality could also be acheived by querying the
 * routing information database (see: http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fr%2Froute_proto.html,
 * and Apple's Open Source route.c implementation https://opensource.apple.com/source/network_cmds/network_cmds-606.40.2/route.tproj/route.c.auto.html)
 *
 * WARNING: some of the datastructures contained within <net/route.h>
 * have been duplicated here because that header is not exposed on iOS.
 */

#include "default_gateway.h"

#define IOS_PLATFORM 1

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#ifndef IOS_PLATFORM
#include <net/route.h>
#endif

#ifdef IOS_PLATFORM
// Redefine required <net/route.h> code

#define RTA_DST         0x1
#define RTA_NETMASK     0x4
#define RTAX_DST        0
#define RTAX_NETMASK    2
#define RTAX_MAX        8

#define RTF_UP          0x1             /* route usable */
#define RTF_GATEWAY     0x2             /* destination is a gateway */
#define RTF_HOST        0x4             /* host entry (net otherwise) */
#define RTF_REJECT      0x8             /* host or net unreachable */
#define RTF_DYNAMIC     0x10            /* created dynamically (by redirect) */
#define RTF_MODIFIED    0x20            /* modified dynamically (by redirect) */
#define RTF_DONE        0x40            /* message confirmed */
#define RTF_DELCLONE    0x80            /* delete cloned route */
#define RTF_CLONING     0x100           /* generate new routes on use */
#define RTF_XRESOLVE    0x200           /* external daemon resolves name */
#define RTF_LLINFO      0x400           /* DEPRECATED - exists ONLY for backward
*  compatibility */
#define RTF_LLDATA      0x400           /* used by apps to add/del L2 entries */
#define RTF_STATIC      0x800           /* manually added */
#define RTF_BLACKHOLE   0x1000          /* just discard pkts (during updates) */
#define RTF_NOIFREF     0x2000          /* not eligible for RTF_IFREF */
#define RTF_PROTO2      0x4000          /* protocol specific routing flag */
#define RTF_PROTO1      0x8000          /* protocol specific routing flag */

#define RTF_PRCLONING   0x10000         /* protocol requires cloning */
#define RTF_WASCLONED   0x20000         /* route generated through cloning */
#define RTF_PROTO3      0x40000         /* protocol specific routing flag */
/* 0x80000 unused */
#define RTF_PINNED      0x100000        /* future use */
#define RTF_LOCAL       0x200000        /* route represents a local address */
#define RTF_BROADCAST   0x400000        /* route represents a bcast address */
#define RTF_MULTICAST   0x800000        /* route represents a mcast address */
#define RTF_IFSCOPE     0x1000000       /* has valid interface scope */
#define RTF_CONDEMNED   0x2000000       /* defunct; no longer modifiable */
#define RTF_IFREF       0x4000000       /* route holds a ref to interface */
#define RTF_PROXY       0x8000000       /* proxying, no interface scope */
#define RTF_ROUTER      0x10000000      /* host is a router */
#define RTF_DEAD        0x20000000      /* Route entry is being freed */
/* 0x40000000 and up unassigned */

/*
 * These numbers are used by reliable protocols for determining
 * retransmission behavior and are included in the routing structure.
 */
struct rt_metrics {
    u_int32_t       rmx_locks;      /* Kernel leaves these values alone */
    u_int32_t       rmx_mtu;        /* MTU for this path */
    u_int32_t       rmx_hopcount;   /* max hops expected */
    int32_t         rmx_expire;     /* lifetime for route, e.g. redirect */
    u_int32_t       rmx_recvpipe;   /* inbound delay-bandwidth product */
    u_int32_t       rmx_sendpipe;   /* outbound delay-bandwidth product */
    u_int32_t       rmx_ssthresh;   /* outbound gateway buffer limit */
    u_int32_t       rmx_rtt;        /* estimated round trip time */
    u_int32_t       rmx_rttvar;     /* estimated rtt variance */
    u_int32_t       rmx_pksent;     /* packets sent using this route */
    u_int32_t       rmx_state;      /* route state */
    u_int32_t       rmx_filler[3];  /* will be used for T/TCP later */
};

// NOTE: copied from <net/route.h>
struct rt_msghdr2 {
    u_short rtm_msglen;     /* to skip over non-understood messages */
    u_char  rtm_version;    /* future binary compatibility */
    u_char  rtm_type;       /* message type */
    u_short rtm_index;      /* index for associated ifp */
    int     rtm_flags;      /* flags, incl. kern & message, e.g. DONE */
    int     rtm_addrs;      /* bitmask identifying sockaddrs in msg */
    int32_t rtm_refcnt;     /* reference count */
    int     rtm_parentflags; /* flags of the parent route */
    int     rtm_reserved;   /* reserved field set to 0 */
    int     rtm_use;        /* from rtentry */
    u_int32_t rtm_inits;    /* which metrics we are initializing */
    struct rt_metrics rtm_rmx; /* metrics themselves */
};

#endif

int lflag = 1; /* show routing table with use and ref */

// NOTE: copied from Apple netstat source
typedef union {
    uint32_t dummy;        /* Helps align structure. */
    struct    sockaddr u_sa;
    u_short    u_data[128];
} sa_u;

// Copied from Apple netstat source
// TODO: hard coded ipv4
#define    WID_DST(af) \
((af) == AF_INET6 ? (lflag ? 39 : (nflag ? 39: 18)) : 18)
#define    WID_GW(af) \
((af) == AF_INET6 ? (lflag ? 31 : (nflag ? 31 : 18)) : 18)
#define    WID_RT_IFA(af) \
((af) == AF_INET6 ? (lflag ? 39 : (nflag ? 39 : 18)) : 18)
#define    WID_IF(af)    ((af) == AF_INET6 ? 8 : 7)

// NOTE: copied from Apple netstat source
#define ROUNDUP(a) \
((a) > 0 ? (1 + (((a) - 1) | (sizeof(uint32_t) - 1))) : sizeof(uint32_t))

// NOTE: adapted from p_sockaddr in Apple netstat source
static int
is_default_route(struct sockaddr *sa, struct sockaddr *mask, int flags)
{
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;

        if ((sin->sin_addr.s_addr == INADDR_ANY) &&
            mask &&
            (ntohl(((struct sockaddr_in *)mask)->sin_addr.s_addr) == 0L || mask->sa_len == 0) &&
            flags & RTF_GATEWAY) {
            return 1;
        }
    }

    return 0;
}

// NOTE: copied from Apple netstat source
static void
get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
    int i;

    for (i = 0; i < RTAX_MAX; i++)
    {
        if (addrs & (1 << i))
        {
            rti_info[i] = sa;
            sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
        }
        else
        {
            rti_info[i] = NULL;
        }
    }
}

// NOTE: adapted from np_rtentry in Apple netstat source
static void
np_rtentry(struct rt_msghdr2 *rtm)
{

    struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
    struct sockaddr *rti_info[RTAX_MAX];
    u_short lastindex = 0xffff;
    static char ifname[IFNAMSIZ + 1];
    sa_u addr, mask;

    get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
    bzero(&addr, sizeof(addr));

    if ((rtm->rtm_addrs & RTA_DST))
        bcopy(rti_info[RTAX_DST], &addr, rti_info[RTAX_DST]->sa_len);
    bzero(&mask, sizeof(mask));
    if ((rtm->rtm_addrs & RTA_NETMASK))
        bcopy(rti_info[RTAX_NETMASK], &mask, rti_info[RTAX_NETMASK]->sa_len);

    if (is_default_route(&addr.u_sa, &mask.u_sa, rtm->rtm_flags) == 1)
    {

        if (rtm->rtm_index != lastindex)
        {
            if_indextoname(rtm->rtm_index, ifname);
            lastindex = rtm->rtm_index;
        }

        printf("(default_gateway.c) Default gateway: %*.*s", WID_IF(addr.u_sa.sa_family),
               WID_IF(addr.u_sa.sa_family), ifname);

        putchar('\n');
    }
}

/*
 * Print routing tables.
 * NOTE: `routepr` modified to print the interface associated with the default gateway.
 */
void
print_default_gateway(void)
{
    size_t extra_space;
    size_t needed;
    int mib[6];
    char *buf, *next, *lim;
    struct rt_msghdr2 *rtm = NULL;
    int try = 1;

again:
    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE;
    mib[2] = 0;
    mib[3] = 0;
    mib[4] = NET_RT_DUMP2;
    mib[5] = 0;
    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
        err(1, "(default_gateway.c) sysctl: net.route.0.0.dump estimate");
    }
    /* allocate extra space in case the table grows */
    extra_space = needed / 2;
    if (needed <= (SIZE_MAX - extra_space)) {
        needed += extra_space;
    }
    if ((buf = malloc(needed)) == 0) {
        err(2, "(default_gateway.c) malloc(%lu)", (unsigned long)needed);
    }
    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
#define MAX_TRIES    10
        if (errno == ENOMEM && try < MAX_TRIES) {
            /* the buffer we provided was too small, try again */
            free(buf);
            try++;
            goto again;
        }
        err(1, "(default_gateway.c) sysctl: net.route.0.0.dump");
    }
    lim  = buf + needed;
    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr2 *)next;
        np_rtentry(rtm);
    }
}
