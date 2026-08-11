BEGIN {
	print typeof(var1)
	print typeof(SYMTAB["var1"])
	var1 = "42"
	print typeof(var1)
	print typeof(SYMTAB["var1"])
}
