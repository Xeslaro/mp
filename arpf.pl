#!/usr/bin/perl -w
use strict;
while (<>) {
	chomp;
	if (/ses (0x[0-9a-f]+)/) {
		next if (/PADT/);
		qx!echo $1 | ./terminate!;
		print "terminating $1\n";
	}
}
