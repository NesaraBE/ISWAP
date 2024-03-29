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


#ifndef __MEM_RUBY_NETWORK_GARNET2_0_COMMONTYPES_HH__
#define __MEM_RUBY_NETWORK_GARNET2_0_COMMONTYPES_HH__
#define MY_PRINT 0
#define INTERPOSR_RANDOM_ROUTING_EN 1
#include "mem/ruby/common/NetDest.hh"

// All common enums and typedefs go here

// enum flit_type {HEAD_, BODY_, TAIL_, HEAD_TAIL_, NUM_FLIT_TYPE_};
// Loupe
enum flit_type {HEAD_, BODY_, TAIL_, HEAD_TAIL_, CREDIT_HEAD_, NUM_FLIT_TYPE_};
enum VC_state_type {IDLE_, VC_AB_, ACTIVE_, NUM_VC_STATE_TYPE_};
enum VNET_type {CTRL_VNET_, DATA_VNET_, NULL_VNET_, NUM_VNET_TYPE_};
enum flit_stage {I_, VA_, SA_, ST_, LT_, NUM_FLIT_STAGE_};
// Loupe
// enum link_type { EXT_IN_, EXT_OUT_, INT_, NUM_LINK_TYPES_ };
enum RoutingAlgorithm { TABLE_ = 0, XY_ = 1, TURN_MODEL_OBLIVIOUS_ = 2,
                        TURN_MODEL_ADAPTIVE_ = 3, RANDOM_OBLIVIOUS_ = 4,
                        RANDOM_ADAPTIVE_ = 5, CUSTOM_ = 6,
                        NUM_ROUTING_ALGORITHM_};
//SWAP_GARNET_2.0_MERGE
enum when_to_swap { TDM_ = 1, _2_TDM_ = 2, _4_TDM_ = 3, _8_TDM_ = 4,
                    _16_TDM_ =5, _32_TDM_ = 6, _64_TDM_ = 7, _512_TDM_ = 8,
					_1024_TDM_ = 9, _2048_TDM_ = 10 };
enum which_to_swap { DISABLE_LOCAL_SWAP_ = 1, ENABLE_LOCAL_SWAP_ = 2 };

struct RouteInfo
{
    // destination format for table-based routing
    int vnet;
    NetDest net_dest;

    // src and dest format for topology-specific routing
    int src_ni;
    // Loupe
    int src_router;
    int dest_ni;
    int dest_router;
    int hops_traversed;
};

#define INFINITE_ 10000

#endif //__MEM_RUBY_NETWORK_GARNET2_0_COMMONTYPES_HH__
