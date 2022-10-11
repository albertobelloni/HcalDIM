#!/usr/bin/env python

import os

if not os.path.exists(os.path.join(os.curdir,"input/CaptureStatusOutput")):
    os.mkdir(os.path.join(os.curdir,"input/CaptureStatusOutput"))

outputdir=os.path.join(os.curdir,"CaptureStatusOutput")

for i in [2,3,4,5,6,7,8,13,14,15,16,17,18]:

    print "Executing on slot # ",i
    cmd="htr.exe -x input/CaptureStatus_slot%i.htr > %s/output_CaptureStatus_slot%i.txt"%(i,outputdir,i)
    os.system(cmd)
    #print cmd
