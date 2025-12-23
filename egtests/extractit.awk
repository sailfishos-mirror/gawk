# extract.awk --- estrae file ed esegue programmi dal file Texinfo
#
# This file and how to use it are described in the manual.
# Therefore, we respectfully advise you to Read The Fine Manual
# for more information.
#
# Arnold Robbins, arnold@skeeve.com, Public Domain
# May 1993
# Revised September 2000
# Antonio Colombo, October 2020, test for Italian accented letters

BEGIN    { IGNORECASE = 1 }

/^@c(omment)?[ \t]+system/ {
    if (NF < 3) {
        e = ("extract: " FILENAME ":" FNR)
        e = (e  ": riga `system' con formato errato")
        print e > "/dev/stderr"
        next
    }
    $1 = ""
    $2 = ""
    stat = system($0)
    if (stat != 0) {
        e = ("extract: " FILENAME ":" FNR)
        e = (e ": attenzione: system ha restituito " stat)
        print e > "/dev/stderr"
    }
}
/^@c(omment)?[ \t]+file/ {
    if (NF != 3) {
        e = ("extract: " FILENAME ":" FNR ": riga `file' con formato errato")
        print e > "/dev/stderr"
        next
    }
    if ($3 != file_corrente) {
        if (file_corrente != "")
            lista_file[file_corrente] = 1   # memorizza per chiudere dopo
        file_corrente = $3
    }

    for (;;) {
        if ((getline riga) <= 0)
            fine_file_inattesa()
        if (riga ~ /^@c(omment)?[ \t]+endfile/)
            break
        else if (riga ~ /^@(end[ \t]+)?group/)
            continue
        else if (riga ~ /^@c(omment+)?[ \t]+/)
            continue
        if (index(riga, "@") == 0) {
            print riga > file_corrente
            continue
        }
        # gestisci istruzioni che convertono caratteri accentati
        if (index(riga, "gsub(\"@@") > 0) {
            gsub("@{","{",riga)
            gsub("@}","}",riga)
            gsub("@@","@",riga)
            print riga > file_corrente
            continue
        }
        # istruzioni che convertono caratteri accentati
        gsub("@`a","à",riga)
        gsub("@`e","è",riga)
        gsub("@'e","é",riga)
        gsub("@`{@dotless{i}}","ì",riga)
        gsub("@`o","ò",riga)
        gsub("@`u","ù",riga)
        # riga contiene ancora caratteri @?
        if (index(riga, "@") == 0) {
            print riga > file_corrente
            continue
        }
        n = split(riga, a, "@")
        # if a[1] == "", vuol dire riga che inizia per @,
        # non salvare un @
        for (i = 2; i <= n; i++) {
            if (a[i] == "") { # era un @@
                a[i] = "@"
                if (a[i+1] == "")
                    i++
            }
        }
        print join(a, 1, n, SUBSEP) > file_corrente
    }
}
END {
    close(file_corrente)    # chiudi l'ultimo file
    for (f in lista_file)   # chiudi tutti gli altri
        close(f)
}
function fine_file_inattesa()
{
    printf("extract: %s:%d: fine-file inattesa, o errore\n",
                     FILENAME, FNR) > "/dev/stderr"
    exit 1
}
# join.awk --- trasforma un vettore in una stringa
#
# This file and how to use it are described in the manual.
# Therefore, we respectfully advise you to Read The Fine Manual
# for more information.
#
# Arnold Robbins, arnold@skeeve.com, Public Domain
# May 1993

function join(vettore, iniz, fine, separ,    risultato, i)
{
    if (separ == "")
       separ = " "
    else if (separ == SUBSEP) # valore magico
       separ = ""
    risultato = vettore[iniz]
    for (i = iniz + 1; i <= fine; i++)
        risultato = risultato separ vettore[i]
    return risultato
}
