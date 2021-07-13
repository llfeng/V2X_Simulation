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


cplusplus {{
#include "veins/base/utils/SimpleAddress.h"
#include "veins/base/messages/MacPkt_m.h"

//FRAME FIELD DEFINE
#define DEFAULT_FRAME_TYPE    0x00
#define DISC_REQUEST    0x01
#define DISC_RESPONSE   0x02
#define QUERY_REQUEST   0x03
#define QUERY_RESPONSE  0x04
#define DISC_ACK        0x05


}}


namespace veins;
//class veins::MacPkt;
packet veins::MacPkt;

packet VlbcMacPkt extends veins::MacPkt{
    int vlbcRealId;
	int vlbcSourceAddr;
	int vlbcDestAddr;
	int type;
    int ack_flag;
    int round;
    //int ack_crc8;
    int collision_num;
    int hop;
    int overhear_count;
    int overhear_buf[1024];
    int ack_count;
    int ack_buf[1024];
    int response_count;
    int response_buf[1024];
}
