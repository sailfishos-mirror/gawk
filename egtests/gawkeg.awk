# input is the output of a "find"
# output is the list of stuff to test
# secondary output is another gawk program that tells the real name behind the "short" filename
# lang can be "en" or "it"
BEGIN {
	egi="../egidx.awk"
	if ( lang=="en" ) { altl="it" }
	if ( lang=="it" ) { altl="en" }
	print "BEGIN { do_init() }" >egi
	print " t==\"true\" { print true[$0]; exit }" >egi
	print " t==\"alte\" { print alte[$0]; exit }" >egi
	print "function do_init() {" >egi

}

END { print "}" >egi }

{	wrk=$0
	if ( wrk ~ ".un~"      ) { next }
	if ( wrk ~ ".swp"      ) { next }
	if ( wrk ~ ".swo"      ) { next }
	if ( wrk ~ "guide-it"  ) { next }
	nel=split(wrk,el,"/")
	if ( nel!=3 ) { next }
	gsub(".data","_data",el[3])
	gsub("-","_",el[3])
	print el[3];
	if ( el[2]=="prog" ) {
		print "true[\"" el[3] "\"]=\"" el[3] "\"" >egi
		print "alte[\"" el[3] "\"]=\"../../../" altl "/eg/prog/" el[3] "\"" >egi
	} else {
		print "true[\"" el[3] "\"]=\"." $0    "\"" >egi
		print "alte[\"" el[3] "\"]=\"../../../" altl "/eg/" el[2] "/" el[3] "\"" >egi
	}
	if ( el[3]=="gettime.awk" ) {
		print "getlocaltime"
		print "true[\"getlocaltime\"]=\"." $0    "\"" >egi;
		print "alte[\"getlocaltime\"]=\"../../../" altl "/eg/" el[2] "/" el[3] "\"" >egi
	}
}
