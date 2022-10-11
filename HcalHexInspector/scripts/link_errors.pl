#!/usr/bin/perl
#
# extract some info from getAll script output
# usage:  link_errors.pl output_directory
#
use strict;

my ($i, $v, @vals, $offs, $addr, @crates, $slot, @regs, $colr, $debug, $na, $stat);
my ($run_list, $run_count, $run, $dir, $crate, $cn, $line, $val);
my (%runs, @run_nums, %run_list, %crate_list);
my ($fed, $str, $key, %lines, $c);

$debug = 0;

$na = $#ARGV + 1;

@crates = ( "HBHEa/2", "HBHEa/3", "HF/1", "HO/1",
	    "HBHEa/1", "HBHEb/1", "HO/3", "HO/2",
	    "-", "HF/2", "HBHEc/3", "HBHEb/2",
	    "HF/3", "HO/4", "HBHEc/2", "HBHEb/3",
	    "laser", "HBHEc/1" );



if( $na == 0) {
    print "usage:  link_errors.pl <directories...>\n";
    exit;
}

# try to get the run number(s) from the arguments
print "PASS 1\n" if($debug);
foreach $dir ( @ARGV ) {
    print "Processing dir $dir\n" if($debug);
    if( $dir =~ /run\d+Out.*crate\d+/) {
	($run,$crate) = $dir =~ /run(\d+)Out.*crate(\d+)/;
	$runs{$run}++;
	$run_list{$dir} = $run;
	$crate_list{$dir} = $crate;
	print "parsed run=$run crate=$crate\n" if( $debug);
    } else {
	print "Unrecognized directory $dir... skipping\n";
    }
}

@run_nums = keys %runs;
$run_count = $#run_nums + 1;

if( $run_count != 1) {
    print "Expecting only 1 run in directory list!\n";
}

print "<h1>Run $run</h1>\n";

make_table( "UERR and CERR events as counted by DCC", 2,
	    "Corrected Errors", "Uncorrected Errors", 0x540, 0x580);

make_table( "HTR CRC Error and OrN mismatch", 2,
	    "HTR CRC Errors", "OrN Mismatch", 0x500, 0x680);

make_table( "EvN and BcN mismatches", 2,
	    "Event Number Mismatch", "Bunch Count Number Mismatch",
	    0x600, 0x640);

make_table( "DCC Re-Sync: Skipped and Padded Events", 2,
	    "Extra Events from HTR", "Missing Events from HTR",
	    0x6c0, 0x7c0);

#
# make a table
#   make_table( "title", columns, left_heading, right_heading, left_offset, right_offset);
#
sub make_table {
    my ($title, $columns, $left_hdg, $right_hdg, $left_off, $right_off) = @_;

    print "<h2>$title</h2>\n";

    print "<table border>\n";
    print "<tr><th>FED<th colspan=2>Crate<th>Slot<th colspan=15>$left_hdg<th colspan=15>$right_hdg\n";
    print "<tr><th colspan=4>&nbsp;";
    for( $i=0; $i<15; $i++) { print "<th>$i"; }
    for( $i=0; $i<15; $i++) { print "<th>$i"; }
    print "\n";

    print "PASS 2\n" if($debug);
    foreach $dir ( @ARGV ) {
	print "Processing $dir\n" if($debug);

	if( $dir =~ /run\d+Out.*crate\d+/) {

	    $run = $run_list{$dir};
	    $crate = $crate_list{$dir};

	    print "Looked up run $run crate $crate\n" if($debug);

	    open SDIR, "ls -1 $dir/*dccSlot*Status*.txt |" or die "opening ls pipe";

	    while( $stat = <SDIR>) {
		chomp $stat;
		print "   found status file $stat\n" if( $debug);
		($slot) = $stat =~ /dccSlot(\d+)Status/;
		$cn = $crates[$crate];
		print "   slot=$slot crate=$crate ($cn)\n" if( $debug);

		@regs = ();

		open SF, "< $stat" or die "opening $stat: $!\n";
		while( $line = <SF>) {
		    chomp $line;
		    if( $line =~ /^0...:/) {
			print "$line\n" if( $debug);
			@vals = split /\s+/, $line;
			$addr = hex($vals[0]);
			for( $i=1; $i<=$#vals; $i++) {
			    $offs = $addr+4*($i-1);
			    $val = hex($vals[$i]);
			    $regs[$offs] = $val;
			    printf "Set [0x%04x] = %d\n", $offs, $val if($debug);
			}
		    }
		}
		close SF;

		# now we have the register values at addresses 0x5.. in @regs
		# do the CERR
		$fed = $regs[0x1c];
		$str = "<tr><td>$fed<td>$cn<td>$crate <td>$slot ";
		for( $i=0; $i<15; $i++) {
		    $v = $regs[$left_off+4*$i];
		    $colr = "#ffe0e0";
		    $colr = "#e0e0ff" if( ($i % 2) == 0);
		    if( $v != 0) {
			$str .= sprintf "<td bgcolor=$colr>%d", $v;
		    } else {
			$str .= "<td bgcolor=$colr>";
		    }
		}

		# now the UERR
		for( $i=0; $i<15; $i++) {
		    $v = $regs[$right_off+4*$i];
		    $colr = "#ffe0e0";
		    $colr = "#e0e0ff" if( ($i % 2) == 0);
		    if( $v != 0) {
			$str .= sprintf "<td bgcolor=$colr>%d", $v;
		    } else {
			$str .= "<td bgcolor=$colr>";
		    }
		}
#	$key = sprintf "%02d-%02d", $crate ,$slot;
		$key = $fed;
		$lines{$key} = $str;
		print "key = $key\n" if( $debug);
	    }
	    close SDIR;
	}
    }

    foreach $c ( sort keys %lines ) {
	print $lines{$c} . "\n";
    }

    print "</table>\n";
}
