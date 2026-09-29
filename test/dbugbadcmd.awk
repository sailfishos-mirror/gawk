BEGIN { arr["k"]="v"; arr["sub"]["z"]="q"; x=1 }
{ arr[$1]=$2; x=$1; print x }
                                                 