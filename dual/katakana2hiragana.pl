#!/usr/bin/perl


use strict;
use utf8;
use warnings;
use open IO  => ":utf8";
use open ":std";

while (<>) {
    chomp;
    if (/EOS/) {
	print "EOS\n";
    } else {
	# 梅棹    名詞,固有名詞,人名,姓,*,*,梅棹,ウメサオ,
	my ($s, $n) = split /\t/, $_;
	my @a = split /,/, $n;
	my $h = $a[7];
	$h =~ tr/ア-ン/あ-ん/;
	print "$h\t$n\n";
    }
}
