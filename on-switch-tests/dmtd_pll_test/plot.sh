#!/bin/bash

IP=192.168.1.6

scp root@$IP:/tmp/dmpll_meas.dat ./

gcc convert_meas.c -o convert_meas -I../../include

./convert_meas dmpll_meas.dat dmpll_plotdata.dat 40000

gnuplot -e "set xlabel 'time'; set y2label 'DAC value'; set ylabel 'Phase/freq error'; set yrange [-10000:10000]; set y2range [0:65540]; plot 'dmpll_plotdata.dat' using 1:2 title 'Phase/freq error' with lines axis x1y1, \
	          'dmpll_plotdata.dat' using 1:3 title 'DAC drive' with lines axis x1y2, \
	          'dmpll_plotdata.dat' using 1:4 title 'Freq/Phase mode' with lines axis x1y2; \
             "
#, plot "dmpll_plotdata.dat" using 1:3 title "integral value" with lines, plot "dmpll_plotdata.dat" using 1:4 title "DAC drive" with lines'
