#With Swap enable run this below command -
./build/Garnet_standalone/gem5.opt configs/example/garnet_synth_traffic.py \
--network=garnet2.0 \
--num-cpus=64 \
--num-dirs=64 \
--mesh-rows=8 \
--topology=Het_meshs \
--sim-cycles=200000 \
--num-packets-max=60 \
--injectionrate=0.50 \
--vcs-per-vnet=2 \
--inj-vnet=0 \
--synthetic=inter_mesh_traffic \
--routing-algorithm=custom \
--interswap=1 \
--whenToSwap=1 \
--whichToSwap=1 \
--no-is-swap=1 
#--enable-loupe
#--topology=Het_meshs \
#--synthetic=inter_mesh_traffic \
#--routing-algorithm=custom \
#--num-packets-max=60 \

