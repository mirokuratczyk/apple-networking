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

#include "interfaces_ioctl.h"

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/sysctl.h>

// Forward declarations
void print_all_interfaces(int family);
static void list_interfaces(int fd, void (*show)(int fd, const char *name));
static void print_interface(int fd, const char *name);
static void print_interface_address(int, const char *);
static void print_interface_flags(int, const char *);
static void print_interface_addresses(int, const char *);

void print_all_interfaces(int family) {
    int fd;

    // TODO: SOCK_DGRAM as well for UDP?
    fd=socket(family, SOCK_STREAM, 0);
    if(fd<0) {
        perror("(interfaces_ioctl.c) socket()");
        exit(EXIT_FAILURE);
    }
    list_interfaces(fd, print_interface);
    close(fd);
}

static void list_interfaces(int fd, void (*show)(int fd, const char *name)) {
    struct ifreq *ifreq;
    struct ifconf ifconf;
    char buf[16384];
    unsigned i;
    size_t len;

    ifconf.ifc_len=sizeof buf;
    ifconf.ifc_buf=buf;
    if(ioctl(fd, SIOCGIFCONF, &ifconf)!=0) {
        perror("(interfaces_ioctl.c) ioctl(SIOCGIFCONF)");
        exit(EXIT_FAILURE);
    }

    ifreq=ifconf.ifc_req;
    for(i=0; i < ifconf.ifc_len; ) {
        /* some systems have ifr_addr.sa_len and adjust the length that
         * way, but not mine. weird */
#ifndef linux
        len=IFNAMSIZ + ifreq->ifr_addr.sa_len;
#else
        len=sizeof *ifreq;
#endif
        if (show) {
            show(fd, ifreq->ifr_name);
        } else {
            printf("(interfaces_ioctl.c) %s\n", ifreq->ifr_name);
        }
        ifreq=(struct ifreq*)((char*)ifreq+len);
        i+=len;
    }
}

static void print_interface(int fd, const char *name) {
    printf("(interfaces_ioctl.c) %s\n", name);
    print_interface_address(fd, name);
    print_interface_addresses(fd, name);
    print_interface_flags(fd, name);
}

static void print_interface_address(int fd, const char *name) {
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof ifreq);
    strncpy(ifreq.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd,SIOCGIFBRDADDR , &ifreq) < 0) {
        printf("(interfaces_ioctl.c) ioctl failed");
        return;
    }
    in_addr_t addrp = ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr.s_addr;

    char str[50];
    inet_ntop(AF_INET, &(addrp), str, INET_ADDRSTRLEN);
    printf("(interfaces_ioctl.c) \tInterface addr: %s\n", str);
}

static void print_interface_flags(int fd, const char *name) {
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof ifreq);
    strncpy(ifreq.ifr_name, name, IFNAMSIZ);

    strncpy(ifreq.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFFLAGS, &ifreq) < 0) {
        printf("(interfaces_ioctl.c) ioctl failed");
        return;
    }

    printf("(interfaces_ioctl.c) \tFlags: %hx\n", ifreq.ifr_flags);
}

static void print_interface_addresses(int fd, const char *name) {
    int family;
    struct ifreq ifreq;
    char host[128];
    memset(&ifreq, 0, sizeof ifreq);
    strncpy(ifreq.ifr_name, name, IFNAMSIZ);
    if(ioctl(fd, SIOCGIFADDR, &ifreq)!=0) {
        /* perror(name); */
        return; /* ignore */
    }
    switch(family=ifreq.ifr_addr.sa_family) {
        case AF_UNSPEC:
            return; /* ignore */
        case AF_INET:
        case AF_INET6:
            getnameinfo(&ifreq.ifr_addr, sizeof ifreq.ifr_addr, host, sizeof host, 0, 0, NI_NUMERICHOST);
            break;
        default:
            sprintf(host, "(interfaces_ioctl.c) unknown (family: %d)", family);
    }
    printf("(interfaces_ioctl.c) \tAddr: %s\n", host);
}
