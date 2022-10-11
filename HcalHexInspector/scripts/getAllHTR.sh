#!/bin/bash

######
# This script runs a few scripts to collect 
# diagnostic data from the DCC and HTR
#
# first argument:  run number
# second argument: crate number/name
# third argument:  bus:device

# This script needs to know where it is
installDir="$HOME/GetStatus"

if [ "$1" == "" ]
  then
   run=13
else 
   run=$1
fi

if [ "$2" == "" ]
  then
   crate="no_crate"
else
   crate=$2
fi

if [ "$3" == "" ]
then
   bus="no_bus"
else
   bus=$3
fi

# Check for output directory
outputBaseDir="$HOME/GetStatusOutput"
if [ ! -d $outputBaseDir ]
then
  echo "Creating base direcitory: $outputBaseDir"
  mkdir $outputBaseDir
fi


echo ">>> Processing $crate bus $bus" 

runDir="$outputBaseDir/run"$run"Output"

if [ -d $runDir ]
then
  echo "$runDir already exists"
else
  echo "Creating $runDir"
  mkdir $runDir
fi

outputDir=$runDir"/"$crate
mkdir $outputDir

date=`eval date +%Y%m%d`

dccStatus_fn="dccStatusRun"$run"_"$date".txt"

version_fn=$outputDir"/run"$run"_versions.txt"
log_fn=$outputDir"/log.txt"

echo "run is " $run ". dccStatus filename is " $dccStatus_fn

rm -f $version_fn
echo "Firmware Versions:" > $version_fn

rm -f $log_fn
echo "Command Log:" > $log_fn

### DCC status, data, versions

for slot in 10 20 
do 

### get data from DCC Slot 10 ###
echo ">>>>> Running dump on DCC slot  ${slot}" 
dccDump_fn=$outputDir"/dccSlot${slot}Dump_run"$run"_"$date".dat"
rm -rf dccDump.cfg
echo "DCC/OPEN" $dccDump_fn > dccDump.cfg
#echo "DCC/NEXT" >> dccDump.cfg
echo "DCC/DUMP 999" >> dccDump.cfg
echo "q" >> dccDump.cfg
echo "q" >> dccDump.cfg

DCC2Tool.exe $bus $slot -x dccDump.cfg > /dev/null

### get status from DCC Slot 10 ###
echo ">>>>> Running status on DCC slot ${slot}" 
dccStatus_fn=$outputDir"/dccSlot${slot}Status_run"$run"_"$date".txt"
rm -f $dccStatus_fn
DCC2Tool.exe $bus $slot -x dcc2Status.cfg > $dccStatus_fn

DCC2Tool.exe $bus $slot -x dcc2Versions.cfg >> $version_fn
#DCCprogrammer.exe $bus $slot -i >> $version_fn

###############################
# Dump registers on DCC2
dccRegDump_fn=$outputDir"/dccSlot${slot}RegisterDump_run"$run"_"$date".dat"
DCC2Tool.exe $bus $slot -x $installDir/DumpSafe.dcc2 | strings > $dccRegDump_fn

done

echo ">>>>> Geting HTR versions" 
htr.exe -w $bus -f -m >> $version_fn

echo ">>>>> Getting env variables"
printenv >> $version_fn

#####################
# Now fetch HTR info
echo ">>>>> Getting HTR info"
source $XDAQ_ROOT/etc/env.sh
export HTR_HAL_TABLES=$XDAQ_ROOT/hal/hcal

# Generate new .htr_parameters file
# There is currently no way to specify the TTC bus on the command line and it will fail without it
rm -f $HOME/.htr_parameters
echo "VMEBUS $bus" > $HOME/.htr_parameters
echo "TTCBUS $bus" >> $HOME/.htr_parameters

mkdir $outputDir"/htrOutput"
for slot in  2 3 4 5 6 7 8 13 14 15 16 17 18
do 
   htr.exe -w $bus -x "$installDir/input/CaptureStatus_slot"$slot".htr" >> $outputDir"/htrOutput/output_CaptureStatus_slot"$slot".txt"
done


######################
echo "all done"
exit 0
