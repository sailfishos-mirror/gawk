@load "islocal"

BEGIN {
	foo()
	print ""

	foo(1)
	print ""

	foo(1, 2)
	print ""

	foo(an_undefined_var)
}

function foo(a, b, c)  
{
	print "a is local:", islocal(a) ? "true" : "false"
	print "b is local:", islocal(b) ? "true" : "false"
	print "c is local:", islocal(c) ? "true" : "false"
}
