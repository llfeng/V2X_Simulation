//
// Copyright (C) 2019 Max Schettler <schettler@ccs-labs.org>
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

#pragma once

#include "veins-vlc/messages/VlbcMacPkt_m.h"

#include "veins/base/modules/BaseMacLayer.h"



#define INITIAL_STATE           0
#define DISC_REQ_SENT_STATE     1
#define DISC_ACK_RECVD_STATE    2
#define QUERY_REQ_SENT_STATE    3
#define QUERY_RSP_RECVD_STATE   4


namespace veins {

class ReaderMacLayerVlc : public BaseMacLayer {
private:
    int vlbcState;
    int vlbcSelfAddr;
    int destAddr;
    int msgType;
    int collisionNum;
    int windows;
    int round;
    int ackFlag;
    int ackCrc8;
    int overhearCount;
    int overhear_buf[1024];
    int ackCount;
    int ack_buf[1024];

    int collision_sem;
    cMessage *originMsg = NULL;

    int tag_vec[128];
    int tag_vec_direct[128];
    int tag_vec_overhear[128];
    int tag_vec_relay[128];
    int tag_vec_first_hop[128];

    int realId;
    int collision_uplink_count;
    int direct_uplink_count;
    int overhear_uplink_count;
    int first_hop_uplink_count;
    int relay_uplink_count;

public:
    void initialize(int) override;

    void handleUpperMsg(cMessage* msg) override;
    void handleLowerControl(cMessage* msg) override;
    void handleLowerMsg(cMessage* msg) override;
    void handleSelfMsg(cMessage* msg) override;
    VlbcMacPkt* encapsMsg(cPacket* netwPkt) override;
    //VlbcMacPkt* encaps();
    void enqueuePacket(cPacket* pkt);
    void transmissionOpportunity();

    cPacketQueue queue;
    int queueSize;
    bool transmitting;

protected:
    cOutVector currentCollision;
};

} // namespace veins

