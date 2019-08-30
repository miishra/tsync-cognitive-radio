set terminal png
set output 'TotalOverHeadBytes.png'
set title 'TotalOverHeadBytes Plot'
set xlabel 'Number of Ordinary Nodes'
set ylabel 'Total Overhead to Synchronize(Bytes)'
plot '-'  title 'TotalOverHeadBytes Data' with linespoints
10 9392.7
20 21463.2
40 51319.6
60 74539.3
80 109530
100 137752
e
