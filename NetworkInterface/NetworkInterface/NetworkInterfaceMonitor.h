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

#import <Foundation/Foundation.h>
#import <Network/path.h>
#import <Network/path_monitor.h>


NS_ASSUME_NONNULL_BEGIN

bool interfaceIsActiveAndNotLoopback(const char* interfaceName);

typedef enum {
    monitor_network_path_state_unsupported = 0,
    monitor_network_path_state_supported = 1
} monitor_network_path_state_support_t;

@interface NetworkPathStateObjC : NSObject

@property (assign) int updateCount;

@property (assign) nw_path_status_t status;

// NOTE: only available on iOS 14.2 and above, otherwise set to 0.
@property (assign) nw_path_unsatisfied_reason_t unsatisfiedReason;

@property (assign) BOOL isExpensive;

@property (assign) BOOL isConstrained;

@property (assign) BOOL supportsDNS;

@property (assign) BOOL supportsIPv4;

@property (assign) BOOL supportsIPv6;

@property (nonatomic, nullable) nw_interface_t activeInterface;

@property (nonatomic) NSArray<nw_interface_t> *interfaces;

@end

typedef void (^NetworkPathStateUpdateHandler) (NetworkPathStateObjC *pathState);

/// Objective-C implementation of network interface discovery code
/// with nw_path.
@interface NetworkInterfaceMonitor : NSObject

+ (monitor_network_path_state_support_t)monitorNetworkPathState:(NetworkPathStateUpdateHandler)updateHandler;

@end

NS_ASSUME_NONNULL_END
