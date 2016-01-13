all : wrs_failures.pdf

.PHONY : all clean

RELEASE = $(shell git describe --always --dirty)

wrs_failures.pdf : wrs_failures.tex fail.tex intro.tex snmp_exports.tex snmp_objects.tex
	@echo '\\newcommand{\\gitrevinfo}{'$(RELEASE)'}' > revinfo.tex
	pdflatex wrs_failures.tex
	pdflatex wrs_failures.tex

clean :
	rm -f *.eps  *.dat *.log *.out *.aux *.dvi *.ps *.toc *.pdf revinfo.tex

