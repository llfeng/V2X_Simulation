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

import org.car2x.veins.base.phyLayer.BasePhyLayer;

simple PhyLayerVlc extends BasePhyLayer
{
    parameters:
        @class(veins::PhyLayerVlc);
        //enables/disables collection of statistics about collision. notice that
        //enabling this feature increases simulation time
        bool collectCollisionStatistics = default(true);
        
        double txPower @unit(mW);
        double bitrate @unit(bps);

        // Parameters for LsvLightModel
        double photodiodeGroundOffsetZ @unit("m"); //relative to ground
        double interModuleDistance @unit("m"); //distance between left and right light module
        string radiationPatternId;
        string photodiodeId;

}
