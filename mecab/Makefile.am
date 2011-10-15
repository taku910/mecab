# Process this file with automake to produce Makefile.in
#
# $Id: Makefile.am,v 1.7 2006/05/29 15:36:08 taku-ku Exp $;
#
@SET_MAKE@
AUTOMAKE_OPTIONS = 1.4 no-dependencies
SUBDIRS = src man doc tests
confdir = $(sysconfdir)
conf_DATA = mecabrc
EXTRA_DIST = LGPL GPL BSD Makefile.train
EXTRA_DIR = swig example
bin_SCRIPTS = mecab-config

dist-hook:
	for subdir in $(EXTRA_DIR); do \
	  cp -rp $$subdir $(distdir); \
	  rm -f $(distdir)/$$subdir/*~; \
	  rm -f $(distdir)/$$subdir/*.{bak,orig}; \
	  rm -rf $(distdir)/$$subdir/CVS; \
	  rm -rf $(distdir)/$$subdir/*/CVS; \
	  rm -rf $(distdir)/$$subdir/.svn; \
	  rm -rf $(distdir)/.svn; \
	  rm -rf $(distdir)/*/.svn; \
	  rm -rf $(distdir)/*/*/.svn; \
	  rm -rf $(distdir)/$$subdir/*/CVS; \
	done

rpm: dist
	env PATH="/usr/bin:$$PATH" rpmbuild -ta @PACKAGE@-@VERSION@.tar.gz

win:
	make
	(cd src; wcmd /c make.bat)

win-dic:	
	(cd dic; ../src/mecab-dict-index -f EUC-JP -t SHIFT-JIS)

win-dist:
	rm -fr wintmp @PACKAGE@-@VERSION@.exe
	cp -fr example wintmp
	perl -pe 's#\@prefix\@/lib/mecab/dic/ipadic#\$$(rcpath)\\..\\dic\\ipadic#g;' \
	     < mecabrc.in > wintmp/mecabrc
	(cd wintmp; qkc -m -s -c *.c *.cpp)
	mkdir wintmp/dic
	cp -f dic/*.csv dic/*.def dic/dicrc wintmp/dic
	sed s/EUC-JP/SHIFT-JIS/ dic/dicrc > wintmp/dic/dicrc
	(cd wintmp/dic; qkc -m -s -c *)	
	wine ~/.root/bin/inno/ISCC.exe mecab.iss
	rm -fr wintmp

script-dist:
	for subdir in perl ruby python java; do \
	  rm -fr mecab-$${subdir}-@VERSION@; \
	  mkdir mecab-$${subdir}-@VERSION@; \
	  cp -r COPYING BSD LGPL GPL AUTHORS mecab-$${subdir}-@VERSION@; \
	  cp -r $${subdir}/* mecab-$${subdir}-@VERSION@; \
	  cp -r doc/bindings.html  mecab-$${subdir}-@VERSION@; \
	  find ./mecab-$${subdir}-@VERSION@ -type d -name CVS | xargs rm -fr; \
	  find ./mecab-$${subdir}-@VERSION@ -type d -name .svn | xargs rm -fr; \
	  rm -f mecab-$${subdir}-@VERSION@/*~; \
	  tar zcfv mecab-$${subdir}-@VERSION@.tar.gz mecab-$${subdir}-@VERSION@; \
	  rm -fr mecab-$${subdir}-@VERSION@; \
	done

script-clean:
	(cd perl; $(MAKE) clean; rm -f Makefile.old Makefile)
	(cd ruby; $(MAKE) clean; rm -f Makefile *.log)
	(cd python; python setup.py clean --all)
	(cd java; $(MAKE) clean;)

dist-all-package:
	(cd swig; $(MAKE))
	$(MAKE) script-clean
	(test -f Makefile) && $(MAKE) distclean
	./configure
	$(MAKE) dist
	$(MAKE) script-dist
	$(MAKE) clean
	$(MAKE) win
	$(MAKE) win-dic
	$(MAKE) win-dist

export-package:
	./upload.pl -p mecab -n mecab        -r @VERSION@ -f mecab-@VERSION@.tar.gz
	./upload.pl -p mecab -n mecab-win32  -r @VERSION@ -f mecab-@VERSION@.exe
	./upload.pl -p mecab -n mecab-perl   -r @VERSION@ -f mecab-perl-@VERSION@.tar.gz
	./upload.pl -p mecab -n mecab-ruby   -r @VERSION@ -f mecab-ruby-@VERSION@.tar.gz
	./upload.pl -p mecab -n mecab-python -r @VERSION@ -f mecab-python-@VERSION@.tar.gz
	./upload.pl -p mecab -n mecab-java   -r @VERSION@ -f mecab-java-@VERSION@.tar.gz
	scp doc/*.html sf:public_html/mecab/
	scp -r doc/doxygen sf:public_html/mecab/
	
