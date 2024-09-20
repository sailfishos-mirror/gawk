BEGIN {
    FPAT = "([^,]*)|(\"([^\"]|\"\")+\")"
}

{
    for (i = 1; i <= NF; i++) {
        # Extract data from double-quoted fields
        if (substr($i, 1, 1) == "\"") {
            gsub(/^"|"$/, "", $i)    # Remove enclosing quotes
            gsub(/""/, "\"", $i)    # Convert "" to "
        }
        # Print tab-separated values
        printf (i < NF) ? "%s\t" : "%s\n", $i
    }
}
