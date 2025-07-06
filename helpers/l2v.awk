#! /usr/bin/gawk -f

# l2v.awk --- convert logical ordering for right-to-left text to visual.
#
# This program is intended for use with Hebrew Unicode text,
# in particular for processing the po/he.po file in the gawk distribution.
# It isn't necessarily going to work on any other right-to-left file.
#
# Arnold Robbins
# arnold@skeeve.com
# June-July 2025

# There are lots of global variables here. Since it's a small script, that's
# OK, but things might be cleaner if functions could do pass by reference.

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

# reset_output --- reset the global variables for each line

function reset_output()
{
	Direction = "L2R"	# initial direction, left to right
	Left = ""		# left side of output
	Right = ""		# right side of output
	Format_count = 1	# for use in %<n>$xyz<format>
	delete Chars		# Input line after splitting
	Len = 0			# Length of input line
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

# parse_and_build_format --- parse a printf format and revise it as needed

function parse_and_build_format(start, lastpos,		i, resultstpos)
{
	# input is something like  %...s or `%...s'

	if (Chars[start] == "`") {
		result = "`%" Format_count++ "$"
		start += 2
		for (i = start; Chars[i] != "'"; i++) {
			result = result Chars[i]
		}
		result = result "'"
	} else {
		result = "%" Format_count++ "$"
		for (i = ++start; ! (Chars[i] in Conversions); i++) {
			result = result Chars[i]
		}
		result = result Chars[i]
	}

	lastpos[1] = i

	return result
}

# insert_text --- add new text to the correct part of the result

function insert_text(new_text)
{
	if (Direction == "R2L")
		Right = new_text Right
	else
		Left = Left new_text
}

# build_output --- build the output line

function build_output(		i, new_format, lastpos)
{
	for (i = 1; i <= Len; i++) {
		delete lastpos
		if (Chars[i] == "`" && Chars[i+1] == "%") {
			new_format = parse_and_build_format(i, lastpos)
			i = lastpos[1]
			insert_text(new_format)
			continue
		} else if (Chars[i] == "%") {
			if (Chars[i+1] == " " || Chars[i+1] == "%") {	# pass through
				insert_text(Chars[i] Chars[i+1])
				i++
			} else {
				new_format = parse_and_build_format(i, lastpos)
				i = lastpos[1]
				insert_text(new_format)
			}
			continue
		} else if (is_hebrew(Chars[i]) || ord(Chars[i]) > 127) {
			Direction = "R2L"
			insert_text(Chars[i])
		} else if (is_space(Chars[i]) || is_punct(Chars[i])) {
			insert_text(Chars[i])
		} else {
			Direction = "L2R"
			insert_text(Chars[i])
		}
	}
}

# for each line
/(^#)|(^$)/ || /msgid/	{ Processing = FALSE ; print ; next }

/msgstr/ { Processing = TRUE }

# default
! Processing { print ; next }

Processing {
	reset_output()

	if ($1 ~ /msgstr/) {
		key = $1
		sub("^" key, "")
		sub(/^[[:space:]]+/, "")
	} else
		key = ""

	Len = length($0)
	if (substr($0, 1, 1) == "\"")
		$0 = substr($0, 2, Len-2)

	Len = split($0, Chars, "")	# get indvidual characters

	build_output()

	if (key)
		printf("%s ", key)
	printf("\"%s%s\"\n", Left, Right)
	next
}
