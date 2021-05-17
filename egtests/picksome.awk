# given a list
# display it and accept a choice from the user
BEGIN { # input string
	if ( choice=="" ) { msg() }
	# directory in which to write the user choices
	if ( s==""    )   { msg() }
	# what to produce, list of names or command
	if ( what=="" )   { msg() }
	nel=split(choice,el,"%%")
	if ( what=="f" )  {
		print el[1]> s "/sublst"
		exit
	}
	if ( what=="c" )  {
		print el[2]> s "/subcmd"
		exit
	}
	# if it gets here, something is wrong
	msg()
}

function msg() {
	print "picksome: invoked with wrong or missing parameters, nothing done"
	exit(4)
}
