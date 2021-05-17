# given a list of tests to perform in "list"
# each name in the list does NOT contain a space char
# display it by "cols" and accept a choice from the user
# the display line is always long 80,
# the visible part of each choice is reduced accordingly
BEGIN { # number of columns in the display (max 6)
	if ( cols=="" ) { exit(4) }
	# check that indeed the cols are between 1 and 6
	if ( cols>6 || cols<0 ) { cols=6 }
	# list of alternatives
	if ( list=="" ) { exit(4) }
	# directory in which to write the user choices
	if ( s==""    ) { exit(4) }
	#print list
	do_init()
	size=int(80/cols)
	nel=split(list,el," ")
	#print "visualizing",nel,"choices in",cols,"columns, each choice uses max",size,"chars"
	# real size is 5 less... "nnn choice.... "
	namesize=size-5
	# size,namesize
	#print "# size,namesize = " size,namesize;
	build_rows()
	# if valid becomes "n", prompt again for input
	valid="n"
	msg=""
	while ( valid=="n" ) {
		display_rows()
		if ( msg!="" ) { print "===>",msg }
		printf "===> Which test(s)? [1-" nel ",q,h,...] "
		getline
		build_line()
		# either line or cmds could be empty (hopefully not both)
		# line,cmds
		#print "# line,cmds = " line,cmds;
		print line "%%" cmds > s "/choice"
	}
	exit
}

function build_rows() {
	# each element is in col[row,number] last column may be shorter
	nrows=int(nel/cols)
	if ( nrows*cols<nel ) { nrows++ }
	#print nrows,"rows necessary"
	nrow=1;
	for ( i=1; i<=nrows*cols; i++ ) {
		nchoice=i
		cchoice=i""
		while ( length(cchoice)<3 ) { cchoice=" " cchoice }
		cdesc=el[i] ""
		if    ( length(cdesc)>namesize ) { cdesc= substr(cdesc,1,namesize) }
		while ( length(cdesc)<namesize ) { cdesc= cdesc " " }
		if ( cdesc==substr(blancs,1,namesize ) ) { cchoice="   " }
		row[nrow]=row[nrow] cchoice " " cdesc " "
		nrow++
		if ( nrow>nrows ) { nrow=1 }
	}
	# drop the last character, which is a space anyway
	for ( i=1; i<=nrows; i++ ) { row[i]=substr(row[i],1,length(row[i])-1) }
}

function display_rows() {
	for ( i=1; i<=nrows; i++ ) {
		print row[i]
	}
}

function build_line() {
	# each file name is in el[1...nel]
	wrk=$0
	# it could be a list, to be built, something like:
	# q  /  ,h  / 1-7,5,49f
	# each fi[] is either a single digit or a range (or a char)
	# containing a list of test names
	# containing a command
	# if there are commands, they are at the end
	chk_final_letters()
	# wrk
	#print "# wrk = " wrk;
	cmds=""
	nfi=split(wrk,fi,",")
	for ( i=1; i<=nfi; i++ ) {
		item=fi[i]
		eval_item()
	}
	if ( valid=="n" ) { return }
	line=""
	for ( i=1; i<=nel; i++ ){
		if ( use[i]=="y" ) { line=line el[i] " " }
	}
	# drop final space
	line=substr(line,1,length(line)-1)
}

function do_init() {
	numbers="0123456789"
	commands=" b d e h q Q quit w x ZZ zz "
	letters="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	blancs="                                                                                                "
}

function eval_item() {
	# could be a range, a single number, a single letter or more combinations, comma separated
	# out of range numbers are flagged as invalid
	#
	# single number or letter
	#print "evaluating",item
	if ( length(item)==1 ) {
		idx=index(numbers,item)
		# if it is a number
		if ( idx>0 ) {
			use[item]="y"
			valid="y"
			return
		}
		tmp=" " item " "
		#print tmp
		idx=index(commands,tmp)
		#print idx
		if ( idx>0 ) {
			cmds=cmds item
			valid="y"
			return
		} else {
			msg="Invalid command " item ", please give a valid one"
			valid="n"
			return
		}
	}
	#
	# again a number
	#
	if ( length(item)==2 ) {
		if ( item+0>9 && item+0<=nel ) {
			use[item]="y"
			valid="y"
			return
		}
	}
	# range, or more than one char
	nra=split(item,ra,"-")
	if ( nra==2 ) {
		if ( ra[1]>ra[2] ) {
			msg="Invalid range " item ", please give a valid one"
			valid="n"
			return
		}
		if ( ra[1]<1 || ra[2]>nel ) {
			msg="Invalid range " item ", please give a valid one"
			valid="n"
			return
		}
		for ( k=ra[1]; k<=ra[2]; k++ ) {
			use[k]="y"
		}
		valid="y"
		return
	}
	# more than one char
	wrk=" " item " "
	idx=index(commands,wrk)
	if ( idx>0 ) {
		cmds=cmds item
		valid="y"
		return
	}
	# everything else is wrong...
	msg="Invalid input " item ", please give a valid one"
	valid="n"
}

function chk_final_letters() {
	#
	# just insert a comma after the last digit...
	#
	# if the last char is a number, nothing to do
	ch=substr(wrk,length(wrk),1)
	idx=index(numbers,ch)
	if ( idx>0 ) { return }
	# otherwise, check...
	final=""
	for ( l=length(wrk); l>=1; l-- ) {
		ch=substr(wrk,l,1)
		idx=index(letters,ch)
		if ( idx>0 ) {
			final=ch final
			continue
		}
		# if there is already a comma
		if ( substr(wrk,l,1)=="," ) { return }
		# insert a comma
		wrk=substr(wrk,1,l) "," final
		return
	}
}
