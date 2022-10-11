#!/bin/bash

######
# provide run number as first argument. 
# This script runs a few scripts to collect 
# diagnostic data from the DCC and HTR
#
#

if [ "$1" == "" ]
  then
   run=13
else 
   run=$1
fi

scriptUser=$USER

## HBHE
ssh $scriptUser@hcaldaq01 "cd GetStatus; sh getAllHTR.sh $run crate04 caen:0"
ssh $scriptUser@hcaldaq01 "cd GetStatus; sh getAllHTR.sh $run crate00 caen:1"
ssh $scriptUser@hcaldaq02 "cd GetStatus; sh getAllHTR.sh $run crate01 caen:0"
ssh $scriptUser@hcaldaq03 "cd GetStatus; sh getAllHTR.sh $run crate05 caen:0"
ssh $scriptUser@hcaldaq03 "cd GetStatus; sh getAllHTR.sh $run crate11 caen:1"
ssh $scriptUser@hcaldaq04 "cd GetStatus; sh getAllHTR.sh $run crate15 caen:0"
ssh $scriptUser@hcaldaq05 "cd GetStatus; sh getAllHTR.sh $run crate17 caen:0"
ssh $scriptUser@hcaldaq05 "cd GetStatus; sh getAllHTR.sh $run crate14 caen:1"
ssh $scriptUser@hcaldaq06 "cd GetStatus; sh getAllHTR.sh $run crate10 caen:0"

## HF
ssh $scriptUser@hcaldaq07 "cd GetStatus; sh getAllHTR.sh $run crate02 caen:0"
ssh $scriptUser@hcaldaq07 "cd GetStatus; sh getAllHTR.sh $run crate09 caen:1"
ssh $scriptUser@hcaldaq08 "cd GetStatus; sh getAllHTR.sh $run crate12 caen:0"

## HO
ssh $scriptUser@hcaldaq09 "cd GetStatus; sh getAllHTR.sh $run crate03 caen:0"
ssh $scriptUser@hcaldaq09 "cd GetStatus; sh getAllHTR.sh $run crate07 caen:1"
ssh $scriptUser@hcaldaq10 "cd GetStatus; sh getAllHTR.sh $run crate06 caen:1"
ssh $scriptUser@hcaldaq10 "cd GetStatus; sh getAllHTR.sh $run crate13 caen:0"

echo "all done"
exit 0
