#With Swap enable run this below command -
./build/Garnet_standalone/gem5.opt configs/example/garnet_synth_traffic.py \
--network=garnet2.0 \
--num-cpus=64 \
--num-dirs=64 \
--mesh-rows=8 \
--topology=Mesh \
--sim-cycles=200000 \
--injectionrate=0.50 \
--num-packets-max=60 \
--vcs-per-vnet=1 \
--inj-single-vnet=0 \
--synthetic=uniform_random \
--interswap=1 \
--whenToSwap=1 \
--whichToSwap=1 \
--no-is-swap=1 
#--routing-algorithm=random_oblivious \
#--num-packets-max=60 \
