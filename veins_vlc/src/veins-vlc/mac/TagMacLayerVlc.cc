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
#include "veins-vlc/mac/TagMacLayerVlc.h"
#include "veins/base/phyLayer/MacToPhyInterface.h"
#include "veins/base/messages/MacPkt_m.h"
#include "veins-vlc/PhyLayerVlc.h"
#include "veins-vlc/messages/VlbcMacPkt_m.h"
#include "veins/modules/phy/DeciderResult80211.h"
#include "veins/base/phyLayer/PhyToMacControlInfo.h"



using namespace veins;

using std::unique_ptr;
using std::string;

Define_Module(veins::TagMacLayerVlc);


#define SELF_TX_MSG         0x20
#define SELF_RX_MSG         0x21
#define NEW_MSG             0x22

#define SELF_SENDDOWN_MSG   0x10
#define SELF_TIMEOUT_MSG    0x11
#define SELF_HELLO_MSG      0x12




void TagMacLayerVlc::initialize(int stage)
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
            contentId = id + 0x90;
            realId = id+0x80;
//            ackCrc8 = id + 0x90 - 1;
//            msgid = ackCrc8;
            EV_INFO << "maclayervlc full path:" << getFullPath() << "  selfAddr:" << id << "contentId:" <<contentId<< std::endl;
        }
        prefix = "rsu[";
        s_pos = fullpath.find(prefix);
        if(s_pos != string::npos){
            int e_pos = fullpath.find("]", s_pos);
            s_pos += prefix.length();
            string id_str = fullpath.substr(s_pos, e_pos-s_pos);
            int id = stoi(id_str);
            contentId = id;
            realId = id;
            //msgid = id - 1;
            EV_INFO << "maclayervlc full path:" << getFullPath() << "  selfAddr:" << id << std::endl;
        }
        vlbcSelfAddr = 0;
        round = 0;
        shortid = 0;

        phy->setRadioState(0);
        cMessage *selfmsg = new cMessage("NEW_MSG", NEW_MSG);
        scheduleAt(simTime(), selfmsg);
    }
}

void TagMacLayerVlc::handleUpperMsg(cMessage* msg)
{
    int hop_threshold = 10;
    EV_DETAIL << "+++++++++++++Received a message from upper layer" << std::endl;
    //EV_TRACE << "Received a message from upper layer" << std::endl;
    ASSERT(dynamic_cast<cPacket*>(msg));
    VlbcMacPkt* vlbcpkt = check_and_cast<VlbcMacPkt*>(msg);

    int dest_addr_tmp = vlbcpkt->getVlbcDestAddr();
    int round_tmp = vlbcpkt->getRound();
    int src_addr_tmp = vlbcpkt->getVlbcSourceAddr();
    int hop_tmp = vlbcpkt->getHop() + 1;

    EV_DETAIL <<""
            " sourceAddr :" << src_addr_tmp <<
            " +++destAddr :" << dest_addr_tmp<<
            " round :" << round_tmp <<
            " hop :" << hop_tmp <<
            std::endl;

#define RELAY 1

#if RELAY
    int response_count_tmp = vlbcpkt->getResponse_count();
    for(int n = 0; n < response_count_tmp; n++){
        int ack_crc8_tmp = vlbcpkt->getResponse_buf(n);
        int hop_tmp = 0;

        if(ack_crc8_tmp < 128 && hop_tmp < hop_threshold){
            std::vector<msgRecord>::iterator i = msgRecords.begin();
            for(; i != msgRecords.end(); i++){
                if(i->msgid == ack_crc8_tmp){
                    break;
                }
            }
            if(i == msgRecords.end()){
                msgRecord msg_record(ack_crc8_tmp);
                msgRecords.push_back(msg_record);
            }
        }
    }
#endif
    delete msg;
}


void TagMacLayerVlc::handleLowerControl(cMessage* msg)
{
    switch (msg->getKind()) {
    case MacToPhyInterface::TX_OVER:
    {
        EV_INFO << "TX_OVER" << std::endl;
        phy->setRadioState(0);
        cMessage *selfmsg = new cMessage("SELF_TX", SELF_TX_MSG);
        scheduleAt(simTime(), selfmsg);
    }
        break;
    default:
        EV_DETAIL << "BaseMacLayer does not handle control messages of this type (name was " << msg->getName() << ")\n";
        delete msg;
        break;
    }   
}



void TagMacLayerVlc::handleLowerMsg(cMessage* msg)
{
    EV_DETAIL << "Received a message from lower layer" << std::endl;
    ASSERT(dynamic_cast<cPacket*>(msg));

    if(phy->getRadioState()){
        delete msg;
    }else{
        VlbcMacPkt* vlbcpkt = check_and_cast<VlbcMacPkt*>(msg);
        DeciderResult80211* result = check_and_cast<DeciderResult80211*>(PhyToMacControlInfo::getDeciderResult(msg));

        EV_DETAIL << "recvd msg\r\n" <<
                "msgType :" << vlbcpkt->getType() <<
                " sourceAddr :" << vlbcpkt->getVlbcSourceAddr() <<
                " destAddr :" << vlbcpkt->getVlbcDestAddr()<<
                " ackFlag :" << vlbcpkt->getAck_flag() <<
                " round :" << vlbcpkt->getRound() <<
                " hop :" << vlbcpkt->getHop() <<
                " collisionNum :" << vlbcpkt->getCollision_num() <<
                std::endl;
        EV_DETAIL << " overhearCount :" << vlbcpkt->getOverhear_count() << std::endl;
        for(int index = 0; index< vlbcpkt->getOverhear_count(); index++){
            EV_DETAIL << "ackCrc8: " << vlbcpkt->getOverhear_buf(index) << std::endl;
        }


        EV_DETAIL << " ackCount :" << vlbcpkt->getAck_count() << std::endl;
        for(int index = 0; index < vlbcpkt->getAck_count(); index++){
            EV_DETAIL << "ackCrc8: " << vlbcpkt->getAck_buf(index)<< std::endl;
        }

        if(vlbcpkt->getType() == DISC_REQUEST){
            //overhear ack
            int overhear_count_tmp = vlbcpkt->getOverhear_count();
            EV_DETAIL << "overhear:"<<std::endl;
            for(int index = 0; index< overhear_count_tmp; index++){
                int ack_ackCrc8 = vlbcpkt->getOverhear_buf(index);
                for(std::vector<msgRecord>::iterator i = msgRecords.begin(); i != msgRecords.end(); i++){
                    EV_DETAIL << "msgid: " << i->msgid << ", ack_ackCrc8: "<<ack_ackCrc8  << std::endl;
                    int reader_acked_flag = 0;
                    if(i->msgid == ack_ackCrc8){
                        i->silentReaders.push_back(std::make_pair(vlbcpkt->getVlbcSourceAddr(), simTime()));
                        reader_acked_flag = 1;
                    }
                    if(reader_acked_flag){
                        break;
                    }
                }
            }


            //batch ack
            EV_DETAIL << "ack_buf:"<<std::endl;
            int ack_count_tmp = vlbcpkt->getAck_count();
            for(int index = 0; index< ack_count_tmp; index++){
                int ack_ackCrc8 = vlbcpkt->getAck_buf(index);

                EV_DETAIL << "msgRecords size:" << msgRecords.size() << std::endl;

                for(std::vector<msgRecord>::iterator i = msgRecords.begin(); i != msgRecords.end(); i++){
                    EV_DETAIL << "--msgid:" << i->msgid << std::endl;
                    int reader_acked_flag = 0;
                    if(i->msgid == ack_ackCrc8){
                        i->silentReaders.push_back(std::make_pair(vlbcpkt->getVlbcSourceAddr(), simTime()));
                        reader_acked_flag = 1;
                    }
                    if(reader_acked_flag){
                        break;
                    }
                }
            }

            int reader_noacked_flag = 1;
            if(msgRecords.size() == 0){
                reader_noacked_flag = 0;
            }
            for(std::vector<msgRecord>::iterator i = msgRecords.begin(); i != msgRecords.end(); i++){
                reader_noacked_flag = 1;
                EV_DETAIL << "msgid:" << i->msgid << std::endl;
                for(std::vector<std::pair<int, simtime_t> >::iterator j = i->silentReaders.begin(); j != i->silentReaders.end(); j++){
                    EV_DETAIL << "silentReader: " << j->first << ", recvd reader: " <<vlbcpkt->getVlbcSourceAddr() << std::endl;
                    if(j->first == vlbcpkt->getVlbcSourceAddr()){
                        reader_noacked_flag = 0;
                        break;
                    }
                }
                if(reader_noacked_flag){
                    break;
                }
            }

            if(reader_noacked_flag){
                vlbcSelfAddr = vlbcpkt->getVlbcSourceAddr();
                destAddr = vlbcpkt->getVlbcDestAddr();
                msgType = vlbcpkt->getType();
                collisionNum = vlbcpkt->getCollision_num();

                windows = 1;
                for(int i = 0; i < collisionNum; i++){
                    windows *= 2;
                }
/*
                if(collisionNum){
                    windows = 2*collisionNum;
                }else{
                    windows = 1;
                }
*/
                shortid = intrand(windows);
                EV_DETAIL << "assigned shortid:" << shortid << std::endl;
                round = vlbcpkt->getRound();
                vlbcState = DISC_REQ_RECVD_STATE;
                cMessage *selfmsg = NULL;
                selfmsg = new cMessage("TX_MSG", SELF_TX_MSG);
                scheduleAt(simTime(), selfmsg);
            }
        }else if(vlbcpkt->getType() == QUERY_REQUEST){
            EV_DETAIL << "Tag, vlbcSelfAddr :" << vlbcSelfAddr <<
                    " destAddr :" << shortid<<
                    " received_ackCrc8 :" << ackCrc8 <<
                    " round :" << round <<
                    std::endl;

            //batch ack
            EV_DETAIL << "ack_buf:"<<std::endl;
            int ack_count_tmp = vlbcpkt->getAck_count();
            for(int index = 0; index< ack_count_tmp; index++){
                int ack_ackCrc8 = vlbcpkt->getAck_buf(index);
                for(std::vector<msgRecord>::iterator i = msgRecords.begin(); i != msgRecords.end(); i++){
                    EV_DETAIL << "msgid:" << i->msgid;
                    int reader_acked_flag = 0;
                    if(i->msgid == ack_ackCrc8){
                        i->silentReaders.push_back(std::make_pair(vlbcpkt->getVlbcSourceAddr(), simTime()));
                        reader_acked_flag = 1;
                    }
                    if(reader_acked_flag){
                        break;
                    }
                }
            }

            int query_readerAddr = vlbcpkt->getVlbcSourceAddr();
            int query_round = vlbcpkt->getRound();
            int query_shortid = vlbcpkt->getVlbcDestAddr();
            VID vid(query_readerAddr, query_round, query_shortid, 0);
            VID vid_tag(vlbcSelfAddr, round, shortid, 0);
            if(vid_tag == vid){ //query
                vlbcState = QUERY_REQ_RECVD_STATE;
                cMessage *selfmsg = NULL;
                selfmsg = new cMessage("TX_MSG", SELF_TX_MSG);
                scheduleAt(simTime(), selfmsg);
            }
        }
/*
        if(getFullPath().find("rsu") != string::npos){
            int reader_num = 60;
            int res[reader_num] = {0};
            EV_INFO << "tag:" << getFullPath() << ", reader id:" << std::endl;
            for(std::vector<msgRecord>::iterator i = msgRecords.begin(); i != msgRecords.end(); i++){
                EV_DETAIL << "msgid: " << i->msgid << std::endl;
                for(std::vector<std::pair<int, simtime_t> >::iterator j = i->silentReaders.begin(); j != i->silentReaders.end(); j++){
                    res[j->first-128] = 1;
                }
            }
            for(int i = 0; i < reader_num; i++){
                if(res[i]){
                    EV_INFO << i+128 << " ok" << std::endl;
                }else{
                    EV_INFO << i+128 << " miss" << std::endl;
                }
            }
            EV_INFO << "end" << std::endl;
        }
        */
    }

}

void TagMacLayerVlc::handleSelfMsg(cMessage* msg)
{
    cMessage *Msg = msg;
    EV_DETAIL << "==================msg kind" << Msg->getKind() << ", vlbcState:" << vlbcState << std::endl;
    if(Msg->getKind() == SELF_TIMEOUT_MSG){

    }else if(Msg->getKind() == SELF_TX_MSG){
        if(vlbcState == DISC_REQ_RECVD_STATE){
            cPacket* origin_pkt = new cPacket("DISC_ACK", DISC_ACK);
            VlbcMacPkt *pkt = encapsMsg(origin_pkt);
            sendDown(pkt);
            if(shortid == 0){
                EV_DETAIL << "change state to QUERY_REQ_RECVD_STATE" << std::endl;
                vlbcState = QUERY_REQ_RECVD_STATE;
            }
        }else if(vlbcState == QUERY_REQ_RECVD_STATE){
            EV_DETAIL << "send QUERY_RESPONSE" << Msg->getKind() << std::endl;
            cPacket* origin_pkt = new cPacket("QEURY_RSP", QUERY_RESPONSE);
            VlbcMacPkt *pkt = encapsMsg(origin_pkt);
            sendDown(pkt);
        }
    }else if(Msg->getKind() == NEW_MSG){
        if(getFullPath().find("rsu[") != string::npos){
            msgRecord msg_record(contentId);
            msgRecords.push_back(msg_record);
        }
        delete Msg;
/*
        string fullpath = getFullPath();
        if(fullpath.find("rsu[") == string::npos){
            if(fullpath.find("node[") != string::npos){
                cMessage *selfmsg = new cMessage("NEW_MSG", NEW_MSG);
                scheduleAt(simTime()+10, selfmsg);
            }
        }
*/

//        if(node is vechile){
//        cMessage *selfmsg = NULL;
//        selfmsg = new cMessage("NEW_MSG", NEW_MSG);
//        scheduleAt(simTime()+10, selfmsg);
//        }
    }
}


VlbcMacPkt* TagMacLayerVlc::encapsMsg(cPacket* netwPkt)
{

    VlbcMacPkt* pkt = new VlbcMacPkt(netwPkt->getName(), netwPkt->getKind());
    pkt->addBitLength(headerLength);

    // TODO: setting up proper control info: according to the interfaces (?)

    pkt->setDestAddr(LAddress::L2BROADCAST());
    // set the src address to own mac address (nic module getId())
    pkt->setSrcAddr(myMacAddr);

    if(vlbcState == DISC_REQ_RECVD_STATE){
        pkt->setType(DISC_ACK);
        pkt->setVlbcSourceAddr(0);
        pkt->setVlbcDestAddr(0);
        pkt->setAck_flag(0);
//        pkt->setAck_crc8(0);
        pkt->setRound(0);
        pkt->setHop(0);
        pkt->setCollision_num(0);
        pkt->setOverhear_count(0);
        pkt->setVlbcRealId(realId);
        vlbcState = DISC_ACK_SENT_STATE;
        pkt->setByteLength(0);
    }else if(vlbcState == QUERY_REQ_RECVD_STATE){
        int pktlen = 5;
        pkt->setVlbcRealId(realId);
        pkt->setType(QUERY_RESPONSE);
        pkt->setVlbcSourceAddr(shortid);
        pkt->setVlbcDestAddr(vlbcSelfAddr);
        pkt->setRound(round);
        pkt->setHop(0);
        pkt->setAck_flag(0);
//        pkt->setAck_crc8(contentId);
        pkt->setCollision_num(0);
        int oCount = 0;
        for(std::vector<msgRecord>::iterator i = msgRecords.begin(); i != msgRecords.end(); i++){
            int silentFlag = 0;
            for(std::vector<std::pair<int, simtime_t> >::iterator j = i->silentReaders.begin(); j != i->silentReaders.end(); j++){
                if(j->first == vlbcSelfAddr){
                    silentFlag = 1;
                    break;
                }
            }

            if(silentFlag == 0){
                pkt->setResponse_buf(oCount, i->msgid);
                oCount++;
            }
        }
        pktlen += oCount*4;
        pkt->setResponse_count(oCount);
        pkt->setByteLength(pktlen);
        EV_DETAIL << "======+++++tag send responseCount :" << pkt->getResponse_count() << std::endl;
        for(int index = 0; index < pkt->getResponse_count(); index++){
            EV_DETAIL << "msgid: " << pkt->getResponse_buf(index) << std::endl;
        }

        vlbcState = QUERY_RSP_SENT_STATE;
    }

    // encapsulate the network packet
    pkt->encapsulate(netwPkt);
    EV_TRACE << "pkt encapsulated\n";
    return pkt;
}

void TagMacLayerVlc::enqueuePacket(cPacket* pkt)
{
    if (queueSize != -1 && queue.getLength() >= queueSize) {
        // emit(sigDropped, true);
        delete pkt;
        return; // TODO send FULL QUEUE message
    }
    queue.insert(pkt);
    // emit(sigQueueLength, queue.getLength());
}

void TagMacLayerVlc::transmissionOpportunity()
{
    if (queue.isEmpty()) {
        return;
    }

    sendDown(encapsMsg(queue.pop()));
    // emit(sigQueueLength, queue.getLength());
    transmitting = true;
}

