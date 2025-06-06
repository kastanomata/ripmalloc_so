TEX=pdflatex
SRC=my_thesis.tex
OUTDIR=tex_compile
PDF=my_thesis.pdf
PARENT=..

all: $(PARENT)/$(PDF)

$(OUTDIR)/$(PDF): $(SRC)
		mkdir -p $(OUTDIR)
		$(TEX) -output-directory=$(OUTDIR) $(SRC)
		$(TEX) -output-directory=$(OUTDIR) $(SRC)

$(PARENT)/$(PDF): $(OUTDIR)/$(PDF)
		cp $(OUTDIR)/$(PDF) $(PARENT)/

clean:
		rm -rf $(OUTDIR)/*
		rm -f $(PARENT)/$(PDF)

.PHONY: all clean