# Copyright (c) 2010 Advanced Micro Devices, Inc.
#               2016 Georgia Institute of Technology
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Brad Beckmann
#          Tushar Krishna

from m5.params import *
from m5.objects import *

from BaseTopology import SimpleTopology

# Creates a generic Mesh assuming an equal number of cache
# and directory controllers.
# XY routing is enforced (using link weights)
# to guarantee deadlock freedom.

class Het_meshs(SimpleTopology):
    description='Het_meshs'

    print "Creating Topology: " + description

    def __init__(self, controllers):
        self.nodes = controllers

    # Makes a generic mesh
    # assuming an equal number of cache and directory cntrls

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        nodes = self.nodes

        num_routers = options.num_cpus
        num_rows = options.mesh_rows

        # default values for link latency and router latency.
        # Can be over-ridden on a per link/router basis
        link_latency = options.link_latency # used by simple and garnet
        router_latency = options.router_latency # only used by garnet


        # There must be an evenly divisible number of cntrls to routers
        # Also, obviously the number or rows must be <= the number of routers
        cntrls_per_router, remainder = divmod(len(nodes), num_routers)
        assert(num_rows > 0 and num_rows <= num_routers)
        num_columns = int(num_routers / num_rows)
        assert(num_columns * num_rows == num_routers)

        # Create the routers in the mesh
        routers = [Router(router_id=i, latency = router_latency) \
            for i in range(num_routers)]
        network.routers = routers

        # link counter to set unique link ids
        link_count = 0

        # Add all but the remainder nodes to the list of nodes to be uniformly
        # distributed across the network.
        network_nodes = []
        remainder_nodes = []
        for node_index in xrange(len(nodes)):
            if node_index < (len(nodes) - remainder):
                network_nodes.append(nodes[node_index])
            else:
                remainder_nodes.append(nodes[node_index])

        # Connect each node to the appropriate router
        ext_links = []
        for (i, n) in enumerate(network_nodes):
            cntrl_level, router_id = divmod(i, num_routers)
            assert(cntrl_level < cntrls_per_router)
            ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                    int_node=routers[router_id],
                                    latency = link_latency))
            link_count += 1

        # Connect the remainding nodes to router 0.  These should only be
        # DMA nodes.
        for (i, node) in enumerate(remainder_nodes):
            assert(node.type == 'DMA_Controller')
            assert(i < remainder)
            ext_links.append(ExtLink(link_id=link_count, ext_node=node,
                                    int_node=routers[0],
                                    latency = link_latency))
            link_count += 1

        network.ext_links = ext_links

        # Create the mesh links.
        int_links = []
        
        ###### for chiplet1 (16 nodes): 0 - 15 nodes
        print("Creating Chiplet 1 (16 nodes): nodes 0 to 15")
        # East output to West input links (weight = 1)
        num_rows = 4
        num_columns = 4
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_out = col + (row * num_columns)
                    west_in = (col + 1) + (row * num_columns)
                    print "Router " + get_id(routers[east_out]) + " created a link to Router " +  get_id(routers[west_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[east_out],
                                             dst_node=routers[west_in],
                                             src_outport="East",
                                             dst_inport="West",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # West output to East input links (weight = 1)
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_in = col + (row * num_columns)
                    west_out = (col + 1) + (row * num_columns)
                    print "Router " + get_id(routers[west_out]) + " created a link to Router " +  get_id(routers[east_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[west_out],
                                             dst_node=routers[east_in],
                                             src_outport="West",
                                             dst_inport="East",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # North output to South input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_out = col + (row * num_columns)
                    south_in = col + ((row + 1) * num_columns)
                    print "Router " + get_id(routers[north_out]) + " created a link to Router " +  get_id(routers[south_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[north_out],
                                             dst_node=routers[south_in],
                                             src_outport="North",
                                             dst_inport="South",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # South output to North input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_in = col + (row * num_columns)
                    south_out = col + ((row + 1) * num_columns)
                    print "Router " + get_id(routers[south_out]) + " created a link to Router " +  get_id(routers[north_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[south_out],
                                             dst_node=routers[north_in],
                                             src_outport="South",
                                             dst_inport="North",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1
        ##### end of chiplet 1


        ###### for chiplet2 (16 nodes): 16 - 31 nodes
        print("Creating Chiplet 2 (16 nodes): nodes 16 to 31")
        # East output to West input links (weight = 1)
        num_rows = 4
        num_columns = 4
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_out = col + (row * num_columns) + 16
                    west_in = (col + 1) + (row * num_columns) + 16
                    print "Router " + get_id(routers[east_out]) + " created a link to Router " +  get_id(routers[west_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[east_out],
                                             dst_node=routers[west_in],
                                             src_outport="East",
                                             dst_inport="West",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # West output to East input links (weight = 1)
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_in = col + (row * num_columns) + 16
                    west_out = (col + 1) + (row * num_columns) + 16
                    print "Router " + get_id(routers[west_out]) + " created a link to Router " +  get_id(routers[east_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[west_out],
                                             dst_node=routers[east_in],
                                             src_outport="West",
                                             dst_inport="East",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # North output to South input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_out = col + (row * num_columns) + 16
                    south_in = col + ((row + 1) * num_columns) + 16
                    print "Router " + get_id(routers[north_out]) + " created a link to Router " +  get_id(routers[south_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[north_out],
                                             dst_node=routers[south_in],
                                             src_outport="North",
                                             dst_inport="South",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # South output to North input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_in = col + (row * num_columns) + 16
                    south_out = col + ((row + 1) * num_columns) + 16
                    print "Router " + get_id(routers[south_out]) + " created a link to Router " +  get_id(routers[north_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[south_out],
                                             dst_node=routers[north_in],
                                             src_outport="South",
                                             dst_inport="North",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1
        ##### end of chiplet 2


        ###### for chiplet3 (16 nodes): 32 - 47 nodes
        print("Creating Chiplet 3 (4 nodes): nodes 32 to 47")
        # East output to West input links (weight = 1)
        num_rows = 4
        num_columns = 4
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_out = col + (row * num_columns) + 32
                    west_in = (col + 1) + (row * num_columns) + 32
                    print "Router " + get_id(routers[east_out]) + " created a link to Router " +  get_id(routers[west_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[east_out],
                                             dst_node=routers[west_in],
                                             src_outport="East",
                                             dst_inport="West",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # West output to East input links (weight = 1)
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_in = col + (row * num_columns) + 32
                    west_out = (col + 1) + (row * num_columns) + 32
                    print "Router " + get_id(routers[west_out]) + " created a link to Router " +  get_id(routers[east_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[west_out],
                                             dst_node=routers[east_in],
                                             src_outport="West",
                                             dst_inport="East",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # North output to South input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_out = col + (row * num_columns) + 32
                    south_in = col + ((row + 1) * num_columns) + 32
                    print "Router " + get_id(routers[north_out]) + " created a link to Router " +  get_id(routers[south_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[north_out],
                                             dst_node=routers[south_in],
                                             src_outport="North",
                                             dst_inport="South",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # South output to North input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_in = col + (row * num_columns) + 32
                    south_out = col + ((row + 1) * num_columns) + 32
                    print "Router " + get_id(routers[south_out]) + " created a link to Router " +  get_id(routers[north_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[south_out],
                                             dst_node=routers[north_in],
                                             src_outport="South",
                                             dst_inport="North",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1
        ##### end of chiplet 3
        
        ###### for Interposer (16 nodes): 48 - 63 nodes
        print("Creating Chiplet 3 (4 nodes): nodes 48 to 63")
        # East output to West input links (weight = 1)
        num_rows = 4
        num_columns = 4
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_out = col + (row * num_columns) + 48
                    west_in = (col + 1) + (row * num_columns) + 48
                    print "Router " + get_id(routers[east_out]) + " created a link to Router " +  get_id(routers[west_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[east_out],
                                             dst_node=routers[west_in],
                                             src_outport="East",
                                             dst_inport="West",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # West output to East input links (weight = 1)
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_in = col + (row * num_columns) + 48
                    west_out = (col + 1) + (row * num_columns) + 48
                    print "Router " + get_id(routers[west_out]) + " created a link to Router " +  get_id(routers[east_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[west_out],
                                             dst_node=routers[east_in],
                                             src_outport="West",
                                             dst_inport="East",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # North output to South input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_out = col + (row * num_columns) + 48
                    south_in = col + ((row + 1) * num_columns) + 48
                    print "Router " + get_id(routers[north_out]) + " created a link to Router " +  get_id(routers[south_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[north_out],
                                             dst_node=routers[south_in],
                                             src_outport="North",
                                             dst_inport="South",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1

        # South output to North input links (weight = 2)
        for col in xrange(num_columns):
            for row in xrange(num_rows):
                if (row + 1 < num_rows):
                    north_in = col + (row * num_columns) + 48
                    south_out = col + ((row + 1) * num_columns) + 48
                    print "Router " + get_id(routers[south_out]) + " created a link to Router " +  get_id(routers[north_in])
                    int_links.append(IntLink(link_id=link_count,
                                             src_node=routers[south_out],
                                             dst_node=routers[north_in],
                                             src_outport="South",
                                             dst_inport="North",
                                             latency = link_latency,
                                             weight=1))
                    link_count += 1
        ##### end of Interposer


        ##### Chiplet to interposer connections

        print("Creating Chiplet and interposer connections")
        
        print("1. Creating Chiplet1 and interposer connections")
        print "Router " + get_id(routers[0]) + " created a link to Router " +  get_id(routers[48])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[0],
                                 dst_node=routers[48],
                                 src_outport="South",
                                 dst_inport="West",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[1]) + " created a link to Router " +  get_id(routers[52])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[1],
                                 dst_node=routers[52],
                                 src_outport="South",
                                 dst_inport="West",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[2]) + " created a link to Router " +  get_id(routers[56])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[2],
                                 dst_node=routers[56],
                                 src_outport="South",
                                 dst_inport="West",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[3]) + " created a link to Router " +  get_id(routers[60])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[3],
                                 dst_node=routers[60],
                                 src_outport="South",
                                 dst_inport="West",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        

        print "Router " + get_id(routers[48]) + " created a link to Router " +  get_id(routers[0])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[48],
                                 dst_node=routers[0],
                                 src_outport="West",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[52]) + " created a link to Router " +  get_id(routers[1])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[52],
                                 dst_node=routers[1],
                                 src_outport="West",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[56]) + " created a link to Router " +  get_id(routers[2])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[56],
                                 dst_node=routers[2],
                                 src_outport="West",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[60]) + " created a link to Router " +  get_id(routers[3])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[60],
                                 dst_node=routers[3],
                                 src_outport="West",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print("2. Creating Chiplet2 and interposer connections")
        print "Router " + get_id(routers[16]) + " created a link to Router " +  get_id(routers[63])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[16],
                                 dst_node=routers[63],
                                 src_outport="South",
                                 dst_inport="East",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[17]) + " created a link to Router " +  get_id(routers[59])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[17],
                                 dst_node=routers[59],
                                 src_outport="South",
                                 dst_inport="East",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[18]) + " created a link to Router " +  get_id(routers[55])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[18],
                                 dst_node=routers[55],
                                 src_outport="South",
                                 dst_inport="East",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[19]) + " created a link to Router " +  get_id(routers[51])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[19],
                                 dst_node=routers[51],
                                 src_outport="South",
                                 dst_inport="East",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[63]) + " created a link to Router " +  get_id(routers[16])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[63],
                                 dst_node=routers[16],
                                 src_outport="East",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[59]) + " created a link to Router " +  get_id(routers[17])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[59],
                                 dst_node=routers[17],
                                 src_outport="East",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[55]) + " created a link to Router " +  get_id(routers[18])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[55],
                                 dst_node=routers[18],
                                 src_outport="East",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
    

        print "Router " + get_id(routers[51]) + " created a link to Router " +  get_id(routers[19])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[51],
                                 dst_node=routers[19],
                                 src_outport="East",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print("3. Creating Chiplet3 and interposer connections")
        print "Router " + get_id(routers[44]) + " created a link to Router " +  get_id(routers[48])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[44],
                                 dst_node=routers[48],
                                 src_outport="North",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[45]) + " created a link to Router " +  get_id(routers[49])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[45],
                                 dst_node=routers[49],
                                 src_outport="North",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[46]) + " created a link to Router " +  get_id(routers[50])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[46],
                                 dst_node=routers[50],
                                 src_outport="North",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[47]) + " created a link to Router " +  get_id(routers[51])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[47],
                                 dst_node=routers[51],
                                 src_outport="North",
                                 dst_inport="South",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[48]) + " created a link to Router " +  get_id(routers[44])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[48],
                                 dst_node=routers[44],
                                 src_outport="South",
                                 dst_inport="North",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1
        
        print "Router " + get_id(routers[49]) + " created a link to Router " +  get_id(routers[45])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[49],
                                 dst_node=routers[45],
                                 src_outport="South",
                                 dst_inport="North",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[50]) + " created a link to Router " +  get_id(routers[46])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[50],
                                 dst_node=routers[46],
                                 src_outport="South",
                                 dst_inport="North",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        print "Router " + get_id(routers[51]) + " created a link to Router " +  get_id(routers[47])
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[51],
                                 dst_node=routers[47],
                                 src_outport="South",
                                 dst_inport="North",
                                 latency = link_latency,
                                 weight=1))
        link_count += 1

        network.int_links = int_links

def get_id(node) :
    return str(node).split('.')[3].split('routers')[1]
