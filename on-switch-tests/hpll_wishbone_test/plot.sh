#!/bin/bash

IP=192.168.1.6

scp root@$IP:/tmp/hpll_meas.dat ./

./convert_meas hpll_meas.dat hpll_plotdata.dat 100000

gnuplot -e "set xlabel 'time'; set ylabel 'DAC value'; set y2label 'Phase/freq error'; set yrange [-10000:10000]; set y2range [0:65540]; plot 'hpll_plotdata.dat' using 1:2 title 'Phase/freq error' with lines axis x1y1, \
	          'hpll_plotdata.dat' using 1:3 title 'DAC drive' with lines axis x1y2, \
	          'hpll_plotdata.dat' using 1:4 title 'Freq/Phase mode' with lines axis x1y2; \
             "
#, plot "hpll_plotdata.dat" using 1:3 title "integral value" with lines, plot "hpll_plotdata.dat" using 1:4 title "DAC drive" with lines'
