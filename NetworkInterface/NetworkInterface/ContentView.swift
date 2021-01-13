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

import SwiftUI

struct ContentView: View {
    @ObservedObject var viewModel = NetworkInterfaceViewModel()
    var body: some View {
        displayNetworkInterfaceInfo(viewModelState: viewModel.state).padding()
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}

func displayNetworkInterfaceInfo(viewModelState: NetworkInterfaceViewModel.NetworkInterfaceViewModelState) -> AnyView {
    switch viewModelState {
    case .nwpathSupportUnknown:
        return AnyView(Text("Querying NWPath support..."))
    case .nwpathUnsupported:
        return AnyView(Text("NWPath unsupported"))
    case .nwpathSupported:
        return AnyView(Text("Querying network state..."))
    case .nwpathQueryResult(let networkInterfaceState):
        let networkPathState = networkInterfaceState.networkPathState
        return
            AnyView(VStack(alignment: .leading, spacing: .some(10), content: {
                Text("NWPath State").bold()
                VStack(alignment: .leading, spacing: .some(10), content: {
                    Text("Update count: \(networkInterfaceState.updateCount)")
                    Text("Is expensive: \(String(describing:networkPathState?.isExpensive))")
                    Text("Is constrained: \(String(describing:networkPathState?.isConstrained))")
                    Text("Supports DNS: \(String(describing:networkPathState?.supportsDNS))")
                    Text("Supports IPv4: \(String(describing:networkPathState?.supportsIPv4))")
                    Text("Supports IPv6: \(String(describing:networkPathState?.supportsIPv6))")
                    Text("Path status:\n" + String(describing: networkPathState?.status))
                    Text("Active interface:" + String(describing: networkPathState?.activeInterface))
                    Text("Active interface type:" + String(describing: networkPathState?.activeInterface?.type))
                    Text("Available interfaces:" + String(describing: networkPathState?.interfaces))
                })
            }))
    }
}

class NetworkInterfaceViewModel: ObservableObject {
    @Published var state: NetworkInterfaceViewModelState = .nwpathSupportUnknown
    
    public enum NetworkInterfaceViewModelState {
        case nwpathSupportUnknown
        case nwpathUnsupported
        case nwpathSupported
        case nwpathQueryResult(NetworkInterfaceState)
        
        public struct NetworkInterfaceState {
            var updateCount: Int = 0
            var networkPathState: NetworkPathState? = nil
        }
    }
    
    init() {
        // Update the UI with the Swift implementation
        let supported =
            monitorNetworkPathState { (pathState) in
                print("NetworkPathState:\(pathState), activeInterfaceType: \(String(describing: pathState.activeInterface?.type))")

                // Print the default gateway according to the modified netstat code
                print_default_gateway()

                DispatchQueue.main.async {
                    switch self.state {
                    case .nwpathQueryResult(let state):
                        self.state = .nwpathQueryResult(NetworkInterfaceViewModelState.NetworkInterfaceState(updateCount: state.updateCount+1, networkPathState: pathState))
                    default:
                        self.state = .nwpathQueryResult(NetworkInterfaceViewModelState.NetworkInterfaceState(updateCount: 1, networkPathState: pathState))
                    }
                }
            }
        
        switch supported {
        case .supported:
            self.state = .nwpathSupported
        case .unsupported:
            self.state = .nwpathUnsupported
        }
        
        // Simply log the Objective-C implementation
        // Note: only need active interface to cross reference against Swift implementation
        NetworkInterfaceMonitor.monitorNetworkPathState { (state) in
            print("NetworkPathStateObjC(\"activeInterface\":\(String(describing: state.activeInterface)))")
        }
    }
}
