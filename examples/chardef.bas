5 SCNCLR 
10 CHARDEF "$",24,36,32,112,32,98,92,0
20 READ C$, A,B,C,D,E,F,G,H
30 CHARDEF C$, A,B,C,D,E,F,G,H
40 PRINT "$$$ ###"
50 INPUT "PRESS ENTER" K$
60 CHARDEF C$,A,B,C,D,E,F,G,H
70 PRINT "READ VALUES:"
80 RCHARDEF C$,ISMONO, A,B,C,D,E,F,G,H 
100 PRINT "MONO? "; ISMONO
110 PRINT "   $";RIGHT$(HEX$(A), 8)
120 PRINT "   $";RIGHT$(HEX$(B), 8) 
130 PRINT "   $";RIGHT$(HEX$(C), 8) 
140 PRINT "   $";RIGHT$(HEX$(D), 8) 
150 PRINT "   $";RIGHT$(HEX$(E), 8) 
160 PRINT "   $";RIGHT$(HEX$(F), 8) 
170 PRINT "   $";RIGHT$(HEX$(G), 8) 
180 PRINT "   $";RIGHT$(HEX$(H), 8)
1000 REM MULTICOLOR IMAGE
1010 DATA "#"
1020 DATA $11111144
1030 DATA $11144444
1040 DATA $11144444
1050 DATA $14444444
1060 DATA $14444446
1070 DATA $14446666
1080 DATA $44446666
1090 DATA $44466666
