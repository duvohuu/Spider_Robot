BUILDDIR = build
MAIN     = main

.PHONY: all clean

all:
	mkdir -p $(BUILDDIR)
	pdflatex -output-directory=$(BUILDDIR) $(MAIN).tex
	pdflatex -output-directory=$(BUILDDIR) $(MAIN).tex
	cp $(BUILDDIR)/$(MAIN).pdf $(MAIN).pdf

clean:
	rm -rf $(BUILDDIR)
	rm -f $(MAIN).pdf
