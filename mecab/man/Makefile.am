# Process this file with automake to produce Makefile.in

man_MANS = mecab.1
EXTRA_DIST = $(man_MANS) mecab1.ins mecab2.ins meca13.ins man.sh

update-man:
	@for man in $(man_MANS); do \
		cmd=`basename $$man .1` && \
		LANG=C ./man.sh ../src/$$cmd  > $$man; \
	done
