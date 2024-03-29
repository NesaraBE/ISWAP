/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Niket Agarwal
 *          Tushar Krishna
 */


#include "mem/ruby/network/garnet2.0/flit.hh"

// Constructor for the flit
flit::flit(int id, int  vc, int vnet, RouteInfo route, int size,
    MsgPtr msg_ptr, Cycles curTime)
{
    m_size = size;
    m_msg_ptr = msg_ptr;
    m_enqueue_time = curTime;
    m_dequeue_time = curTime;
    m_time = curTime;
    m_id = id;
    m_vnet = vnet;
    m_vc = vc;
    m_route = route;
    m_stage.first = I_;
    m_stage.second = m_time;

    if (size == 1) {
        m_type = HEAD_TAIL_;
        return;
    }
    // Loupe
    // if (id == 0)
    if (id%8 == 0)
        m_type = HEAD_;
    else if (id == (size - 1))
        m_type = TAIL_;
    else
        m_type = BODY_;

    // Loupe - unclear
    m_type = CREDIT_HEAD_;
}

// Flit can be printed out for debugging purposes
void
flit::print(std::ostream& out) const
{
    // Loupe
    // out << "[flit:: ";
    // out << "Id=" << m_id << " ";
    // out << "Type=" << m_type << " ";
    // out << "Vnet=" << m_vnet << " ";
    // out << "VC=" << m_vc << " ";
    // out << "Src NI=" << m_route.src_ni << " ";
    // out << "Src Router=" << m_route.src_router << " ";
    // out << "Dest NI=" << m_route.dest_ni << " ";
    // out << "Dest Router=" << m_route.dest_router << " ";
    // out << "Enqueue Time=" << m_enqueue_time << " ";
    // out << "]";
    out << "flit,";
    out << m_id << ",";
    out << m_type << ","; //
    out << m_vnet << ","; //
    out << m_vc << ",";
    out << m_route.src_router << ","; //
    out << m_route.dest_router << ","; //
    out << m_enqueue_time;
}

bool
flit::functionalWrite(Packet *pkt)
{
    Message *msg = m_msg_ptr.get();
    return msg->functionalWrite(pkt);
}

//SWAP_GARNET_2.0_MERGE
void
flit::set_outport_dir(PortDirection dir)
{
	m_outport_dir = dir;
}

PortDirection
flit::get_outport_dir()
{
	return m_outport_dir;
}
/*
void flit::deadlockSnapshot(int routerNum) {

   

    std::cout << "            " << "Flit ID: " << m_id << "\n";
    std::cout << "            ";
    std::cout << "Type=" << m_type << " ";
    std::cout << "Vnet=" << m_vnet << " ";
    std::cout << "VC=" << m_vc << " ";
    std::cout << "Src NI=" << m_route.src_ni << " ";
    std::cout << "Src Router=" << m_route.src_router << " ";
    std::cout << "Dest NI=" << m_route.dest_ni << " ";
    std::cout << "Dest Router=" << m_route.dest_router << " \n";
}
*/
