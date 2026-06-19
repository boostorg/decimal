# Build the Boost.Decimal TOMS paper outline against the UNMODIFIED ACM
# template in ../template. We do not copy or edit any template file; we only
# point LaTeX, BibTeX, and the bibliography resolver at that directory via the
# standard search-path variables. The trailing // means "search recursively".

PAPER     := toms_decimal
TEMPLATE  := ../template

export TEXINPUTS := .:$(TEMPLATE)//:
export BSTINPUTS := .:$(TEMPLATE)//:
export BIBINPUTS := .:$(TEMPLATE)//:

.PHONY: all clean

all: $(PAPER).pdf

$(PAPER).pdf: $(PAPER).tex references.bib
	pdflatex -interaction=nonstopmode $(PAPER).tex
	bibtex $(PAPER)
	pdflatex -interaction=nonstopmode $(PAPER).tex
	pdflatex -interaction=nonstopmode $(PAPER).tex

clean:
	rm -f $(PAPER).aux $(PAPER).bbl $(PAPER).blg $(PAPER).log \
	      $(PAPER).out $(PAPER).pdf $(PAPER).fls $(PAPER).fdb_latexmk
