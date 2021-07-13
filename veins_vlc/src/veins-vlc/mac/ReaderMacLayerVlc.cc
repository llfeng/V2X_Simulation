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

#include <string>
#include "veins-vlc/mac/ReaderMacLayerVlc.h"
#include "veins/base/phyLayer/MacToPhyInterface.h"
#include "veins/base/messages/MacPkt_m.h"
#include "veins-vlc/PhyLayerVlc.h"
#include "veins-vlc/messages/VlbcMacPkt_m.h"
#include "veins/modules/phy/DeciderResult80211.h"
#include "veins/base/phyLayer/PhyToMacControlInfo.h"

using namespace veins;

using std::unique_ptr;
using std::string;

Define_Module(veins::ReaderMacLayerVlc);


#define SELF_TX_MSG         0x20
#define SELF_RX_MSG         0x21

#define SELF_SENDDOWN_MSG   0x10
#define SELF_TIMEOUT_MSG    0x11
#define SELF_HELLO_MSG      0x12




void ReaderMacLayerVlc::initialize(int stage)
{
    BaseMacLayer::initialize(stage);
    if (stage == 0) {

        // TODO: port these parameters to MAC (?)
        // bit rate
        // int bitrate @unit(bps) = default(6 Mbps);
        // tx power [mW]
        // double txPower @unit(mW);

        queueSize = par("queueSize");
        transmitting = false;
        string fullpath = getFullPath();
        string prefix = "node[";
        int s_pos = fullpath.find(prefix);
        if(s_pos != string::npos){
            int e_pos = fullpath.find("]", s_pos);
            s_pos += prefix.length();
            string id_str = fullpath.substr(s_pos, e_pos-s_pos);
            int id = stoi(id_str);
            vlbcSelfAddr = id+0x80;
            realId = id+0x80;
            EV_INFO << "maclayervlc full path:" << getFullPath() << "  selfAddr:" << id << std::endl;
        }



        msgType = DISC_REQUEST;
        vlbcState = INITIAL_STATE;
        destAddr = 0;
        collisionNum = 0;
        round = 0;
        ackFlag = 0;
        ackCrc8 = 0;
        overhearCount = 0;
        ackCount = 0;
        windows = 1;
        collision_sem = 0;

        direct_uplink_count = 0;
        overhear_uplink_count = 0;
        collision_uplink_count = 0;
        first_hop_uplink_count = 0;
        relay_uplink_count = 0;

        memset(tag_vec, 0, sizeof(tag_vec));
        memset(tag_vec_direct, 0, sizeof(tag_vec_direct));
        memset(tag_vec_overhear, 0, sizeof(tag_vec_overhear));
        memset(tag_vec_first_hop, 0, sizeof(tag_vec_first_hop));
        memset(tag_vec_relay, 0, sizeof(tag_vec_relay));

        originMsg = new cMessage("TX_MSG", SELF_TX_MSG);
        cMessage *selfmsg = originMsg;
        EV_INFO << "selfmsg_ptr: " << static_cast<const void *>(selfmsg) << std::endl;
        //simtime_t first_run_time = simTime()+(dblrand()*10);
        simtime_t first_run_time = simTime()+1.0;
        EV_INFO << "first run time: " << first_run_time << endl;
        EV_TRACE << "first run time: " << first_run_time << endl;
        scheduleAt(first_run_time, selfmsg);
    }
}

void ReaderMacLayerVlc::handleUpperMsg(cMessage* msg)
{
    EV_TRACE << "Received a message from upper layer" << std::endl;
    ASSERT(dynamic_cast<cPacket*>(msg));
    cMessage *selfmsg = new cMessage("SENDDOWN_MSG", SELF_TX_MSG);
    selfmsg->setContextPointer(msg);
    scheduleAt(simTime(), selfmsg);
    //EV_INFO << "start scheduleAt" << std::endl;
    //selfmsg->isSelfMessage()
    
    //sendDown(encapsMsg(check_and_cast<cPacket*>(msg)));

    // if (transmitting) {
    //    enqueuePacket(check_and_cast<cPacket*>(msg));
    // }
    // else {
    //    sendDown(encapsMsg(check_and_cast<cPacket*>(msg)));
    //    transmitting = true;
    // }
}

void ReaderMacLayerVlc::handleLowerControl(cMessage* msg)
{
    switch (msg->getKind()) {
    case MacToPhyInterface::TX_OVER:
        // transmitting = false;
        // transmissionOpportunity();
        delete msg;
        break;
    case MacToPhyInterface::ENERGY_CHECKED:
        EV_DETAIL << "*****************energy detected, vlbcState : " << vlbcState << std::endl;
        if(vlbcState == DISC_REQ_SENT_STATE){
            collisionNum = 0;
            EV_DEBUG << "selfmsg_ptr: " << static_cast<const void *>(originMsg) << std::endl;
            if(originMsg->isScheduled() == false){
                EV_DETAIL << "msg is not scheduled, vlbcState: "<<vlbcState << std::endl;
            }else{
                EV_DETAIL << "msg is scheduled, vlbcState: "<<vlbcState << std::endl;
            }
            cancelEvent(originMsg);
            destAddr = 0;
            vlbcState = DISC_ACK_RECVD_STATE;
            cMessage *selfmsg = originMsg;
            selfmsg->setKind(SELF_TX_MSG);
            scheduleAt(simTime(), selfmsg);
        }else if(vlbcState == QUERY_REQ_SENT_STATE){
            cancelEvent(originMsg);
            scheduleAt(simTime() + 0.3, originMsg);
        }
        delete msg;
        break;
    case MacToPhyInterface::COLLISION_DETECTED:
        EV_DETAIL << "*****************collision detected, vlbcState : " << vlbcState << std::endl;
        if(vlbcState == QUERY_REQ_SENT_STATE){
            if(collision_sem){
                collision_sem = 0;
                EV_DETAIL << "path: " << getFullPath() << "collisionNum:" << collisionNum <<std::endl;
                collisionNum++;
                collision_uplink_count++;
            }
        }
        delete msg;
        break;
    default:
        EV << "ReaderMacLayerVlc does not handle control messages of this type (name was " << msg->getName() << ")\n";
        delete msg;
        break;
    }
}




void ReaderMacLayerVlc::handleLowerMsg(cMessage* msg)
{

    ASSERT(dynamic_cast<cPacket*>(msg));
    VlbcMacPkt* vlbcpkt = check_and_cast<VlbcMacPkt*>(msg);

    DeciderResult80211* result = check_and_cast<DeciderResult80211*>(PhyToMacControlInfo::getDeciderResult(msg));
    EV_DETAIL << "vlbcState :" << vlbcState << ", destAddr :" << destAddr  << ", windows :" << windows << std::endl;

    EV_DETAIL <<
            "msgType :" << vlbcpkt->getType() <<
            " sourceAddr :" << vlbcpkt->getVlbcSourceAddr() <<
            " destAddr :" << vlbcpkt->getVlbcDestAddr()<<
            " round :" << vlbcpkt->getRound() <<
            " hop :" << vlbcpkt->getHop() <<
            std::endl;
    EV_DETAIL << " responseCount :" << vlbcpkt->getResponse_count() << std::endl;
    for(int index = 0; index < vlbcpkt->getResponse_count(); index++){
        EV_DETAIL << "ackCrc8: " << vlbcpkt->getResponse_buf(index) << std::endl;
    }

    if(msg){
        VlbcMacPkt* vlbcpkt = check_and_cast<VlbcMacPkt*>(msg);

        if(vlbcpkt->getType() == DISC_ACK){
            if(vlbcState == DISC_REQ_SENT_STATE){
                for(int k = 0; k < overhearCount; k++){
                    overhear_buf[k] = 0;
                }
                overhearCount = 0;
                for(int k = 0; k < ackCount; k++){
                    ack_buf[k] = 0;
                }
                ackCount = 0;
                collisionNum = 0;
                cancelEvent(originMsg);
                destAddr = 0;
                vlbcState = DISC_ACK_RECVD_STATE;
                cMessage *selfmsg = originMsg;
                selfmsg->setKind(SELF_TX_MSG);
                scheduleAt(simTime(), selfmsg);
            }
        }else if(vlbcpkt->getType() == QUERY_RESPONSE){
            if(vlbcpkt->getVlbcDestAddr() == vlbcSelfAddr){
                if(vlbcState == QUERY_REQ_SENT_STATE){
                    direct_uplink_count++;
                    if(vlbcpkt->getVlbcRealId() > 100){
                        relay_uplink_count++;
                    }else{
                        first_hop_uplink_count++;
                    }
                    cancelEvent(originMsg);
                    EV_DETAIL << "path: " << getFullPath() << "destAddr:" << destAddr <<std::endl;
                    destAddr++;
                    vlbcState = QUERY_RSP_RECVD_STATE;
                    ackFlag = 1;
                    ackCount = vlbcpkt->getResponse_count();
                    EV_DETAIL << "ackCount: " << ackCount << std::endl;
                    for(int i = 0; i < ackCount; i++){
                        ack_buf[i] = vlbcpkt->getResponse_buf(i);
                        tag_vec[ack_buf[i]]++;
                        tag_vec_direct[ack_buf[i]]++;
                        if(vlbcpkt->getVlbcRealId() > 100){
                            tag_vec_relay[ack_buf[i]]++;
                        }else{
                            tag_vec_first_hop[ack_buf[i]]++;
                        }
                    }

                    cMessage *selfmsg = NULL;
                    EV_DETAIL << "##################vlbcState = QUERY_RSP_RECVD_STATE" << std::endl;
                    selfmsg = originMsg;
                    selfmsg->setKind(SELF_TX_MSG);
                    scheduleAt(simTime(), selfmsg);
#define WITH_OVERHEAR   1
#define SINGLE_RELAY    0
#if SINGLE_RELAY
                    sendUp(msg);
#endif
                }
            }else{
                overhear_uplink_count++;
                if(vlbcpkt->getVlbcRealId() > 100){
                    relay_uplink_count++;
                }else{
                    first_hop_uplink_count++;
                }
#if WITH_OVERHEAR
                int overhearCountOffset = overhearCount;
                for(int i = 0; i < vlbcpkt->getResponse_count(); i++){
                    int overheard_flag = 0;
                    int ack_crc8_tmp = vlbcpkt->getResponse_buf(i);

                    overheard_flag = 0;
                    for(int j = 0; j < overhearCountOffset; j++){
                        if(ack_crc8_tmp == overhear_buf[j]){
                            overheard_flag = 1;
                            break;
                        }
                    }
                    tag_vec[ack_crc8_tmp]++;
                    tag_vec_overhear[ack_crc8_tmp]++;
                    if(vlbcpkt->getVlbcRealId() > 100){
                        tag_vec_relay[ack_crc8_tmp]++;
                    }else{
                        tag_vec_first_hop[ack_crc8_tmp]++;
                    }
                    if(overheard_flag == 0){
                        overhear_buf[overhearCount] = ack_crc8_tmp;
                        overhearCount++;
                    }
                }
#if SINGLE_RELAY
                delete msg;
#endif
#endif
            }
            EV_WARN << getFullPath() << "tag_count:" << simTime() << std::endl;
            for(int i = 0; i < 30; i++){
                EV_WARN << "#" << i << "#" << tag_vec[i] << "#" << tag_vec_direct[i] << "#" << tag_vec_overhear[i] << "#" << tag_vec_first_hop[i] << "#" << tag_vec_relay[i] << std::endl;
            }
            EV_WARN << "end" << std::endl;
            EV_WARN << getFullPath() << "tag_direct_uplink_count:" << direct_uplink_count << std::endl;
            EV_WARN << getFullPath() << "tag_overhear_uplink_count:" << overhear_uplink_count << std::endl;
            EV_WARN << getFullPath() << "tag_collision_uplink_count:" << collision_uplink_count << std::endl;
            EV_WARN << getFullPath() << "tag_first_hop_uplink_count:" << first_hop_uplink_count << std::endl;
            EV_WARN << getFullPath() << "tag_relay_uplink_count:" << relay_uplink_count << std::endl;

#if SINGLE_RELAY

#else
            sendUp(msg);
#endif
        }
    }
}



void ReaderMacLayerVlc::handleSelfMsg(cMessage* msg)
{
    dblrand();
    //EV_INFO << "random:" << dblrand() << std::endl;
    cMessage *Msg = msg;
    //SelfMessage *Msg = check_and_cast<SelfMessage*>(msg);
    EV_DETAIL << "msg kind" << Msg->getKind() << ", vlbcState" << vlbcState << std::endl;
    if(Msg->getKind() == SELF_TIMEOUT_MSG){
        EV_DETAIL << "SELF_TIMEOUT_MSG" << std::endl;
        if(vlbcState == DISC_REQ_SENT_STATE){
            vlbcState = INITIAL_STATE;
            Msg->setKind(SELF_TX_MSG);
            scheduleAt(simTime()+dblrand()/4.0, Msg);

        }else if(vlbcState == QUERY_REQ_SENT_STATE){
            ackFlag = 0;
            ackCrc8 = 0;
            EV_DETAIL << "path: " << getFullPath() << "destAddr:" << destAddr <<std::endl;
            destAddr++;
            vlbcState = QUERY_RSP_RECVD_STATE;
            Msg->setKind(SELF_TX_MSG);
            scheduleAt(simTime(), Msg);
        }else{
            EV_DETAIL << "-------------------UNKOWN SELF_TIMEOUT_MSG vlbcState: " << vlbcState << std::endl;
        }
    }else if(Msg->getKind() == SELF_TX_MSG){
        if(vlbcState == INITIAL_STATE){
            EV_DETAIL << "SELF_TX_MSG vlbcState == INITIAL_STATE, windows:" << windows << std::endl;
            simtime_t busyTime = phy->getChannelState();
            if(busyTime == 0){
                cPacket* origin_pkt = new cPacket("DISC_REQ", DISC_REQUEST);

                VlbcMacPkt *pkt = encapsMsg(origin_pkt);
                sendDown(pkt);

                Msg->setKind(SELF_TIMEOUT_MSG);

                scheduleAt(simTime() + 0.001 + (8*8+pkt->getBitLength())/10000.0, Msg);
            }else{
                double randNum = dblrand()/4.0;
                EV_DETAIL << "mac layer busyTime: " << busyTime << ", tar_time: " << busyTime + randNum << endl;

                scheduleAt(busyTime + randNum, Msg);
            }
        }else if(vlbcState == DISC_ACK_RECVD_STATE){
            EV_DETAIL << "SELF_TX_MSG vlbcState == DISC_ACK_RECVD_STATE" << std::endl;
            vlbcState = QUERY_REQ_SENT_STATE;
            collision_sem = 1;
            Msg->setKind(SELF_TIMEOUT_MSG);
            scheduleAt(simTime()+0.048+0.001,  Msg);

        }else if(vlbcState == QUERY_RSP_RECVD_STATE){
            EV_DETAIL << "destAddr : " << destAddr << ", windows : " << windows << std::endl;
            if(destAddr < windows){
                cPacket* origin_pkt = new cPacket("QUERY_REQ", QUERY_REQUEST);
                VlbcMacPkt *pkt = encapsMsg(origin_pkt);
                sendDown(pkt);
                collision_sem = 1;
                Msg->setKind(SELF_TIMEOUT_MSG);
                EV_DETAIL << "mac bitlength: " <<pkt->getBitLength() << std::endl;
                scheduleAt(simTime() + 0.001 + (8*8+pkt->getBitLength())/10000.0, Msg);
                //scheduleAt(simTime() + pkt->getBitLength()/ +0.01, Msg);
                //scheduleAt(simTime() + pkt->getBitLength()/ +0.01, Msg);
            }else{
                Msg->setKind(SELF_TX_MSG);
                vlbcState = INITIAL_STATE;
                windows = 1;
                for(int i = 0; i < collisionNum; i++){
                    windows *= 2;
                }
/*
                if(collisionNum){
                    windows = 2*collisionNum;
                    EV_DETAIL << "path: " << getFullPath() << "windows:" << windows <<std::endl;
                }else{
                    windows = 1;
                }
*/
                round++;
                scheduleAt(simTime()+dblrand()/4.0, Msg);
            }
        }else{
            EV_DETAIL << "-------------------UNKOWN vlbcState: " << vlbcState << std::endl;
        }
    }else{
        EV_DETAIL << "hello world" << std::endl;
        cMessage *selfmsg = new cMessage("hello", SELF_HELLO_MSG);
        scheduleAt(simTime()+dblrand(), selfmsg);
    }
}


VlbcMacPkt* ReaderMacLayerVlc::encapsMsg(cPacket* netwPkt)
{


    VlbcMacPkt* pkt = new VlbcMacPkt(netwPkt->getName(), netwPkt->getKind());
    pkt->addBitLength(headerLength);

    EV_DETAIL << "pkt len:" << netwPkt->getByteLength() << ", headerLength:" << headerLength << std::endl;

    // TODO: setting up proper control info: according to the interfaces (?)

    pkt->setDestAddr(LAddress::L2BROADCAST());


    // set the src address to own mac address (nic module getId())
    pkt->setSrcAddr(myMacAddr);


    pkt->setVlbcSourceAddr(vlbcSelfAddr);

    currentCollision.record(collisionNum);

    if(vlbcState == INITIAL_STATE){
        pkt->setType(DISC_REQUEST);
        pkt->setVlbcDestAddr(destAddr);
        pkt->setAck_flag(ackFlag);
        pkt->setRound(round);
        pkt->setHop(0);
        pkt->setVlbcRealId(realId);
        pkt->setCollision_num(collisionNum);
        pkt->setOverhear_count(overhearCount);
        //EV_INFO << "overhear_buf:" << std::endl;
        for(int k = 0; k < overhearCount; k++){
            pkt->setOverhear_buf(k, overhear_buf[k]);
        }
        pkt->setAck_count(ackCount);
        EV_DETAIL << "ack_buf:" << std::endl;
        for(int i = 0; i < ackCount; i++){
            pkt->setAck_buf(i, ack_buf[i]);
        }
        pkt->setByteLength(5+5*(ackCount+overhearCount));
        vlbcState = DISC_REQ_SENT_STATE;
    }else if(vlbcState == QUERY_RSP_RECVD_STATE){
        pkt->setType(QUERY_REQUEST);
        pkt->setVlbcDestAddr(destAddr);
        pkt->setAck_flag(ackFlag);
        pkt->setRound(round);
        pkt->setVlbcRealId(realId);
        pkt->setHop(0);
        pkt->setCollision_num(collisionNum);
        pkt->setOverhear_count(0);
        pkt->setAck_count(ackCount);
        for(int i = 0; i < ackCount; i++){
            pkt->setAck_buf(i, ack_buf[i]);
        }
        pkt->setByteLength(5+5*(ackCount));
        ackCount = 0;
        vlbcState = QUERY_REQ_SENT_STATE;
    }
    pkt->setVlbcDestAddr(destAddr);
    pkt->setVlbcSourceAddr(vlbcSelfAddr);

    // encapsulate the network packet
    pkt->encapsulate(netwPkt);
    EV_TRACE << "pkt encapsulated\n";

    return pkt;
}

void ReaderMacLayerVlc::enqueuePacket(cPacket* pkt)
{
    if (queueSize != -1 && queue.getLength() >= queueSize) {
        // emit(sigDropped, true);
        delete pkt;
        return; // TODO send FULL QUEUE message
    }
    queue.insert(pkt);
    // emit(sigQueueLength, queue.getLength());
}

void ReaderMacLayerVlc::transmissionOpportunity()
{
    if (queue.isEmpty()) {
        return;
    }

    sendDown(encapsMsg(queue.pop()));
    // emit(sigQueueLength, queue.getLength());
    transmitting = true;
}

