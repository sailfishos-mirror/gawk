#! /usr/bin/gawk -f

# l2p.awk --- convert logical ordering for right-to-left text to physical
#
# This program is intended for use with Hebrew Unicode text,
# in particular for processing the po/he.po file in the gawk distribution.
# It isn't necessarily going to work on any other right-to-left file.
#
# Arnold Robbins
# arnold@skeeve.com
# June 2025

@load "ordchr"

BEGIN {
	TRUE = 1
	FALSE = 0

	hl = "אבגדהוזחטיכךלמםנןסעפףצץקרשת"
	split(hl, hl2, "")
	for (i in hl2)
		Hebrew[hl2[i]] = 1	# letters are now indices

	c = "aAcdieEfFgGsouxX%"
	split(c, c2, "")
	for (i in c2)
		Conversions[c2[i]] = 1	# conversion chars are now indices

	reset_output()
	Processing = FALSE
}

# is_hebrew --- return true if a character is Hebrew

function is_hebrew(char)
{
	return char in Hebrew
}

# is_conversion --- return true if a character is a printf conversion specifier

function is_conversion(char)
{
	return char in Conversions
}

# reset_output --- reset the indices for each line and the output array

function reset_output()
{
	Direction = "L2R"	# initial direction, left to right
	Left = ""		# left side of output
	Right = ""		# right side of output
}

# is_space --- check if a character is white space

function is_space(char)
{
	return char ~ /[[:space:]]/
}

# is_punct --- check if a character is punctuation

function is_punct(char)
{
	return char ~ /[[:punct:]]/
}

# build_output --- build the output line

function build_output(chars, len,	i)
{
	for (i = 1; i <= len; i++) {
		# if (chars[i] == "%") {
		# } else
		if (is_hebrew(chars[i]) || ord(chars[i]) > 127) {
			Direction = "R2L"
			Right = chars[i] Right
		} else if (is_space(chars[i]) || is_punct(chars[i])) {
			if (Direction == "R2L")
				Right = chars[i] Right
			else
				Left = Left chars[i]
		} else {
			Direction = "L2R"
			Left = Left chars[i]
		}
	}
}

# for each line
/(^#)|(^$)/ || /msgid/	{ Processing = FALSE ; print ; next }

/msgstr/ { Processing = TRUE }

# default
Processing == FALSE { print ; next }

Processing == TRUE {
	reset_output()

	if ($1 ~ /msgid|msgstr/) {
		key = $1
		sub("^" key, "")
		sub(/^[[:space:]]+/, "")
	} else
		key = ""

	len = length($0)
	if (substr($0, 1, 1) == "\"")
		$0 = substr($0, 2, len-2)

	len = split($0, chars, "")	# get indvidual characters

	build_output(chars, len)

	if (key)
		printf("%s ", key)
	printf("\"%s%s\"\n", Left, Right)
	next
}
