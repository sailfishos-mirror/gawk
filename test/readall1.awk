BEGIN {
	x = 5.9
	y = 3
	z = -2.327
	guide::answer = 42
	zebra[0] = "apple"
	zebra["archie"] = "banana"
	zebra[3]["foo"] = "bar"
	zebra[3]["bar"] = "foo"
	print writeall(ofile)
}
