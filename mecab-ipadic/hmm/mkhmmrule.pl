#!/usr/bin/perl -w

use strict;

sub convert {
    my $file = shift @_;
    my $section  = shift @_;
    my $type = shift @_;
    my @other;
    my @result;

    print "[$section]\n";
    open(F, $file) || die "$!: $file\n";
    while (<F>) {
        chomp;
        next if (/^\s*$/);
        next if (/^\#/);
        my ($cost, $group, $lex, $pos, $ctype, $cform);
        
        if ($type == 1) {
            ($cost, $group, $lex, $pos, $ctype, $cform) = split /\t/, $_;
        } else {
            ($lex, $pos, $ctype, $cform) = split /\t/, $_;
        }

        my @src = split "-", $pos;
        while (@src != 4) { push @src, "*"; }
        $ctype = "*" if (!$ctype || $ctype eq "");
        $cform = "*" if (!$cform || $ctype eq "");
        $group = ""  if (!$group || $group eq "");

        push @src, $ctype;
        push @src, $cform;
        push @src, $lex;

        my $is_other = 0;
        my @dst = ('$1', '$2', '$3', '$4', '$5', '$6', '$7');
        
        for (my $i = 0; $i < 7; ++$i) {
            if ($src[$i] eq "*") {
                $dst[$i] = "*";
            } elsif ($src[$i] eq "_all_") {
                $src[$i] = "*";
                $dst[$i] = "*";
            } elsif ($src[$i] eq "_each_") {
                $src[$i] = "*";
            } elsif ($src[$i] eq "_other_") {
                $src[$i] = "*";
                $dst[$i] = "*";
                $is_other = 1;
            } else {
                $dst[$i] = $src[$i];
            }
        }

        my $src_str = join ",", @src;

        # wordlex
        my $dst_str = join ",", @dst;
        if ($type == 0) {
            $dst_str = "$dst_str \$7";
        }

        my $out = "";
        if ($group eq "") {
            $out = "$src_str\t\t\t$dst_str\n";
        } else {
            $out = "$src_str\t\t\t$group\n";
        }

        if ($is_other) {
            push @other,  $out;
        } else {
            push @result, $out;
        }

    }

    for (@result) { print; }

    if (@other)  {
        print "\n### Other rules\n";
        for (@other) { print; }
    }
        
    print "\n### default rules\n";
    if ($type == 1) {
        print "*,*,*,*\t\t", '$1,$2,$3,$4', "\n";
    } else {
        print "*,*,*,*,*,*,*\t\t", '$1,$2,$3,$4,*,* $7', "\n";
    }
    print "\n";
}

&convert("wordext.seed", "unigram rewrite", 0);
&convert("edit1.seed",   "right rewrite",   1);
&convert("edit2.seed",   "left rewrite",    1);

