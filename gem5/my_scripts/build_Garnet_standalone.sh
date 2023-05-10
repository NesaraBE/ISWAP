#!/bin/tcsh
source my_scripts/set_env.cshrc
python `which scons` -j 64 build/Garnet_standalone/gem5.opt
