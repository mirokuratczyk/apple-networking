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

#import "NetworkInterfaceMonitor.h"

#import <Network/path.h>
#import <Network/path_monitor.h>
#import <arpa/inet.h>
#import <net/if.h>
#import <ifaddrs.h>

@implementation NetworkPathStateObjC

@end

bool interfaceIsActiveAndNotLoopback(const char* interfaceName) {

    struct ifaddrs *interfaces;
    if (getifaddrs(&interfaces) != 0) {
        return nil;
    }

    struct ifaddrs *interface;
    for (interface=interfaces; interface; interface=interface->ifa_next) {
        // Only IFF_UP interfaces. Loopback is ignored.
        if (interface->ifa_flags & IFF_UP && !(interface->ifa_flags & IFF_LOOPBACK)) {

            if (interface->ifa_addr && (interface->ifa_addr->sa_family==AF_INET || interface->ifa_addr->sa_family==AF_INET6)) {

                // ifa_name could be NULL
                // https://sourceware.org/bugzilla/show_bug.cgi?id=21812
                if (interface->ifa_name != NULL) {
                    if (strcmp(interfaceName, interface->ifa_name) == 0) {
                        return TRUE;
                    }
                }
            }
        }
    }

    // Free getifaddrs data
    freeifaddrs(interfaces);

    return FALSE;
}

@implementation NetworkInterfaceMonitor

+ (monitor_network_path_state_support_t)monitorNetworkPathState:(NetworkPathStateUpdateHandler)updateHandler {

    static int callCount = 1;

    if (@available(iOS 12.0, *)) {
        nw_path_monitor_t monitor = nw_path_monitor_create();

        // nw_path_t:
        // An object that contains information about the properties of the network that a connection
        // uses, or that are available to your app.
        nw_path_monitor_set_update_handler(monitor, ^(nw_path_t  _Nonnull path) {

            callCount++;

            NetworkPathStateObjC *state = [[NetworkPathStateObjC alloc] init];
            state.updateCount = callCount;
            state.status = nw_path_get_status(path);
            state.isExpensive = nw_path_is_expensive(path);
            state.isConstrained = nw_path_is_constrained(path);
            state.supportsDNS = nw_path_has_dns(path);
            state.supportsIPv4 = nw_path_has_ipv4(path);
            state.supportsIPv6 = nw_path_has_ipv6(path);

            // Discover the active interface type

            nw_interface_type_t active_interface_type = nw_interface_type_other;

            if (nw_path_uses_interface_type(path, nw_interface_type_wifi)) {
                active_interface_type = nw_interface_type_wifi;
            } else if (nw_path_uses_interface_type(path, nw_interface_type_cellular)) {
                active_interface_type = nw_interface_type_cellular;
            } else if (nw_path_uses_interface_type(path, nw_interface_type_wired)) {
                active_interface_type = nw_interface_type_wired;
            } else if (nw_path_uses_interface_type(path, nw_interface_type_loopback)) {
                active_interface_type = nw_interface_type_loopback;
            } else {
                active_interface_type = nw_interface_type_other;
            }

            // Map the active interface type to the interface itself
            // Note: enumerates the list of all interfaces available to the path, in order of preference.
            nw_path_enumerate_interfaces(path, ^bool(nw_interface_t  _Nonnull interface) {
                if (nw_interface_get_type(interface) == active_interface_type) {
                    if (interfaceIsActiveAndNotLoopback(nw_interface_get_name(interface))) {
                        state.activeInterface = interface;
                        return false;
                    }
                    // TODO: log the rejected interface
                    return true;
                }
                // Continue searching
                return true;
            });
            updateHandler(state);
        });

        nw_path_monitor_set_queue(monitor, dispatch_queue_create("nwpathmonitor.queue", DISPATCH_QUEUE_SERIAL));
        nw_path_monitor_start(monitor);

        return monitor_network_path_state_supported;
    } else {
        // Fallback on earlier versions
    }
    return monitor_network_path_state_unsupported;
}

@end
