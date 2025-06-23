TEX = pdflatex
SRC = my_thesis.tex
CHAPTERS = chapter_1_introduction.tex chapter_2_lavori_correlati.tex chapter_3_contributo_personale.tex chapter_4_esperimenti.tex chapter_5_conclusioni.tex bibliografia.tex
TEXFLAGS = -interaction=nonstopmode -halt-on-error
TEXF = $(TEX) $(TEXFLAGS)
OUTDIR = tex_compile
PDF = my_thesis.pdf
PARENT = ..
IMAGEDIRS = images images/slab images/buddy images/bitmap

# Find all SVG files in IMAGEDIRS
SVGS = $(foreach dir,$(IMAGEDIRS),$(wildcard $(dir)/*.svg))
SVG_PDFS = $(SVGS:.svg=.pdf)

all: $(PARENT)/$(PDF)

# Rule to convert SVG to PDF using Inkscape
%.pdf: %.svg
	inkscape $< --export-filename=$@

# Main PDF compilation
$(OUTDIR)/$(PDF): $(SRC) $(CHAPTERS) $(SVG_PDFS)
	mkdir -p $(OUTDIR)
	$(TEXF) -output-directory=$(OUTDIR) $(SRC)
	$(TEXF) -output-directory=$(OUTDIR) $(SRC)  # Compile twice for references

$(PARENT)/$(PDF): $(OUTDIR)/$(PDF)
	cp $(OUTDIR)/$(PDF) $(PARENT)/
	cp $(OUTDIR)/$(PDF) ./

clean:
	rm -rf $(OUTDIR)/*
	rm -f $(PARENT)/$(PDF)
	rm -f ./$(PDF)

.PHONY: all clean