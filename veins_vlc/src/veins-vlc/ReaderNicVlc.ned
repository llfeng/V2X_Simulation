//
// Copyright (C) 2017 Agon Memedi <memedi@ccs-labs.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


package org.car2x.veinsvlc;

//import org.car2x.veinsvlc.PhyLayerVlc;
import org.car2x.veinsvlc.mac.ReaderMacLayerVlc;
import org.car2x.veinsvlc.phy.ReaderPhyLayerVlc;

module ReaderNicVlc
{
    parameters:
        // Explicitly specify the Connection Manager for this type of NIC
        // If not specified, MiXiM defaults to the BaseConnectionManager
        string connectionManagerName = default("vlcConnectionManager");

    gates:
        input upperLayerIn;
        output upperLayerOut;

        output upperControlOut;
        input upperControlIn;

        input radioIn; // radioIn gate for sendDirect

    submodules:
        readerPhyVlc: ReaderPhyLayerVlc {
            @display("p=69,218;i=block/process_s");
        }

        readerMacVlc: ReaderMacLayerVlc {
            @display("p=69,82");
        }


    connections allowunconnected:
        radioIn --> readerPhyVlc.radioIn;

        // Bottom up
        readerPhyVlc.upperLayerOut --> readerMacVlc.lowerLayerIn;
        readerMacVlc.upperLayerOut --> upperLayerOut;
        // Top down
        upperLayerIn --> readerMacVlc.upperLayerIn;
        readerMacVlc.lowerLayerOut --> readerPhyVlc.upperLayerIn;

		// Bottom up Control
		readerPhyVlc.upperControlOut --> readerMacVlc.lowerControlIn;
		readerMacVlc.upperControlOut --> upperControlOut;
		// Top down Control
        upperControlIn --> readerMacVlc.upperControlIn;
        readerMacVlc.lowerControlOut -->readerPhyVlc.upperControlIn;
}
