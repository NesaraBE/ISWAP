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


#ifndef __MEM_RUBY_NETWORK_GARNET2_0_ROUTER_HH__
#define __MEM_RUBY_NETWORK_GARNET2_0_ROUTER_HH__

#include <iostream>
#include <vector>

#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/common/NetDest.hh"
#include "mem/ruby/network/BasicRouter.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "mem/ruby/network/garnet2.0/GarnetNetwork.hh"
#include "mem/ruby/network/garnet2.0/flit.hh"
#include "params/GarnetRouter.hh"

class NetworkLink;
class CreditLink;
class InputUnit;
class OutputUnit;
class RoutingUnit;
class SwitchAllocator;
class CrossbarSwitch;
class FaultModel;

class Router : public BasicRouter, public Consumer
{
  public:
    typedef GarnetRouterParams Params;
    Router(const Params *p);

    ~Router();

    void wakeup();
    void print(std::ostream& out) const {};

    void init();
    void addInPort(PortDirection inport_dirn, NetworkLink *link,
                   CreditLink *credit_link);
    void addOutPort(PortDirection outport_dirn, NetworkLink *link,
                    const NetDest& routing_table_entry,
                    int link_weight, CreditLink *credit_link);

    Cycles get_pipe_stages(){ return m_latency; }
    int get_num_vcs()       { return m_num_vcs; }
    int get_num_vnets()     { return m_virtual_networks; }
    int get_vc_per_vnet()   { return m_vc_per_vnet; }
    int get_num_inports()   { return m_input_unit.size(); }
    int get_num_outports()  { return m_output_unit.size(); }
    int get_id()            { return m_id; }
    bool has_free_vc(int outport, int vnet);
    int get_numFreeVC(PortDirection dirn_);

    void vcStateDump(void);

    void init_net_ptr(GarnetNetwork* net_ptr)
    {
        m_network_ptr = net_ptr;
    }

    GarnetNetwork* get_net_ptr()                    { return m_network_ptr; }
    std::vector<InputUnit *>& get_inputUnit_ref()   { return m_input_unit; }
    std::vector<OutputUnit *>& get_outputUnit_ref() { return m_output_unit; }
    RoutingUnit* get_routingUnit_ref() {     return  m_routing_unit; }
    PortDirection getOutportDirection(int outport);
    PortDirection getInportDirection(int inport);

    int route_compute(RouteInfo route, int inport, PortDirection direction,
                      int vc);
    //int route_compute(RouteInfo route, int inport, PortDirection direction);
    void grant_switch(int inport, flit *t_flit);
    void schedule_wakeup(Cycles time);

    std::string getPortDirectionName(PortDirection direction);
    void printFaultVector(std::ostream& out);
    void printAggregateFaultProbability(std::ostream& out);

    void regStats();
    void collateStats();
    void resetStats();

    // For Fault Model:
    bool get_fault_vector(int temperature, float fault_vector[]) {
        return m_network_ptr->fault_model->fault_vector(m_id, temperature,
                                                        fault_vector);
    }
    bool get_aggregate_fault_probability(int temperature,
                                         float *aggregate_fault_prob) {
        return m_network_ptr->fault_model->fault_prob(m_id, temperature,
                                                      aggregate_fault_prob);
    }

    // Loupe
    // Added by Hsin. Function to print out snapshop when deadlock is detected.
    void deadlockSnapshot();

    uint32_t functionalWrite(Packet *);

    bool checkSwapPtrValid();
    void makeSwapPtrValid(PortDirection dirn, int vcid);
    // InterSwap
    // 'is_swap' to avoid downstram router taking part in
    // swap on the request of upstream router
	bool is_swap;
    // this is 'swap_ptr' structure; currently vcid is NOT
    // being used as we are using inj-vc=0 and vcs-per-vnet=0
	struct
	{
		bool valid;
		int inport;
		int vcid; // not used currently
		// direction of the inport to which its
		// pointing
		int vnet_id;
		PortDirection inport_dirn;
	} swap_ptr;

    void movSwapPtr();
    bool outportNotLocal();
	// Router's doSwap function: it will check if the queue
	// is empty or not; of empty then return NULL otherwise
	// return the head-flit from that input-queue
	flit* doSwap(PortDirection inport_dirn, int vcid);

    // this will enqueue the flit into the input queue
    void doSwap_enqueue(flit* flit_t, PortDirection inport_dirn, int inport_id, int vcid);

    void scanRouter( void );

    // this will anytime point to the inport that router is working on
    uint32_t curr_inport;

    inline bool
    get_send_routedSwap() {
        return send_routedSwap;
    }

    void
    set_send_routedSwap(bool val) {
      send_routedSwap = val;
    }

    inline bool
    is_myTurn() {

        assert(get_net_ptr()->isEnableInterswap() == true);

        int tdm_ = get_net_ptr()->get_whenToSwap();

        if (curCycle()%(tdm_*(get_net_ptr()->getNumRouters())) == m_id) {
            return true;
        } else {
            return false;
        }

        if (get_net_ptr()->get_whenToSwap() == TDM_) {
            if (curCycle()%(get_net_ptr()->getNumRouters()) == m_id) {
                return true;
            } else {
                return false;
            }

        } else if (get_net_ptr()->get_whenToSwap() == _2_TDM_) {
            if (curCycle()%(2*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        } else if (get_net_ptr()->get_whenToSwap() == _4_TDM_) {
            if (curCycle()%(4*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        } else if (get_net_ptr()->get_whenToSwap() == _8_TDM_) {
            if (curCycle()%(8*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        } else if (get_net_ptr()->get_whenToSwap() == _16_TDM_) {
            if ((curCycle())%(16*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        } else if (get_net_ptr()->get_whenToSwap() == _32_TDM_) {
            if ((curCycle())%(32*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        }  else if (get_net_ptr()->get_whenToSwap() == _64_TDM_) {
            if ((curCycle())%(64*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        }  else if (get_net_ptr()->get_whenToSwap() == _512_TDM_) {
            if ((curCycle())%(512*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        }  else if (get_net_ptr()->get_whenToSwap() == _1024_TDM_) {
            if ((curCycle())%(1024*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        }  else if (get_net_ptr()->get_whenToSwap() == _2048_TDM_) {
            if ((curCycle())%(2048*(get_net_ptr()->getNumRouters())) == m_id) {
                return true;
            } else {
                return false;
            }

        }	else {
            // there has to be some policy on when to Swap.. if control
            // comes here then there is some problem in above layers
            assert(0);
            return false; // to make compiler happy
        }

    }

    RoutingUnit *m_routing_unit;

    //uint32_t functionalWrite(Packet *);

  private:
    bool send_routedSwap;
    Cycles m_latency;
    // Cycles print_trigger;
    int m_virtual_networks, m_num_vcs, m_vc_per_vnet;
    GarnetNetwork *m_network_ptr;

    std::vector<InputUnit *> m_input_unit;
    std::vector<OutputUnit *> m_output_unit;
    //RoutingUnit *m_routing_unit;
    SwitchAllocator *m_sw_alloc;
    CrossbarSwitch *m_switch;

    // Statistical variables required for power computations
    Stats::Scalar m_buffer_reads;
    Stats::Scalar m_buffer_writes;

    Stats::Scalar m_sw_input_arbiter_activity;
    Stats::Scalar m_sw_output_arbiter_activity;

    Stats::Scalar m_crossbar_activity;
};

#endif // __MEM_RUBY_NETWORK_GARNET2_0_ROUTER_HH__
