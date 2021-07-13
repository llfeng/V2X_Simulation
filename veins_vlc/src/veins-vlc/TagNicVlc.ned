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

import org.car2x.veinsvlc.mac.TagMacLayerVlc;
import org.car2x.veinsvlc.phy.TagPhyLayerVlc;

module TagNicVlc
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
        tagPhyVlc: TagPhyLayerVlc {
            @display("p=69,218;i=block/process_s");
        }

        tagMacVlc: TagMacLayerVlc {
            @display("p=69,82");
        }


    connections allowunconnected:
        radioIn --> tagPhyVlc.radioIn;

        // Bottom up
        tagPhyVlc.upperLayerOut --> tagMacVlc.lowerLayerIn;
        tagMacVlc.upperLayerOut --> upperLayerOut;
        // Top down
        upperLayerIn --> tagMacVlc.upperLayerIn;
        tagMacVlc.lowerLayerOut --> tagPhyVlc.upperLayerIn;

		// Bottom up Control
		tagPhyVlc.upperControlOut --> tagMacVlc.lowerControlIn;
		tagMacVlc.upperControlOut --> upperControlOut;
		// Top down Control
        upperControlIn --> tagMacVlc.upperControlIn;
        tagMacVlc.lowerControlOut -->tagPhyVlc.upperControlIn;
}
