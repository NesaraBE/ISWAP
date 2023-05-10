#Run this file to disable SWAP and induce deadlock using random oblivious routing in regular mesh
./build/Garnet_standalone/gem5.opt configs/example/garnet_synth_traffic.py \
--network=garnet2.0 \
--num-cpus=64 \
--num-dirs=64 \
--mesh-rows=8 \
--topology=Mesh \
--sim-cycles=200000 \
--num-packets-max=60 \
--injectionrate=0.50 \
--vcs-per-vnet=1 \
--inj-vnet=0 \
--synthetic=uniform_random \
#--routing-algorithm=xy
#--routing-algorithm=random_oblivious 
#--topology=Het_meshs \
#--synthetic=inter_mesh_traffic \
#--routing-algorithm=custom \
#--num-packets-max=60 \