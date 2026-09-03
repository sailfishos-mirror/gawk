BEGIN {
	target[1] = "abc";	regex[1] = "b+";	replace[1] = "FOO";	how[1] = "g"
	target[2] = "abc";	regex[2] = "q";		replace[2] = "FOO";	how[2] = "g"
	target[3] = "abc";	regex[3] = "b";		replace[3] = "X";	how[3] = 2
	target[4] = "abc";	regex[4] = "x*";	replace[4] = "X";	how[4] = "g"
	target[5] = "abc";	regex[5] = "b*";	replace[5] = "X";	how[5] = "g"
	target[6] = "abc";	regex[6] = "c";		replace[6] = "X";	how[6] = "g"
	target[7] = "abc";	regex[7] = "c+";	replace[7] = "X";	how[7] = "g"
	target[8] = "abc";	regex[8] = "x*$";	replace[8] = "X";	how[8] = "g"
	target[9] = "abc";	regex[9] = "(a)(b)(c)";	replace[9] = "& - \\3\\2\\1 - &";	how[9] = "g"

	j = length(target)
	for (i = 1; i <= j; i++) {
		printf("gensub(/%s/, \"%s\", %s, \"%s\") ---> \"%s\", nsub = %d\n",
		       regex[i], replace[i], how[i], target[i],
		       gensub(regex[i], replace[i], how[i], target[i]),
		       PROCINFO["nsub"])
	}

	target[10] = "abbbc"
	$0 = target[10]
	regex[10] = "(a)(b)(c)"
	replace[10] = "& - \\3\\2\\10 - &"
	how[10] = "g"
	printf("gensub(/%s/, \"%s\", %s, \"%s\") ---> \"%s\", nsub = %d\n",
	       regex[10], replace[10], how[10], target[10],
	       gensub(regex[10], replace[10], how[10]),		# $0, not target
		       PROCINFO["nsub"])
}
