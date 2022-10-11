#!/usr/bin/perl
#
# read and analyze DCC register dumps from "GetStaus"
# displays dumped values using HAL table names
# first effort, has some problems still!
#
# needs to be able to find HAL table
#
# 

$na = $#ARGV+1;
if( $na < 1) {
    print "usage: dcc_dump_analyzer.pl <register_dump>\n";
    exit;
}

open DUMP, $ARGV[0] or die "Error opening $ARGV[0]";

if( !$ENV{"XDAQ_ROOT"}) {
    print "Please set XDAQ_ROOT so I can find the HAL table\n";
    exit;
}

$hal_xilinx = $ENV{"XDAQ_ROOT"} . "/hal/hcal/DCC2_XILINX_A32_r2.dat";

open HAL, "< $hal_xilinx" or die "Error opening HAL table $hal_xilinx";

# read and parse HAL table addresses, store by (decimal) address
#
while( $line = <HAL>) {
    chomp $line;
    if( $line =~ /^[a-zA-Z]/) {
	($name,$addr,$mask,$note) = $line =~ /^(\w+)\s+09\s+4\s+(\w\w\w\w\w\w\w\w)\s+(\w+)\s+\w+\s+\w+\s+(.+)/;
	$daddr = hex $addr;
	$name{$daddr} = $name;
	$note{$daddr} = $note;
	$mask{$daddr} = hex $mask;
    }
}

# read the dump, output all non-zero items
#
while( $line = <DUMP>) {
    chomp $line;
#    if( $line =~ /[0-9a-zA-Z]\:\s/) {
    if( $line =~ /[[:xdigit:]]\:\s/) {
#	print "$line\n";
	@w = split ' ', $line;
	$saddr = hex $w[0];
	for( $i=1; $i<=$#w; $i++) {
	    $addr = $saddr + 4 * ($i-1);
	    if( $addr <= 0x1000 || $addr >= 0x10000) {
		$val = hex $w[$i];
		if( $val != 0) {
		    if( $name{$addr}) {
			$mask_val = $val & $mask{$addr};
			printf "  %08x = %08x (%08x) [%s] %s\n", $addr, $val, $mask_val, $name{$addr}, $note{$addr};
		    } else {
			printf "  %08x = %08x\n", $addr, $val;
		    }
		}
	    }
	}
    }
}
