BEGIN {
	printf "<%#.2o>\n", 1		# 01
	printf "<%#.2x>\n", 1		# 0x01
	printf "<%#.x>\n", 0 		# ""
	printf "<%+.i>\n", 0		# "+"
	printf "<% .i>\n", 0 		# " "
	printf "<%e>\n", "-nan"		# "-nan"
	printf "<%e>\n", "inf"		# "inf"
	printf "<% e>\n", "inf"		# " inf"
	printf "<%20e>\n", "-inf"	# "                -inf"
}
