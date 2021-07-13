//
// Copyright (C) 2016 Alexander Brummer <alexander.brummer@fau.de>
//
// Documentation for these modules is at http://veins.car2x.org/
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

#include "veins/base/phyLayer/Antenna.h"
#include "veins/base/phyLayer/lambertian.h"
using namespace veins;

double Antenna::getGain(Coord senderPos, Coord senderOrient, Coord receiverPos, Coord receiverOrient){
    EV_DETAIL << "senderOrient.x: "<< senderOrient.x << ", receiverOrient.x:" << receiverOrient.x << endl;
    EV_DETAIL << "senderPos.x: "<< senderPos.x << ", receiverPos.x:" << receiverPos.x << endl;
    EV_DETAIL << "senderPos.y: "<< senderPos.y << ", receiverPos.y:" << receiverPos.y << endl;
    EV_DETAIL << "dist:"  << (senderPos.x - receiverPos.x)*(senderPos.x - receiverPos.x) + (senderPos.y - receiverPos.y)*(senderPos.y - receiverPos.y)<< endl;
    double res = 0.0;
    if(senderOrient.x * receiverOrient.x < 0){
        if(receiverOrient.x > 0){       //receiver is reader, tag is sender  uplink
            if(senderPos.x > receiverPos.x){
                int uplink_dist = 81;       //1k-81, 500-90, 250-98, 125-101
                if(senderPos.y < 11.25){
                    uplink_dist = 50;       //1k-50, 500-55, 250-59, 125-61
                }

                if(senderPos.y > 11.25 && senderPos.y < 12.2){
                    uplink_dist = 44;
                }
                EV_DETAIL << "senderPos:" << senderPos.y << std::endl;
                EV_DETAIL << "uplink_dist:" << uplink_dist << std::endl;
                //if(is_connected(receiverPos.x, receiverPos.y, senderPos.x, senderPos.y,  120, (120/2*PI/180))){
                //if(is_connected(receiverPos.x, receiverPos.y, senderPos.x, senderPos.y,  uplink_dist, (120/2*PI/180))){
                if(
                        is_connected(receiverPos.x, receiverPos.y - 3.75/2.0, senderPos.x, senderPos.y,  uplink_dist, (40/2*PI/180)) ||
                        is_connected(receiverPos.x, receiverPos.y + 3.75/2.0, senderPos.x, senderPos.y,  uplink_dist, (40/2*PI/180))
                        ){
                    res = 1.0;
                }else{
                    res = 0.0;
                }
            }else{
                res = 0.0;
            }
        }else{                      //receiver is tag, reader is sender downlink
            if(senderPos.x < receiverPos.x){
                //if(is_connected(senderPos.x, senderPos.y, receiverPos.x, receiverPos.y,  150, (40/2*PI/180))){
                if(
                        is_connected(senderPos.x, senderPos.y - 3.75/2.0, receiverPos.x, receiverPos.y,  120, (120/2*PI/180)) ||
                        is_connected(senderPos.x, senderPos.y + 3.75/2.0, receiverPos.x, receiverPos.y,  120, (120/2*PI/180))
                        ){
                //if(is_connected(senderPos.x, senderPos.y, receiverPos.x, receiverPos.y,  150, (120/2*PI/180))){
                //if((senderPos.x - receiverPos.x)*(senderPos.x - receiverPos.x) + (senderPos.y - receiverPos.y)*(senderPos.y - receiverPos.y)  < 10000){
                    res = 1.0;
                }else{
                    res = 0.0;
                }
            }else{
                res = 0.0;
            }
        }
    }else{
        res = 0.0;
    }
    EV_DETAIL << "antenna gain: "<< res << endl;
    return res;
}

double Antenna::getSenderGain(Coord ownPos, Coord ownOrient, Coord otherPos)
{

    //is_connected(ownPosvehicle_x, double vehicle_y, double tag_x, double tag_y, double max_distance, double fov)
    bool res = false;


    if(ownOrient.x > 0){        //sender is reader
        if(ownPos.x < otherPos.x && (ownPos.x - otherPos.x)*(ownPos.x - otherPos.x) + (ownPos.y - otherPos.y)*(ownPos.y - otherPos.y)  < 10000){
            res = true;
        }else{
            EV_DETAIL << "ownPos.x: "<< ownPos.x << ", otherPos.x:" << otherPos.x << ", dist:"  << (ownPos.x - otherPos.x)*(ownPos.x - otherPos.x) + (ownPos.y - otherPos.y)*(ownPos.y - otherPos.y)<< endl;
            res = false;
        }
//        res = is_connected(ownPos.x, ownPos.y, otherPos.x, otherPos.y, 100, (120/2*PI/180));
    }else{                      //sender is tag
        res = is_connected(otherPos.x, otherPos.y, ownPos.x, ownPos.y,  100, (120/2*PI/180));
    }
    //is_connected(, double vehicle_y, double tag_x, double tag_y, double max_distance, double fov)
    //get_xaxis_range(ownPos.y, otherPos.x, otherPos.y, 90, l, r);

//    EV_INFO << "Sender's pos(" << ownPos.x << "," << ownPos.y << "," << ownPos.z << ")"<< endl;
//    EV_INFO << "Sender's orient(" << ownOrient.x << "," << ownOrient.y << "," << ownOrient.z << ")"<< endl;
//    EV_INFO << "Receiver's pos(" << otherPos.x << "," << otherPos.y << "," << otherPos.z << ")"<< endl;
    // as this base class represents an isotropic antenna, simply return 1.0
    if(res){
        EV_DETAIL << "in range"<< endl;
        return 1.0;
    }else{
        EV_DETAIL << "not in range"<< endl;
        return 0.0;
    }
}


double Antenna::getReceiverGain(Coord ownPos, Coord ownOrient, Coord otherPos)
{

    bool res = false;

//    if(ownOrient.x > 0){        //receiver is reader
//        if(otherPos.x > ownPos.x){
//            res = true;
//        }else{
//            res = false;
//        }
//    }else{                      //receiver is tag
//        if(otherPos.x < ownPos.x){
//            res = true;
//        }else{
//            res = false;
//        }
//    }

    if(ownOrient.x > 0){
        if(otherPos.x > ownPos.x){
            res = true;
        }else{
            res = false;
        }
    }else{                      //receiver is tag
        if(otherPos.x < ownPos.x){
            res = true;
        }else{
            res = false;
        }
    }

    //is_connected(, double vehicle_y, double tag_x, double tag_y, double max_distance, double fov)
    //get_xaxis_range(ownPos.y, otherPos.x, otherPos.y, 90, l, r);

//    EV_INFO << "Receiver's pos(" << ownPos.x << "," << ownPos.y << "," << ownPos.z << ")"<< endl;
//    EV_INFO << "Receiver's orient(" << ownOrient.x << "," << ownOrient.y << "," << ownOrient.z << ")"<< endl;
//    EV_INFO << "Sender's pos(" << otherPos.x << "," << otherPos.y << "," << otherPos.z << ")"<< endl;
    // as this base class represents an isotropic antenna, simply return 1.0
    if(res){
        EV_DETAIL << "in range"<< endl;
        return 1.0;
    }else{
        EV_DETAIL << "not in range"<< endl;
        return 0.0;
    }
}

