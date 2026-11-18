#! /usr/bin/gawk -f

# gensub2 --- Do what gensub() does, but also set result["nsub"] to the
# number of substitutions made, and result["nmatch"] to the number of
# matches.  This is useful for prototyping and exploration.
#
# The implementation relies on a number of features specific to gawk.

# For production, the print statements and assignments to `target'
# should be commented out.

@namespace "gensub2"

BEGIN {
    target = "abc"
    runtest("b+", "FOO", "g", target)
    runtest("q", "FOO", "g", target)
    runtest("b", "X", 2, target)
    runtest("x*", "X", "g", target)
    runtest("b*", "X", "g", target)
    runtest("c", "X", "g", target)
    runtest("c+", "X", "g", target)
    runtest("x*$", "X", "g", target)

    $0 = target
    runtest("(a)(b)(c)", "& - \\3\\2\\1 - &", "g")  # no target

    target = "abbbc"
    runtest("b", "X", 2, target)
}

function runtest(regex, replace, how, target,   result, stats)
{
    if (awk::typeof(target) == "untyped")
        target = $0
            
    result = awk::gensub2(regex, replace, how, target, stats)
    printf("%s ---> %s, nmatch = %d, nsub = %d\n", target, result, stats["nmatch"], stats["nsub"])
}

function compute_replace(replace, matches,  result, chars, i, j, k)
{
    j = split(replace, chars, "")
    for (i = 1; i <= j; i++) {
        if (chars[i] == "\\") {
            if (index("0123456789", chars[i+1]) != 0) {
                k = chars[i+1] + 0
                result = result matches[k]
            } else {
            result = result chars[i+1]  # literal text
        }
        i++
        } else if (chars[i] == "&") {
            result = result matches[0]  # entire text
        } else {
            result = result chars[i]    # literal text
        }
    }
    return result
}

@namespace "awk"

function gensub2(regex, replace, how, target, stats,       # parameters
                 newstr, eosflag, nonzeroflag, global,  # locals
                 which, count, str, matches, repl, nsub,
                 true, false)
{
    true = 1; false = 0
    nsub = 0

    if (typeof(target) == "untyped")
        target = $0

    delete stats

    global = (how == "g" || how == "G")
    which = 1
    if (! global)
        which = how + 0
    if (which <= 0)
        which = 1

    eosflag = nonzeroflag = false
    str = target
    while (match(str, regex, matches)) {
        count++
        if (! global && count != which) {
            newstr = newstr substr(str, 1, RSTART + RLENGTH - 1)
            str = substr(str, RSTART + RLENGTH)
            continue
        }

        repl = gensub2::compute_replace(replace, matches)

        if (RLENGTH > 0) { # easy case
            nonzeroflag = true
            if (RSTART == 1) { # match at front of string
                newstr = newstr repl
            } else {
                newstr = newstr substr(str, 1, RSTART-1) repl
            }
            str = substr(str, RSTART+RLENGTH)
            nsub++
        } else if (nonzeroflag) {
            # last match was non-zero in length, and at the
            # current character, we get a zero length match,
            # which we don't really want, so skip over it
            newstr = newstr substr(str, 1, 1)
            str = substr(str, 2)
            nonzeroflag = false
        } else {
            # zero-length match
            nsub++
            if (RSTART == 1) {
                newstr = newstr repl substr(str, 1, 1)
                str = substr(str, 2)
            } else {
                return newstr str repl
            }
        }
        if (length(str) == 0) {
            if (eosflag)
                break
            else
                eosflag = true
        }
    }
    if (length(str) > 0)
        newstr = newstr str # rest of string

    stats["nmatch"] = count
    stats["nsub"] = nsub

    return newstr
}
