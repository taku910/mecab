mecab_dict_index = @MECAB_DICT_INDEX@
dicdir           = @MECAB_DICDIR@

# SUBDIRS    = doc script
dic_DATA   = @MECAB_GENDATA@ left-id.def right-id.def rewrite.def dicrc pos-id.def
EXTRA_DIST = @MECAB_LEXICAL_DIC@ @MECAB_PREDATA@ RESULT
CLEANFILES = @MECAB_GENDATA@

@MECAB_GENDATA@:
	$(mecab_dict_index) -f euc-jp -t @CHARSET@
	echo To enable dictionary, rewrite @MECAB_MECABRC@ as \"dicrc = @MECAB_DICDIR@\"

export-package:
	./upload.pl -p mecab -n @PACKAGE@ -r @VERSION@ -f @PACKAGE@-@VERSION@.tar.gz
