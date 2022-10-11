#!/usr/local/bin/perl
#
# look in DBS for a list of runs
#

$lo_run = $ARGV[0];
$hi_run = $ARGV[1];

print "Searching for $lo_run to $hi_run\n";

$dim_events = 10000;

$do_cms = 1;
$do_render = 1;

# $dumper = "DumpFEDRawDataProduct";
$dumper = "EricDIM";

$plot_dir = "~/public/web/DIM/Cruzet2";

$render_cmd = "/data/ehazen/work/CMSSW_2_0_5/src/EricRender.cc";
$template_cfg = "/data/ehazen/work/CMSSW_2_0_5/cfg/check_evn_template.cfg";
$temp_dir = "/data/ehazen/temp";
$cmssw_dir = "/data/ehazen/work/CMSSW_2_0_5/cfg";

my %file_list;

for( $run=$lo_run; $run<=$hi_run; $run++) {
    print "Looking for run $run...";

    $test_cmd = qq[~/bin/DDSearchCLI.py --input="find file where run in $run and dataset = /Cosmics/CRUZET2-v1/RAW" --details --limit=-1];

    open DDS, "$test_cmd |" or die "opening $test_cmd: $!";

    $file_list{$run} = [ ];
    $num_file{$run} = 0;

    while( $line = <DDS>) {
	chomp $line;
	if( $line =~ /LOGICALFILENAME/) {
	    ($file) = $line =~ /LOGICALFILENAME (\S+)/;
	    push @{$file_list{$run}}, $file;
	    $num_file{$run}++;
	}
    }
    close DDS;

    print "$num_file{$run} files found\n";

    if( $num_file{$run}) {

	$files = "";
	foreach $f ( @{$file_list{$run}} ) {
	    $files .= "'$f',";
	}
	chop $files;
	$files .= "\n";
	$files =~ s/,/,\n/g;

	$cfg_run = "${temp_dir}/Run${run}.cfg";
	$out_run = "${temp_dir}/Run${run}_output.txt";

	# make config file for checking FEDs
	make_conf( "DumpFEDRawDataProduct", $cfg_run, 1);

	if( $do_cms) {
	    print "Starting CMSSW to check for HCAL FEDs\n";
	    @feds = ();
	    $nfed = 0;
	    open CMS, "cmsRun $cfg_run |" or die "starting cmsRun $cfg_run: $!";
	    while( $line = <CMS>) {
		chomp $line;
		if( $line =~ /FED\#\s+7\d\d/) {
		    ($fed) = $line =~ /FED\#\s+(7\d\d)/;
		    push @feds, $fed;
		    $nfed++;
		}
	    }
	    close CMS;
	    print "$nfed HCAL feds in the run\n";
	}

	if( $nfed) {
	    # make config file for EricDIM
	    make_conf( "EricDIM", $cfg_run, $dim_events);

	    if( $do_cms) {
		print "Starting CMSSW for EricDIM for $dim_events events\n";
		system( "cmsRun $cfg_run");
	    }
	    

	    if( $do_render) {
		$root_cmd = "root -b -q '${render_cmd}(\"${temp_dir}/EricDIM_run${run}.root\")'";
		print "Running $root_cmd\n";
		system( $root_cmd);
		system("ps2pdf Eric_Plots.ps");
		system("mv Eric_Plots.pdf ${plot_dir}/EricDIM_run${run}.pdf");
	    }
	}

    }
}


# make_conf( dumper, cfg_file, evt_count)
sub make_conf {
    my $my_dumper = shift @_;
    my $my_cfg = shift @_;
    my $evt_count = shift @_;

    $abort_count = $evt_count + 10;

    # make the cfg file the hard way
    print "Creating config $my_cfg\n";
    open CFG, "> $my_cfg";
    print CFG "
      process DUMP = {
      untracked PSet maxEvents = {untracked int32 input = $evt_count}
      source = PoolSource {
	untracked vstring fileNames =  { 
    ";
    print CFG $files;
    print CFG "
	}
      }
      service = MessageLogger {
	untracked PSet default = { untracked int32 reportEvery   = 100 }
      }
      module dumperHex = $my_dumper  {
	    untracked vint32 feds = { 
		700,701,702,703,704,705, // HBHEa
		706,707,708,709,710,711, // HBHEb
		712,713,714,715,716,717, // HBHEc
		718,719,720,721,722,723, // HF
		724,725,726,727,728,729,730,731  // HO
	       }
	   untracked bool dumpPayload = false
	   untracked int32 dumpLevel = 0
	   untracked string RootFileName = \"${temp_dir}/EricDIM_run${run}.root\"
	   untracked bool debug = false
	   untracked uint32 DCCversion = 0x2c22
	   untracked bool dumpBinary = false
	   untracked string BinaryFileName = \"${temp_dir}/Run${run}_raw.dat\"
	   untracked int32 abort_count = $abort_count

      }

      path p = { dumperHex }
    }\n";
    close CFG;
}
