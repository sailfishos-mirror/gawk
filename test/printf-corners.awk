BEGIN {
	printf "<%#.o>\n", 0		# 0
	printf "<%#.1o>\n", 0		# 0
	printf "<%#.2o>\n", 0		# 00
	printf "<%#.3o>\n", 0		# 000
	printf "<%#.o>\n", 1		# 01
	printf "<%#.1o>\n", 1		# 01
	printf "<%#.2o>\n", 1		# 01
	printf "<%#.3o>\n", 1		# 001
	printf "<%#.2x>\n", 1		# 0x01
	printf "<%#.x>\n", 0 		# ""
	printf "<%+.d>\n", 0		# "+"
	printf "<%+.i>\n", 0		# "+"
	printf "<%+.u>\n", 0		# ""
	printf "<%+.o>\n", 0		# ""
	printf "<%+.x>\n", 0		# ""
	printf "<%+.X>\n", 0		# ""
	printf "<%+.s>\n", 0		# ""
	printf "<% .d>\n", 0 		# " "
	printf "<% .i>\n", 0 		# " "
	printf "<% .u>\n", 0 		# ""
	printf "<% .o>\n", 0 		# ""
	printf "<% .x>\n", 0 		# ""
	printf "<% .X>\n", 0 		# ""
	printf "<% .s>\n", 0 		# ""
	printf "<%+1.d>\n", 0		# "+"
	printf "<%+1.i>\n", 0		# "+"
	printf "<%+1.u>\n", 0		# " "
	printf "<%+1.o>\n", 0		# " "
	printf "<%+1.x>\n", 0		# " "
	printf "<%+1.X>\n", 0		# " "
	printf "<%+1.s>\n", 0		# " "
	printf "<% 1.d>\n", 0 		# " "
	printf "<% 1.i>\n", 0 		# " "
	printf "<% 1.u>\n", 0 		# " "
	printf "<% 1.o>\n", 0 		# " "
	printf "<% 1.x>\n", 0 		# " "
	printf "<% 1.X>\n", 0 		# " "
	printf "<% 1.s>\n", 0 		# " "
	printf "<%#g>\n", "0"		# "0.00000"
	printf "<%#G>\n", "0"		# "0.00000"
	printf "<%#.e>\n", "0"		# "0.e+00"
	printf "<%#.E>\n", "0"		# "0.E+00"
	printf "<%#.f>\n", "0"		# "0."
	printf "<%#.F>\n", "0"		# "0."
	printf "<%#.g>\n", "0"		# "0."
	printf "<%#.G>\n", "0"		# "0."
	printf "<%e>\n", "-nan"		# "-nan"
	printf "<%E>\n", "-nan"		# "-NAN"
	printf "<%f>\n", "-nan"		# "-nan"
	printf "<%F>\n", "-nan"		# "-NAN"
	printf "<%g>\n", "-nan"		# "-nan"
	printf "<%G>\n", "-nan"		# "-NAN"
	printf "<%a>\n", "-nan"		# "-nan"
	printf "<%A>\n", "-nan"		# "-NAN"
	printf "<%-20e>\n", "-inf"	# "-inf                "
	printf "<%-20E>\n", "-inf"	# "-INF                "
	printf "<%-20f>\n", "-inf"	# "-inf                "
	printf "<%-20F>\n", "-inf"	# "-INF                "
	printf "<%-20g>\n", "-inf"	# "-inf                "
	printf "<%-20G>\n", "-inf"	# "-INF                "
	printf "<%-20a>\n", "-inf"	# "-inf                "
	printf "<%-20A>\n", "-inf"	# "-INF                "
	printf "<%20e>\n", "-inf"	# "                -inf"
	printf "<%20E>\n", "-inf"	# "                -INF"
	printf "<%20f>\n", "-inf"	# "                -inf"
	printf "<%20F>\n", "-inf"	# "                -INF"
	printf "<%20g>\n", "-inf"	# "                -inf"
	printf "<%20G>\n", "-inf"	# "                -INF"
	printf "<%20a>\n", "-inf"	# "                -inf"
	printf "<%20A>\n", "-inf"	# "                -INF"
	printf "<%20.20e>\n", "-inf"	# "                -inf"
	printf "<%20.20E>\n", "-inf"	# "                -INF"
	printf "<%20.20f>\n", "-inf"	# "                -inf"
	printf "<%20.20F>\n", "-inf"	# "                -INF"
	printf "<%20.20g>\n", "-inf"	# "                -inf"
	printf "<%20.20G>\n", "-inf"	# "                -INF"
	printf "<%20.20a>\n", "-inf"	# "                -inf"
	printf "<%20.20A>\n", "-inf"	# "                -INF"
	printf "<%3$*2$.*1$d>\n", 5, 8, 45	# "   00045"
	printf "<%.2i>\n", -1		# "-01"
	printf "<%.3i>\n", -12		# "-012"
	printf "<%.3i>\n", -1		# "-001"
	printf "<%.2i>\n", 1		# "01"
	printf "<%+.2u>\n", 1		# "01"
	printf "<% .3u>\n", 12		# "012
	printf "<%+.2u>\n", 12		# "12"
}
