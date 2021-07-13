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
#define DISC_REQ_RECVD_STATE     1
#define DISC_ACK_SENT_STATE    2
#define QUERY_REQ_RECVD_STATE    3
#define QUERY_RSP_SENT_STATE   4


namespace veins {

class VID{
public:
    int readerAddr;
    int round;
    int shortid;
    int hop;

    VID(int i,int j, int k, int l):readerAddr(i),round(j),shortid(k),hop(l) {}
    int getreaderAddr(){
        return this->readerAddr;
    }
    int getround(){
        return this->round;
    }
    int getshortid(){
        return this->shortid;
    }
    int gethop(){
        return this->hop;
    }
    bool operator==(VID &t){
        if(this->readerAddr == t.readerAddr && this->round == t.round && this->shortid == t.shortid){
            return true;
        }else{
            return false;
        }
    }
};

class msgRecord{
public:
    int msgid;
    std::vector< std::pair<VID, simtime_t> > vids;
    std::vector< std::pair<int, simtime_t> > silentReaders;

    msgRecord(int id):msgid(id) {}
};




class TagMacLayerVlc : public BaseMacLayer {
private:
    int vlbcState;
    int vlbcSelfAddr;
    int destAddr;
    int msgType;
    int collisionNum;
    int windows;
    int round;
    int hop;
    int ackFlag;
    int ackCrc8;
    int overhearCount;

    int contentId;

    int realId;

    std::vector<VID> vids;
    std::vector<int> silentReaders;
    int shortid;

    std::vector<msgRecord> msgRecords;
    //int msgid;
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
    int parseAggAck(std::vector<VID> &vids, uint8_t *buf, int buflen);
    cPacketQueue queue;
    int queueSize;
    bool transmitting;

protected:
    cOutVector silentVec;
};

} // namespace veins

