function check(x,	f) {
	f[x]
	printf "array_f subscript [%s]\n", x
	adump(f, -1)
}

BEGIN {
	check(3.0)
	check(-3)
	check("3.0")
	split(" 3", f, "|")	# create a maybe_num value
	check(f[1])
	check("0")
	check("-1")
}
