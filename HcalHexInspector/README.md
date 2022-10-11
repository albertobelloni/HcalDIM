# EricDIM: tools for checking health of HCAL data

The EricDIM suite of tools started its development in 2008, by the hands of Eric Hazen (BU). Documentation about the main tools is available at the following twiki page:

    https://twiki.cern.ch/twiki/bin/view/Main/EricDQM

The current versions run with `CMSSW 10_6_30`; the instructions on lxplus CentOS7 are not unusual:

```
	cmsrel CMSSW_10_6_22
	cd CMSSW_10_6_22/src
	cmsenv
	git clone ...
	scram b -j 8
```

The main workhorse is the EDM plugin EricDIM; it is run using the python script `test/test_cfg.py`.

List of tools:

- `MonRun`:
- `TestMon`:
- `CheckZS`:
- `dump_DTC`:
- `dump_FED`:
- `dump_sort`:
- `GetDccTTS`:

List of scripts:

- `dcc_dump_analyzer.pl`:
- `getAllCrates.sh`:
- `getAllHTR.sh`:
- `link_errors.pl`"
- `event_spacing.pl`:
- `CaptureStatus.py`:
- `DDSearchCLI.py`:
- `find_runs_dbs.pl`:

Original set of files in the non-CMSSW-compliant part of the package:

```
.
|-- AutoDIM
|   |-- DDSearchCLI.py
|   |-- EricRender.cc
|   |-- README.txt
|   `-- find_runs_dbs.pl
|-- EricRender
|   |-- ChangeLog
|   `-- EricRender.cc
|-- GetStatus
|   |-- ChangeLog
|   |-- DumpSafe.dcc
|   |-- DumpSafe.dcc2
|   |-- dccStatus.cfg
|   |-- dcc_dump_analyzer.pl
|   |-- getAllCrates.sh
|   |-- getAllHTR.sh
|   |-- input
|   |   |-- CaptureStatus.py
|   |   |-- CaptureStatusOutput
|   |   |-- CaptureStatus_slot13.htr
|   |   |-- CaptureStatus_slot14.htr
|   |   |-- CaptureStatus_slot15.htr
|   |   |-- CaptureStatus_slot16.htr
|   |   |-- CaptureStatus_slot17.htr
|   |   |-- CaptureStatus_slot18.htr
|   |   |-- CaptureStatus_slot2.htr
|   |   |-- CaptureStatus_slot3.htr
|   |   |-- CaptureStatus_slot4.htr
|   |   |-- CaptureStatus_slot5.htr
|   |   |-- CaptureStatus_slot6.htr
|   |   |-- CaptureStatus_slot7.htr
|   |   `-- CaptureStatus_slot8.htr
|   `-- link_errors.pl
|-- MonScan
|   |-- ChangeLog
|   |-- GetDccTTS.cc
|   |-- Makefile
|   |-- MonFile.cc
|   |-- MonFile.hh
|   |-- MonRun.cc
|   |-- README.txt
|   |-- TestMon.cc
|   |-- Tokenize.cc
|   `-- Tokenize.hh
`-- RawAnalysis
    |-- ChangeLog
    |-- CheckZS.cc
    |-- Makefile
    |-- crc16d64.cc
    |-- crc16d64.hh
    |-- dccv4_format.h
    |-- dump_DTC.cc
    |-- dump_FED.cc
    |-- dump_FED.cc_old
    |-- dump_sort.cc
    |-- event_spacing.pl
    `-- undump_new_fedkit.cc

7 directories, 50 files
```
