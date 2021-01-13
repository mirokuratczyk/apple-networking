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
 * WARNING: this code is incomplete and unused, but it provides an example of how to use ioctl
 * to retrieve interface information.
 */

#ifndef interfaces_ioctl_h
#define interfaces_ioctl_h

#include <stdio.h>

// Usage:
// print_all_interfaces(PF_INET); /* IPv4 */
// print_all_interfaces(PF_INET6); /* IPv6 */
void print_all_interfaces(int family);

#endif /* interfaces_ioctl_h */
