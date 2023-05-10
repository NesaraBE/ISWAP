# ISWAP
Interposer SWAP to avoid deadlocks in heterogenous NoCs

Please download gem5 "src" and "test" directories separately: https://www.gem5.org/getting_started/ 

On top of the vanilla gem5, please overwrite the files from this repository

Steps to run each of iSWAP configirations -


To induce deadlock with Random oblvious routing in Interposer -
1. Set iSWAP_enable variable to 1 in RoutingUnit.cc
2. Run ./my_scripts/build_Garnet_standalone.sh
3. source run_cmd_swap_disable.sh bash file
4. Deadlock will be detected.

To resolve the deadlock induced -
1. source run_cmd_swap_enable.sh bash file
2. Deadlock will be resovled.
3. Check nextwork_stat.txt file to confirm number of packets.

To induce deadlock with Turns enable -
1. Set deadlock_check variable as 1 in RoutingUnit.cc 
2. Reset iSWAP_enable variable to 0 in RoutingUnit.cc 
3. Run ./my_scripts/build_Garnet_standalone.sh
4. source "run_cmd_swap_disable.sh" bash file
5. Deadlock will be detected.

To resolve the deadlock with composable routing -
1. Set composable variable to 1 in RoutingUnit.cc.
2. Run ./my_scripts/build_Garnet_standalone.sh
3. source run_cmd_composable_routing.sh bash file
4. Deadlock wont occur

Note: Either one of the above mentioned variable should be 1. Default iSWAP_enable variable will be set in RoutingUnit.cc 


