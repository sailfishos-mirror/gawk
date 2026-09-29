BEGIN { fname = "gensub"; print @fname(/(a)(b?)/, "\\1&", "g", "abab") }
