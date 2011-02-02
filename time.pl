#!/usr/bin/perl
use warnings;
use strict;
use Time::HiRes qw(gettimeofday tv_interval);
my ($prog, $file, $times) = @ARGV;
my $args;
{
	my $fp;
	open $fp, '<', $file or die "Couldn't open $file for reading";
	$args = <$fp>;
	chomp $args;
	close $fp;
}
my $fp;
open $fp, ">>", $file or die "Couldn't open $file for appending";
print $fp "$prog\t";
$SIG{INT} = "onexit";
for my $seed (1..$times) {
	my $t0 = [gettimeofday];
	system "$prog $args --seed $seed";
	my $elapsed = substr tv_interval($t0, [gettimeofday]), 0, 7;
	print $fp "$elapsed\t";
}
sub onexit {
	print $fp "\n";
	close $fp;
	$SIG{INT} = "DEFAULT";
}
onexit;
