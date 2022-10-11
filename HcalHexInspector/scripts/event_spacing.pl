#!/usr/bin/perl
#
# read a level >= 2 dump from FedGet and print event spacings
#

$my_fed = "x";
$first_orn = 0;
$nev = 0;
$tyme0 = 0;

while( $line = <>) {
    chomp $line;
    if( $line =~ /^\s*FED:/) {
	($fed,$evn,$bcn,$orn,$tts) = $line =~ /FED:\s+(\w+)\s+EvN:\s+(\w+)\s+BcN:\s+(\w+)\s+OrN:\s+(\w+)\s+TTS:\s(\w+)/;
#	print "FED [$fed] EVN [$evn] BCN [$bcn] ORN [$orn] TTS [$tts]\n";
	if( $my_fed eq "x") {
	    $my_fed = $fed;
	    $first_orn = hex $orn;
	    print "Using FED $my_fed for reference with OrN[0] = $first_orn\n";
	}
	$tyme = ((hex $orn) - $first_orn) * 3563 + hex $bcn;
	$dt = $tyme - $tyme0;
	if( $fed eq $my_fed) {
	    $nev++;
	    print "FED [$fed] EVN [$evn] BCN [$bcn] ORN [$orn] TTS [$tts]  L1A at $tyme BX  dt = $dt\n";
	}
	$tyme0 = $tyme;
    }
}

$ave_spc = $tyme / $nev;
$hertz = 1.0 / ($ave_spc * 25e-9);
print "$nev events in $tyme BX = average spacing of $ave_spc ($hertz Hz)\n";

