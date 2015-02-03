all : wrs_failures.pdf

.PHONY : all clean

wrs_failures.pdf : wrs_failures.tex fail.tex intro.tex snmp_exports.tex
	pdflatex wrs_failures.tex
	pdflatex wrs_failures.tex

clean :
	rm -f *.eps *.pdf *.dat *.log *.out *.aux *.dvi *.ps *.toc

