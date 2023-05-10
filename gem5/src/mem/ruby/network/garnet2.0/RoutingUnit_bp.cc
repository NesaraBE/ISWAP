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


#include "mem/ruby/network/garnet2.0/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/logging.hh"
#include "mem/ruby/network/garnet2.0/InputUnit.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

#include "mem/ruby/network/garnet2.0/OutputUnit.hh"
#include "mem/ruby/network/garnet2.0/GarnetNetwork.hh"

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();

	

}

void
RoutingUnit::addRoute(const NetDest& routing_table_entry)
{
    m_routing_table.push_back(routing_table_entry);
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */

int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table.size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(m_routing_table[link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table.size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(m_routing_table[link])) {

            if (m_weight_table[link] == min_weight) {

                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int
RoutingUnit::outportCompute(RouteInfo route, int inport,
                            PortDirection inport_dirn,
                            int vc)
{
    int outport = -1;

    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case TABLE_:  outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
        case XY_:     outport =
            outportComputeXY(route, inport, inport_dirn); break;
        case TURN_MODEL_OBLIVIOUS_: outport =
            outportComputeTurnModelOblivious(route, inport, inport_dirn); break;
        //case TURN_MODEL_ADAPTIVE_: outport =
        //    outportComputeTurnModelAdaptive(route, inport, inport_dirn); break;
        case TURN_MODEL_ADAPTIVE_: outport =
            outportComputeTurnModelAdaptive(route, inport, inport_dirn); break;
		case RANDOM_OBLIVIOUS_: outport =
            outportComputeRandomOblivious(route, inport, inport_dirn); break;
       // case RANDOM_OBLIVIOUS_: outport = (vc==0) ? outportComputeTurnModelOblivious(route, inport, inport_dirn): outportComputeRandomOblivious(route, inport, inport_dirn); break;
		case RANDOM_ADAPTIVE_: outport =
            outportComputeRandomAdaptive(route, inport, inport_dirn); break;
        // any custom algorithm
        case CUSTOM_: outport =
            outportComputeCustom(route, inport, inport_dirn); break;
        default: outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
    }

    assert(outport != -1);
    return outport;
}	
     
// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                                  int inport,
 	                             PortDirection inport_dirn)
{	
        PortDirection outport_dirn = "Unknown";
 	
 	   int M5_VAR_USED num_rows = m_router->get_net_ptr()->getNumRows();
        int num_cols = m_router->get_net_ptr()->getNumCols();
 	   assert(num_rows > 0 && num_cols > 0);
 	
 	   int my_id = m_router->get_id();
 	   int my_x = my_id % num_cols;
 	   int my_y = my_id / num_cols;
 	
 	   int dest_id = route.dest_router;
 	   int dest_x = dest_id % num_cols;
 	   int dest_y = dest_id / num_cols;
 	
 	   int x_hops = abs(dest_x - my_x);
 	   int y_hops = abs(dest_y - my_y);
 	
 	   bool x_dirn = (dest_x >= my_x);
 	   bool y_dirn = (dest_y >= my_y);
 	
 	   // already checked that in outportCompute() function
 	   assert(!(x_hops == 0 && y_hops == 0));
 	
 	   if (x_hops > 0) {
 	       if (x_dirn) {
 	           assert(inport_dirn == "Local" || inport_dirn == "West");
 	           outport_dirn = "East";
 	       } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
 	           assert(inport_dirn != "North");
                outport_dirn = "North";
            } else {
 	           // "Local" or "North" or "West" or "East"
                assert(inport_dirn != "South");
 	           outport_dirn = "South";
 	       }
        } else {
 	       // x_hops == 0 and y_hops == 0
 	       // this is not possible
            // already checked that in outportCompute() function
 	       panic("x_hops == y_hops == 0");
 	   }
     
 	   return m_outports_dirn2idx[outport_dirn];
}	
 	
 	
int
RoutingUnit::outportComputeTurnModelOblivious(RouteInfo route,
 	                                   int inport,
 	                                   PortDirection inport_dirn)
{	
 	
 	   PortDirection outport_dirn = "Unknown";
 	
 	   int M5_VAR_USED num_rows = m_router->get_net_ptr()->getNumRows();
 	   int num_cols = m_router->get_net_ptr()->getNumCols();
 	   assert(num_rows > 0 && num_cols > 0);
 	
 	   int my_id = m_router->get_id();
 	   int my_x = my_id % num_cols;
 	   int my_y = my_id / num_cols;
 	
 	   int dest_id = route.dest_router;
 	   int dest_x = dest_id % num_cols;
 	   int dest_y = dest_id / num_cols;
 	
    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    /////////////////////////////////////////
    // ICN Lab 3: Insert code here
	if (x_hops == 0)
    {
        if (y_dirn > 0)
            outport_dirn = "North";
        else
            outport_dirn = "South";
    }
    else if (y_hops == 0)
    {
        if (x_dirn > 0)
            outport_dirn = "East";
        else
            outport_dirn = "West";
    } else {
        int rand = random() % 2;

        if (x_dirn && y_dirn) // Quadrant I
            outport_dirn = rand ? "East" : "East";
        else if (!x_dirn && y_dirn) // Quadrant II
            outport_dirn = rand ? "West" : "West";
        else if (!x_dirn && !y_dirn) // Quadrant III
            outport_dirn = rand ? "West" : "South";
        else // Quadrant IV
            outport_dirn = rand ? "East" : "South";
    }


    return m_outports_dirn2idx[outport_dirn];
}

int
RoutingUnit::outportComputeTurnModelAdaptive(RouteInfo route,
                                    int inport,
                                    PortDirection inport_dirn)
{
/*
   PortDirection outport_dirn = "Unknown";

    int M5_VAR_USED num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops == 0)
    {
        if (y_dirn > 0)
            outport_dirn = "North";
        else
            outport_dirn = "South";
    }
    else if (y_hops == 0)
    {
        if (x_dirn > 0)
            outport_dirn = "East";
        else
            outport_dirn = "West";
    } else {
        int rand = random() % 2;

		bool freeVC_E =0;
		bool freeVC_S =0;
		bool freeVC_W =0;
		
		m_input_unit = m_router->get_inputUnit_ref();
    	m_output_unit = m_router->get_outputUnit_ref();

		freeVC_E = 	m_output_unit[m_outports_dirn2idx["East"]]->has_free_vc(route.vnet);
		freeVC_S = 	m_output_unit[m_outports_dirn2idx["South"]]->has_free_vc(route.vnet);
		freeVC_W = 	m_output_unit[m_outports_dirn2idx["West"]]->has_free_vc(route.vnet);
		
		if (x_dirn && y_dirn) { // Quadrant I
				outport_dirn = "East";
		} else if (!x_dirn && y_dirn) { // Quadrant II
				outport_dirn = "West";
		} else if (!x_dirn && !y_dirn) { // Quadrant III
        	if (freeVC_W & freeVC_S) {
				outport_dirn = rand ? "West" : "South";
			} else if (freeVC_S) {
				outport_dirn = "South";
			} else if (freeVC_W) {
				outport_dirn = "West";
			} else {
				outport_dirn = rand ? "West" : "South";
			}
		
		} else {// Quadrant IV
        	if (freeVC_E & freeVC_S) {
				outport_dirn = rand ? "East" : "South";
			} else if (freeVC_S) {
				outport_dirn = "South";
			} else if (freeVC_E) {
				outport_dirn = "East";
			} else {
				outport_dirn = rand ? "East" : "South";
			}
		}

    }
 */
	return 0;//m_outports_dirn2idx[outport_dirn];
	
}


int
RoutingUnit::outportComputeRandomOblivious(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    int M5_VAR_USED num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops == 0)
    {
        if (y_dirn > 0)
            outport_dirn = "North";
        else
            outport_dirn = "South";
    }
    else if (y_hops == 0)
    {
        if (x_dirn > 0)
            outport_dirn = "East";
        else
            outport_dirn = "West";
    } else {
        int rand = random() % 2;

        if (x_dirn && y_dirn) // Quadrant I
            outport_dirn = rand ? "East" : "North";
        else if (!x_dirn && y_dirn) // Quadrant II
            outport_dirn = rand ? "West" : "North";
        else if (!x_dirn && !y_dirn) // Quadrant III
            outport_dirn = rand ? "West" : "South";
        else // Quadrant IV
            outport_dirn = rand ? "East" : "South";
    }

    return m_outports_dirn2idx[outport_dirn];
}

int
RoutingUnit::outportComputeRandomAdaptive(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    panic("%s placeholder executed", __FUNCTION__);
}

// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
int
RoutingUnit::outportComputeCustom(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{

//1. if src dest within the sub-meshes, implement XY within them (use modulo).

//2. if src dest across sub-meshes. Route to the nearest boundary router using sum of xy hops. Enable all turns to interposer from all boundary routers. This should create a valid path to destination. But may involve deadlocks. 

//write a python script to identify illegal turns, using this remove the turns, manually here for 2nd case

//This should implement all of modular routing paper except for boundary router position.

//see if you can control uniform random traffic here: src/cpu/testers/garnet_synthetic_traffic/GarnetSyntheticTraffic.cc

    //panic("%s placeholder executed", __FUNCTION__);
	PortDirection outport_dirn = "Unknown";
    //int M5_VAR_USED num_rows = m_router->get_net_ptr()->getNumRows();
    //int num_cols = m_router->get_net_ptr()->getNumCols();
   	int M5_VAR_USED num_rows = 4;
    int num_cols = 4;
	assert(num_rows > 0 && num_cols > 0);
	
	int my_id = m_router->get_id();
    int dest_id = route.dest_router;
	int src_id = route.src_router;
    	
	if ((dest_id < 16) && (src_id <16) && (my_id <16) ) { /* chiplet 1 x-y routing */
		
		int my_x = my_id % num_cols;
   		int my_y = my_id / num_cols;

	    int dest_x = dest_id % num_cols;
    	int dest_y = dest_id / num_cols;

    	int x_hops = abs(dest_x - my_x);
    	int y_hops = abs(dest_y - my_y);

    	bool x_dirn = (dest_x >= my_x);
    	bool y_dirn = (dest_y >= my_y);

    	// already checked that in outportCompute() function
    	assert(!(x_hops == 0 && y_hops == 0));

    	if (x_hops > 0) {
    	    if (x_dirn) {
    	        assert(inport_dirn == "Local" || inport_dirn == "West");
    	        outport_dirn = "East";
    	    } else {
    	        assert(inport_dirn == "Local" || inport_dirn == "East");
    	        outport_dirn = "West";
    	    }
    	} else if (y_hops > 0) {
    	    if (y_dirn) {
    	        // "Local" or "South" or "West" or "East"
    	        assert(inport_dirn != "North");
    	        outport_dirn = "North";
    	    } else {
    	        // "Local" or "North" or "West" or "East"
    	        assert(inport_dirn != "South");
    	        outport_dirn = "South";
    	    }
    	} else {
    	    // x_hops == 0 and y_hops == 0
    	    // this is not possible
    	    // already checked that in outportCompute() function
    	    panic("x_hops == y_hops == 0");
    	}

	} else if ((dest_id < 32) && (src_id <32) && (my_id <32) && (dest_id > 15) && (src_id >15) && (my_id >15)) { /* chiplet 2 x-y routing */
		//my_id = my_id - 16;
    	//dest_id = dest_id - 16;
		//src_id = src_id - 16;

		int my_x = my_id % num_cols;
   		int my_y = my_id / num_cols;

	    int dest_x = dest_id % num_cols;
    	int dest_y = dest_id / num_cols;

    	int x_hops = abs(dest_x - my_x);
    	int y_hops = abs(dest_y - my_y);

    	bool x_dirn = (dest_x >= my_x);
    	bool y_dirn = (dest_y >= my_y);

    	// already checked that in outportCompute() function
    	assert(!(x_hops == 0 && y_hops == 0));

    	if (x_hops > 0) {
    	    if (x_dirn) {
    	        assert(inport_dirn == "Local" || inport_dirn == "West");
    	        outport_dirn = "East";
    	    } else {
    	        assert(inport_dirn == "Local" || inport_dirn == "East");
    	        outport_dirn = "West";
    	    }
    	} else if (y_hops > 0) {
    	    if (y_dirn) {
    	        // "Local" or "South" or "West" or "East"
    	        assert(inport_dirn != "North");
    	        outport_dirn = "North";
    	    } else {
    	        // "Local" or "North" or "West" or "East"
    	        assert(inport_dirn != "South");
    	        outport_dirn = "South";
    	    }
    	} else {
    	    // x_hops == 0 and y_hops == 0
    	    // this is not possible
    	    // already checked that in outportCompute() function
    	    panic("x_hops == y_hops == 0");
    	}

	} else if ((dest_id < 48) && (src_id <48) && (my_id <48) && (dest_id > 31) && (src_id >31) && (my_id >31)) { /* chiplet 3 x-y routing */
		//my_id = my_id - 32;
    	//dest_id = dest_id - 32;
		//src_id = src_id - 32;

		int my_x = my_id % num_cols;
   		int my_y = my_id / num_cols;

	    int dest_x = dest_id % num_cols;
    	int dest_y = dest_id / num_cols;

    	int x_hops = abs(dest_x - my_x);
    	int y_hops = abs(dest_y - my_y);

    	bool x_dirn = (dest_x >= my_x);
    	bool y_dirn = (dest_y >= my_y);

    	// already checked that in outportCompute() function
    	assert(!(x_hops == 0 && y_hops == 0));

    	if (x_hops > 0) {
    	    if (x_dirn) {
    	        assert(inport_dirn == "Local" || inport_dirn == "West");
    	        outport_dirn = "East";
    	    } else {
    	        assert(inport_dirn == "Local" || inport_dirn == "East");
    	        outport_dirn = "West";
    	    }
    	} else if (y_hops > 0) {
    	    if (y_dirn) {
    	        // "Local" or "South" or "West" or "East"
    	        assert(inport_dirn != "North");
    	        outport_dirn = "North";
    	    } else {
    	        // "Local" or "North" or "West" or "East"
    	        assert(inport_dirn != "South");
    	        outport_dirn = "South";
    	    }
    	} else {
    	    // x_hops == 0 and y_hops == 0
    	    // this is not possible
    	    // already checked that in outportCompute() function
    	    panic("x_hops == y_hops == 0");
    	}

	} else if ((dest_id < 64) && (src_id <64) && (my_id <64) && (dest_id > 47) && (src_id >47) && (my_id >47)) { /* Interposer x-y routing */
		//my_id = my_id - 48;
    	//dest_id = dest_id - 48;
		//src_id = src_id - 48;

		inport_dirn = "Local";

		

		int my_x = my_id % num_cols;
   		int my_y = my_id / num_cols;

	    int dest_x = dest_id % num_cols;
    	int dest_y = dest_id / num_cols;

    	int x_hops = abs(dest_x - my_x);
    	int y_hops = abs(dest_y - my_y);

    	bool x_dirn = (dest_x >= my_x);
    	bool y_dirn = (dest_y >= my_y);

    	// already checked that in outportCompute() function
    	assert(!(x_hops == 0 && y_hops == 0));

    	if (x_hops > 0) {
    	    if (x_dirn) {
    	        assert(inport_dirn == "Local" || inport_dirn == "West");
    	        outport_dirn = "East";
    	    } else {
    	        assert(inport_dirn == "Local" || inport_dirn == "East");
    	        outport_dirn = "West";
    	    }
    	} else if (y_hops > 0) {
    	    if (y_dirn) {
    	        // "Local" or "South" or "West" or "East"
    	        assert(inport_dirn != "North");
    	        outport_dirn = "North";
    	    } else {
    	        // "Local" or "North" or "West" or "East"
    	        assert(inport_dirn != "South");
    	        outport_dirn = "South";
    	    }
    	} else {
    	    // x_hops == 0 and y_hops == 0
    	    // this is not possible
    	    // already checked that in outportCompute() function
    	    panic("x_hops == y_hops == 0");
    	}
		/*
		int my_x = my_id % num_cols;
   				int my_y = my_id / num_cols;

			    int dest_x = dest_id % num_cols;
    			int dest_y = dest_id / num_cols;
				
				int x_hops =1;
				int y_hops =1;
    			if (!(my_id == dest_id)) {
					x_hops = abs(dest_x - my_x);
    				y_hops = abs(dest_y - my_y);
				} else {
					x_hops = 1;
					y_hops = 1;
				}

    			bool x_dirn = (dest_x >= my_x);
    			bool y_dirn = (dest_y >= my_y);

				// already checked that in outportCompute() function
    			assert(!(x_hops == 0 && y_hops == 0));

				if (x_hops == 0)
   				{
   			    	 if (y_dirn > 0)
   			        	 outport_dirn = "North";
   			     	else
   			        	 outport_dirn = "South";
   				}
   			 	else if (y_hops == 0)
   			 	{
   			 	    if (x_dirn > 0)
   			       	  outport_dirn = "East";
   			    	else
   			         outport_dirn = "West";
   			 	} else {
   			     	int rand = random() % 2;

   			     	if (x_dirn && y_dirn) // Quadrant I
   			        	 outport_dirn = rand ? "East" : "North";
   			     	else if (!x_dirn && y_dirn) // Quadrant II
   			        	 outport_dirn = rand ? "West" : "North";
   			     	else if (!x_dirn && !y_dirn) // Quadrant III
   			        	 outport_dirn = rand ? "West" : "South";
   			     	else // Quadrant IV
   			        	 outport_dirn = rand ? "East" : "South";
   			 	}*/


	} else { /* inter sub-mesh routing */
		  if ((src_id <16) && (my_id <16)) { /*chiplet1 to interposer*/
			  //inport_dirn = "Local";
			  int src_id_G  = src_id ;
			  int my_id_G   = my_id  ;
			  int dest_id_G = dest_id;
			  
			  std::cout<<""<<std::endl;
              std::cout<<src_id<<","<<dest_id<<std::endl;
			  std::cout<<"inter-mesh movement - chiplet1 to interposer"<<std::endl;
			  std::cout<<"my_id_G: "<<my_id_G<<std::endl;
			  std::cout<<"dest_id_G: "<<dest_id_G<<std::endl;
		   	  std::cout<<"src_id_G: "<<src_id_G<<std::endl;
			  

			  switch (dest_id%4) {
			  		case 0: dest_id = 0;
							break;
					case 1: dest_id = 1;
							break;
					case 2: dest_id = 2;
							break;
					case 3: dest_id = 3;
							break;
				}
				
		  		int my_x = my_id % num_cols;
   				int my_y = my_id / num_cols;

			    int dest_x = dest_id % num_cols;
    			int dest_y = dest_id / num_cols;

				int x_hops =1;
				int y_hops =1;
    			if (!(my_id ==dest_id)) {
					x_hops = abs(dest_x - my_x);
    				y_hops = abs(dest_y - my_y);
				} else {
					x_hops = 1;
					y_hops = 1;
				}

    			bool x_dirn = (dest_x >= my_x);
    			bool y_dirn = (dest_y >= my_y);

				std::cout<<"inport_dirn: "<<inport_dirn<<std::endl;
				std::cout<<"my_id: "<<my_id<<std::endl;
				std::cout<<"dest_id: "<<dest_id<<std::endl;
				std::cout<<"src_id: "<<src_id<<std::endl;


    			// already checked that in outportCompute() function
    			assert(!(x_hops == 0 && y_hops == 0));
				
				if (my_id ==dest_id) {
						outport_dirn = "South";
		  		} else if (x_hops > 0) {
    			    if (x_dirn) {
    			        assert(inport_dirn == "Local" || inport_dirn == "West");
    			        outport_dirn = "East";
    			    } else {
    			        assert(inport_dirn == "Local" || inport_dirn == "East");
    			        outport_dirn = "West";
    			    }
    			} else if (y_hops > 0) {
    			    if (y_dirn) {
    			        // "Local" or "South" or "West" or "East"
    			        assert(inport_dirn != "North");
    			        outport_dirn = "North";
    			    } else {
    			        // "Local" or "North" or "West" or "East"
    			        assert(inport_dirn != "South");
    			        outport_dirn = "South";
    			    }  
    			} else {
    			    // x_hops == 0 and y_hops == 0
    			    // this is not possible
    			    // already checked that in outportCompute() function
    			    panic("x_hops == y_hops == 0");
    			}

				std::cout<<"outport_dirn: "<<outport_dirn<<std::endl;

		
		  } else if ((src_id <32) && (my_id <32) && (src_id >15) && (my_id >15) ) { /* chiplet2 to interposer*/
			    //inport_dirn = "Local";
			  	int src_id_G  = src_id ;
			  	int my_id_G   = my_id  ;
			  	int dest_id_G = dest_id;
			    std::cout<<""<<std::endl;
				std::cout<<src_id<<","<<dest_id<<std::endl;
			  	std::cout<<"inter-mesh movement - chiplet2 to interposer"<<std::endl;
			  	std::cout<<"my_id_G: "<<my_id_G<<std::endl;
			 	std::cout<<"dest_id_G: "<<dest_id_G<<std::endl;
		   	 	std::cout<<"src_id_G: "<<src_id_G<<std::endl;
			  
			  	switch (dest_id%4) {
			  		case 0: dest_id = 16;
							break;
					case 1: dest_id = 17;
							break;
					case 2: dest_id = 18;
							break;
					case 3: dest_id = 19;
							break;
				}
				
				//my_id = my_id - 16;
    			//dest_id = dest_id - 16;
				//src_id = src_id - 16;

		  		int my_x = my_id % num_cols;
   				int my_y = my_id / num_cols;

			    int dest_x = dest_id % num_cols;
    			int dest_y = dest_id / num_cols;
				
				int x_hops =1;
				int y_hops =1;
    			if (!(my_id == dest_id)) {
					x_hops = abs(dest_x - my_x);
    				y_hops = abs(dest_y - my_y);
				} else {
					x_hops = 1;
					y_hops = 1;
				}
				
    			bool x_dirn = (dest_x >= my_x);
    			bool y_dirn = (dest_y >= my_y);

				std::cout<<"inport_dirn: "<<inport_dirn<<std::endl;
				std::cout<<"my_id: "<<my_id<<std::endl;
				std::cout<<"dest_id: "<<dest_id<<std::endl;
				std::cout<<"src_id: "<<src_id<<std::endl;


    			// already checked that in outportCompute() function
    			assert(!(x_hops == 0 && y_hops == 0));
				
				if (my_id == dest_id) {
						outport_dirn = "South";
		  		} else if (x_hops > 0) {
    			    if (x_dirn) {
    			        assert(inport_dirn == "Local" || inport_dirn == "West");
    			        outport_dirn = "East";
    			    } else {
    			        assert(inport_dirn == "Local" || inport_dirn == "East");
    			        outport_dirn = "West";
    			    }
    			} else if (y_hops > 0) {
    			    if (y_dirn) {
    			        // "Local" or "South" or "West" or "East"
    			        assert(inport_dirn != "North");
    			        outport_dirn = "North";
    			    } else {
    			        // "Local" or "North" or "West" or "East"
    			        assert(inport_dirn != "South");
    			        outport_dirn = "South";
    			    }  
    			} else {
    			    // x_hops == 0 and y_hops == 0
    			    // this is not possible
    			    // already checked that in outportCompute() function
    			    panic("x_hops == y_hops == 0");
    			}

				std::cout<<"outport_dirn: "<<outport_dirn<<std::endl;
		
		  } else if ((src_id < 48) && (my_id <48) && (src_id >31) && (my_id >31) ) { /* chiplet3 to interposer*/
			    //inport_dirn = "Local";
			  	int src_id_G  = src_id ;
			  	int my_id_G   = my_id  ;
			  	int dest_id_G = dest_id;
			    std::cout<<""<<std::endl;
				std::cout<<src_id<<","<<dest_id<<std::endl;
			  	std::cout<<"inter-mesh movement - chiplet3 to interposer"<<std::endl;
			  	std::cout<<"my_id_G: "<<my_id_G<<std::endl;
			  	std::cout<<"dest_id_G: "<<dest_id_G<<std::endl;
		   	  	std::cout<<"src_id_G: "<<src_id_G<<std::endl;
			  	
				switch (dest_id%4) {
			  		case 0: dest_id = 44;
							break;
					case 1: dest_id = 45;
							break;
					case 2: dest_id = 46;
							break;
					case 3: dest_id = 47;
							break;
				}
				
				//my_id = my_id - 32;
    			//dest_id = dest_id - 32;
				//src_id = src_id - 32;

		  		int my_x = my_id % num_cols;
   				int my_y = my_id / num_cols;

			    int dest_x = dest_id % num_cols;
    			int dest_y = dest_id / num_cols;
				
				int x_hops =1;
				int y_hops =1;
    			if (!(my_id == dest_id)) {
					x_hops = abs(dest_x - my_x);
    				y_hops = abs(dest_y - my_y);
				} else {
					x_hops = 1;
					y_hops = 1;
				}
				
    			bool x_dirn = (dest_x >= my_x);
    			bool y_dirn = (dest_y >= my_y);

				std::cout<<"inport_dirn: "<<inport_dirn<<std::endl;
				std::cout<<"my_id: "<<my_id<<std::endl;
				std::cout<<"dest_id: "<<dest_id<<std::endl;
				std::cout<<"src_id: "<<src_id<<std::endl;


    			// already checked that in outportCompute() function
    			assert(!(x_hops == 0 && y_hops == 0));
				
				if (my_id ==dest_id) {
						outport_dirn = "North";
		  		} else if (x_hops > 0) {
    			    if (x_dirn) {
    			        assert(inport_dirn == "Local" || inport_dirn == "West");
    			        outport_dirn = "East";
    			    } else {
    			        assert(inport_dirn == "Local" || inport_dirn == "East");
    			        outport_dirn = "West";
    			    }
    			} else if (y_hops > 0) {
    			    if (y_dirn) {
    			        // "Local" or "South" or "West" or "East"
    			        assert(inport_dirn != "North");
    			        outport_dirn = "North";
    			    } else {
    			        // "Local" or "North" or "West" or "East"
    			        assert(inport_dirn != "South");
    			        outport_dirn = "South";
    			    }  
    			} else {
    			    // x_hops == 0 and y_hops == 0
    			    // this is not possible
    			    // already checked that in outportCompute() function
    			    panic("x_hops == y_hops == 0");
    			}
				std::cout<<"outport_dirn: "<<outport_dirn<<std::endl;
		
		  } else if (my_id > 47) { /* interposer routing */
			    int src_id_G  = src_id ;
			  	int my_id_G   = my_id  ;
			  	int dest_id_G = dest_id;
				//inport_dirn = "Local";
			    std::cout<<""<<std::endl;
				std::cout<<src_id<<","<<dest_id<<std::endl;
			  	std::cout<<"inter-mesh movement - interposer routing"<<std::endl;
			  	std::cout<<"my_id_G: "<<my_id_G<<std::endl;
			 	std::cout<<"dest_id_G: "<<dest_id_G<<std::endl;
		   	  	std::cout<<"src_id_G: "<<src_id_G<<std::endl;	
				
				/*
				if (dest_id < 16) {
					switch (dest_id%4) {
			  		case 3: dest_id = 60;
							break;
					case 2: dest_id = 56;
							break;
					case 1: dest_id = 52;
							break;
					case 0: dest_id = 48;
							break;
					}
				} else if ((dest_id > 15) && (dest_id < 32)) {
					switch (dest_id%4) {
			  		case 0: dest_id = 63;
							break;
					case 1: dest_id = 59;
							break;
					case 2: dest_id = 55;
							break;
					case 3: dest_id = 51;
							break;
					}
				} else if ((dest_id > 31) && (dest_id < 48)) {
					switch (dest_id%4) {
			  		case 0: dest_id = 48;
							break;
					case 1: dest_id = 49;
							break;
					case 2: dest_id = 50;
							break;
					case 3: dest_id = 51;
							break;
					}
				} else if (dest_id > 47) {
					dest_id = dest_id;
				}
				*/

				if (dest_id < 16) {
					switch (dest_id%4) {
			  		case 3: dest_id = 52;
							break;
					case 2: dest_id = 48;
							break;
					case 1: dest_id = 60;
							break;
					case 0: dest_id = 56;
							break;
					}
				} else if ((dest_id > 15) && (dest_id < 32)) {
					switch (dest_id%4) {
			  		case 0: dest_id = 55;
							break;
					case 1: dest_id = 51;
							break;
					case 2: dest_id = 63;
							break;
					case 3: dest_id = 59;
							break;
					}
				} else if ((dest_id > 31) && (dest_id < 48)) {
					switch (dest_id%4) {
			  		case 0: dest_id = 50;
							break;
					case 1: dest_id = 51;
							break;
					case 2: dest_id = 48;
							break;
					case 3: dest_id = 49;
							break;
					}
				} else if (dest_id > 47) {
					dest_id = dest_id;
				}


			   
				int my_x = my_id % num_cols;
   				int my_y = my_id / num_cols;

			    int dest_x = dest_id % num_cols;
    			int dest_y = dest_id / num_cols;
				
				int x_hops =1;
				int y_hops =1;
    			if (!(my_id == dest_id)) {
					x_hops = abs(dest_x - my_x);
    				y_hops = abs(dest_y - my_y);
				} else {
					x_hops = 1;
					y_hops = 1;
				}

    			bool x_dirn = (dest_x >= my_x);
    			bool y_dirn = (dest_y >= my_y);
				
				std::cout<<"inport_dirn: "<<inport_dirn<<std::endl;
				std::cout<<"my_id: "<<my_id<<std::endl;
				std::cout<<"dest_id: "<<dest_id<<std::endl;
				std::cout<<"src_id: "<<src_id<<std::endl;

    			// already checked that in outportCompute() function
    			assert(!(x_hops == 0 && y_hops == 0));

				if (my_id == dest_id) {
							if (dest_id_G < 16) {
								std::cout<<"packet wants to enter chiplet1"<<std::endl;
								outport_dirn = "West";
				  			} else if (dest_id_G > 15 && dest_id_G < 32) {
								std::cout<<"packet wants to enter chiplet2"<<std::endl;
								outport_dirn = "East";
							} else if (dest_id_G > 31 && dest_id_G < 48) {
								std::cout<<"packet wants to enter chiplet3"<<std::endl;
								outport_dirn = "South";
							}	
				}
				else if (x_hops == 0)
   				{
   			    	 if (y_dirn > 0)
   			        	 outport_dirn = "North";
   			     	else
   			        	 outport_dirn = "South";
   				}
   			 	else if (y_hops == 0)
   			 	{
   			 	    if (x_dirn > 0)
   			       	  outport_dirn = "East";
   			    	else
   			         outport_dirn = "West";
   			 	} else {
   			     	int rand = random() % 2;

   			     	if (x_dirn && y_dirn) // Quadrant I
   			        	 outport_dirn = rand ? "East" : "North";
   			     	else if (!x_dirn && y_dirn) // Quadrant II
   			        	 outport_dirn = rand ? "West" : "North";
   			     	else if (!x_dirn && !y_dirn) // Quadrant III
   			        	 outport_dirn = rand ? "West" : "South";
   			     	else // Quadrant IV
   			        	 outport_dirn = rand ? "East" : "South";
   			 	}
				
				//my_id = my_id - 48;
    			//dest_id = dest_id - 48;
				//src_id = src_id - 48;
				/*
		  		int my_x = my_id % num_cols;
   				int my_y = my_id / num_cols;

			    int dest_x = dest_id % num_cols;
    			int dest_y = dest_id / num_cols;
				
				int x_hops =1;
				int y_hops =1;
    			if (!(my_id == dest_id)) {
					x_hops = abs(dest_x - my_x);
    				y_hops = abs(dest_y - my_y);
				} else {
					x_hops = 1;
					y_hops = 1;
				}

    			bool x_dirn = (dest_x >= my_x);
    			bool y_dirn = (dest_y >= my_y);

				//std::cout<<"inport_dirn: "<<inport_dirn<<std::endl;
				//std::cout<<"my_id: "<<my_id<<std::endl;
				//std::cout<<"dest_id: "<<dest_id<<std::endl;
				//std::cout<<"src_id: "<<src_id<<std::endl;


    			// already checked that in outportCompute() function
    			assert(!(x_hops == 0 && y_hops == 0));
				if (my_id == dest_id) {
					if (dest_id_G < 16) {
						//std::cout<<"packet wants to enter chiplet1"<<std::endl;
						outport_dirn = "West";
		  			} else if (dest_id_G > 15 && dest_id_G < 32) {
						//std::cout<<"packet wants to enter chiplet2"<<std::endl;
						outport_dirn = "East";
					} else if (dest_id_G > 31 && dest_id_G < 48) {
						//std::cout<<"packet wants to enter chiplet3"<<std::endl;
						outport_dirn = "South";
					}	
				} else if (x_hops > 0) {
    			    if (x_dirn) {
						assert(inport_dirn == "Local" || inport_dirn == "West");
    			        outport_dirn = "East";
    			    } else {
						assert(inport_dirn == "Local" || inport_dirn == "East");
    			        outport_dirn = "West";
    			    }
    			} else if (y_hops > 0) {
    			    if (y_dirn) {
    			        // "Local" or "South" or "West" or "East"
    			        assert(inport_dirn != "North");
    			        outport_dirn = "North";
    			    } else {
    			        // "Local" or "North" or "West" or "East"
    			        assert(inport_dirn != "South");
    			        outport_dirn = "South";
    			    }  
    			} else {
    			    // x_hops == 0 and y_hops == 0
    			    // this is not possible
    			    // already checked that in outportCompute() function
    			    panic("x_hops == y_hops == 0");
    			}	*/
				//std::cout<<"outport_dirn: "<<outport_dirn<<std::endl;	
		  } else { /*interposer to destination chiplets*/
		  		inport_dirn = "Local";
		  		int src_id_G  = src_id ;
			  	int my_id_G   = my_id  ;
			  	int dest_id_G = dest_id;
			    std::cout<<""<<std::endl;
				std::cout<<src_id<<","<<dest_id<<std::endl;
			  	std::cout<<"inter-mesh movement - dest chiplet"<<std::endl;
			  	std::cout<<"my_id_G: "<<my_id_G<<std::endl;
			 	std::cout<<"dest_id_G: "<<dest_id_G<<std::endl;
		   	  	std::cout<<"src_id_G: "<<src_id_G<<std::endl;	  
 
			  	/*if ((dest_id < 16) && (my_id < 16)) {
					my_id = my_id - 0;
    				dest_id = dest_id - 0;
					src_id = src_id - 0;
				} else if ((dest_id < 32) && (my_id < 32) && (dest_id >15) && (my_id >15)) {
					my_id = my_id - 16;
    				dest_id = dest_id - 16;
					src_id = src_id - 16;
				} else if ((dest_id < 48) && (my_id < 48) && (dest_id >31) && (my_id >31)) {
					my_id = my_id - 32;
    				dest_id = dest_id - 32;
					src_id = src_id - 32;
				} else if ((dest_id > 47) && (my_id < 47)) {
					my_id = my_id - 48;
    				dest_id = dest_id - 48;
					src_id = src_id - 48;
				}*/
				
				int my_x = my_id % num_cols;
   				int my_y = my_id / num_cols;

			    int dest_x = dest_id % num_cols;
    			int dest_y = dest_id / num_cols;

    			int x_hops = abs(dest_x - my_x);
    			int y_hops = abs(dest_y - my_y);

    			bool x_dirn = (dest_x >= my_x);
    			bool y_dirn = (dest_y >= my_y);
				
				std::cout<<"inport_dirn: "<<inport_dirn<<std::endl;
				std::cout<<"my_id: "<<my_id<<std::endl;
				std::cout<<"dest_id: "<<dest_id<<std::endl;
				std::cout<<"src_id: "<<src_id<<std::endl;

				if(dest_id_G == dest_id)
				{
					std::cout<< "Inter-Mesh Traffic reached destination id : " << dest_id_G << "from source id: " << src_id_G << endl; 
				}

    			// already checked that in outportCompute() function
    			assert(!(x_hops == 0 && y_hops == 0));

    			if (x_hops > 0) {
    			    if (x_dirn) {
						
    			        assert(inport_dirn == "Local" || inport_dirn == "West");
    			        outport_dirn = "East";
    			    } else {
						
    			        assert(inport_dirn == "Local" || inport_dirn == "East");
    			        outport_dirn = "West";
    			    }
    			} else if (y_hops > 0) {
    			    if (y_dirn) {
    			        // "Local" or "South" or "West" or "East"
    			        assert(inport_dirn != "North");
    			        outport_dirn = "North";
    			    } else {
    			        // "Local" or "North" or "West" or "East"
    			        assert(inport_dirn != "South");
    			        outport_dirn = "South";
    			    }
    			} else {
    			    // x_hops == 0 and y_hops == 0
    			    // this is not possible
    			    // already checked that in outportCompute() function
    			    panic("x_hops == y_hops == 0");
    			}
				std::cout<<"outport_dirn: "<<outport_dirn<<std::endl;


		  }

	}

    
    return m_outports_dirn2idx[outport_dirn];


}
