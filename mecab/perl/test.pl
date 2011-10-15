#!/usr/bin/perl

use lib "../src/.libs";
use lib $ENV{PWD} . "/blib/lib";
use lib $ENV{PWD} . "/blib/arch";
use MeCab;

print $MeCab::VERSION, "\n";

my $sentence = "太郎はこの本を二郎を見た女性に渡した。";

my $c = new MeCab::Tagger(join " ", @ARGV);

print $c->parse($sentence);
for (my $m = $c->parseToNode($sentence); $m; $m = $m->{next}) {
    printf("%s\t%s\n", $m->{surface}, $m->{feature});
}

my $m = $c->parseToNode($sentence);
my $len = $m->{sentence_length};
for (my $i = 0; $i <= $len; ++$i) {
    for (my $b = $m->begin_node_list($i); $b; $b = $b->{bnext}) {
	printf("B[%d] %s\t%s\n", $i, $b->{surface}, $b->{feature});
    }
    for (my $e = $m->end_node_list($i); $e; $e = $e->{enext}) {
	printf("E[%d] %s\t%s\n", $i, $e->{surface}, $e->{feature});	
    }    
}

for (my $d = $c->dictionary_info(); $d; $d = $d->{next}) {
    printf("filename: %s\n", $d->{filename});
    printf("charset: %s\n", $d->{charset});
    printf("size: %d\n", $d->{size});
    printf("type: %d\n", $d->{type});
    printf("lsize: %d\n", $d->{lsize});
    printf("rsize: %d\n", $d->{rsize});
    printf("version: %d\n", $d->{version});
}
