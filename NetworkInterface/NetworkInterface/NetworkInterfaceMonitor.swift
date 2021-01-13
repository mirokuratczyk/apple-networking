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
 * Swift implementation of network interface discovery code
 * with NWPath.
 */

import Network

public struct NetworkPathState {
    let status : NWPath.Status
    let isExpensive: Bool
    let isConstrained: Bool
    let supportsDNS: Bool
    let supportsIPv4: Bool
    let supportsIPv6: Bool
    let activeInterface : NWInterface?
    let interfaces: [NWInterface]
}

public enum MonitorNetworkPathStateSupport {
    case unsupported
    case supported
}

public func monitorNetworkPathState(handler: @escaping (NetworkPathState) -> Void) -> MonitorNetworkPathStateSupport {
    if #available(iOS 12.0, *) {
        let monitor = NWPathMonitor()
        monitor.pathUpdateHandler = { path in
            var activeInterface: NWInterface?

            for interface in path.availableInterfaces {
                // Select the first interface that matches the
                // active interface type.
                if path.usesInterfaceType(interface.type) {
                    if (interfaceIsActiveAndNotLoopback(interface.name)) {
                        activeInterface = interface
                    }
                } else {
                    // TODO: log the interface
                }
            }

            let state =  NetworkPathState(status: path.status,
                                          isExpensive: path.isExpensive,
                                          isConstrained: path.isConstrained,
                                          supportsDNS: path.supportsDNS,
                                          supportsIPv4: path.supportsIPv4,
                                          supportsIPv6: path.supportsIPv6,
                                          activeInterface: activeInterface,
                                          interfaces: path.availableInterfaces)
            handler(state)
        }
        monitor.start(queue: DispatchQueue(label: "nwpathmonitor.queue"))

        return .supported
    } else {
        // Fallback on earlier versions
        return .unsupported
    }
}
