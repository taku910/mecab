#!/usr/bin/ruby

require 'MeCab'
sentence = "太郎はこの本を二郎を見た女性に渡した。"

begin
     
     print MeCab::VERSION, "\n"	
     c = MeCab::Tagger.new(ARGV.join(" "))

     puts c.parse(sentence)

     n = c.parseToNode(sentence)

     while n do
       print n.surface,  "\t", n.feature, "\t", n.cost, "\n"
       n = n.next
     end
     print "EOS\n";
     
     n = c.parseToNode(sentence)
     len = n.sentence_length;
     for i in 0..len
     	 b = n.begin_node_list(i)
         while b do
           printf "B[%d] %s\t%s\n", i, b.surface, b.feature;
	   b = b.bnext 
	 end
     	 e = n.end_node_list(i)	 
         while e do
           printf "E[%d] %s\t%s\n", i, e.surface, e.feature;
	   e = e.bnext 
	 end  
     end
     print "EOS\n";
				
     d = c.dictionary_info()
     while d do
        printf "filename: %s\n", d.filename
        printf "charset: %s\n", d.charset
        printf "size: %d\n", d.size
        printf "type: %d\n", d.type
        printf "lsize: %d\n", d.lsize
        printf "rsize: %d\n", d.rsize
        printf "version: %d\n", d.version
        d = d.next
     end
     
rescue
      print "RuntimeError: ", $!, "\n";
end
