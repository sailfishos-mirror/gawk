# input is the output of a "find"
# output is the list of stuff to test
# secondary output is another gawk program that tells the real name behind the "short" filename
# lang can be "en" or "it"
BEGIN {
	# in the temporary directory
	egi=s "/egidx.awk"
	print "#",s >egi
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
	# this is not part of the stuff to be tested,
	# it has been prepared in advance
	if ( wrk ~ "guide-it.po"  ) { next }
	nel=split(wrk,el,"/")
	if ( nel!=3 ) { next }
	wrk=el[3]
	gsub(".data","_data",wrk)
	gsub("-","_",wrk)
	print wrk;
	print "true[\"" wrk "\"]=\"" s "/" lang  "/eg/" el[2] "/" el[3] "\"" >egi
	print "alte[\"" wrk "\"]=\"" s "/" altl  "/eg/" el[2] "/" el[3] "\"" >egi
	if ( el[3]=="gettime.awk" ) {
		print "getlocaltime"
		print "true[\"getlocaltime\"]=\"" s "/" lang  "/eg/" el[2] "/" el[3] "\"" >egi
		print "alte[\"getlocaltime\"]=\"" s "/" altl  "/eg/" el[2] "/" el[3] "\"" >egi
	}
}

