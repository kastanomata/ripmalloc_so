TEX=pdflatex
SRC=my_thesis.tex
CHAPTERS=chapter_1_introduction.tex chapter_2_lavori_correlati.tex chapter_3_contributo_personale.tex chapter_4_esperimenti.tex chapter_5_conclusioni.tex
OUTDIR=tex_compile
PDF=my_thesis.pdf
PARENT=..

all: $(PARENT)/$(PDF)

$(OUTDIR)/$(PDF): $(SRC) $(CHAPTERS)
	mkdir -p $(OUTDIR)
	$(TEX) -output-directory=$(OUTDIR) $(SRC)
	$(TEX) -output-directory=$(OUTDIR) $(SRC)

$(PARENT)/$(PDF): $(OUTDIR)/$(PDF)
	cp $(OUTDIR)/$(PDF) $(PARENT)/
	cp $(OUTDIR)/$(PDF) ./

clean:
	rm -rf $(OUTDIR)/*
	rm -f $(PARENT)/$(PDF)
	rm -f ./$(PDF)

.PHONY: all clean