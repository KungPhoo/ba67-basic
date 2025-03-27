10 DIM B%(8) 
20 RCHARDEF "X", MONO, B%(1),B%(2),B%(3),B%(4),B%(5),B%(6),B%(7),B%(8)   
30 FOR B=1 TO 8 
40 PRINT B,B%(B) 
50 NEXT 
60 CHARDEF "Y", B%(1),B%(2),B%(3),B%(4),B%(5),B%(6),B%(7),B%(8)
70 PRINT "XY"
