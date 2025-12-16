/* Replacement for the missing nl_langinfo.  Only CODESET is currently
   supported.  */
#include <langinfo.h>

char *
nl_langinfo (int item)
{
  switch (item)
    {
      case CODESET:
	{
	  /* Shamelessly stolen from Gnulib's nl_langinfo.c.  */
	  static char buf[2 + 10 + 1];
	  char const *locale = setlocale (LC_CTYPE, NULL);
	  char *codeset = buf;
	  size_t codesetlen;
	  codeset[0] = '\0';

	  if (locale && locale[0])
	    {
	      /* If the locale name contains an encoding after the
		 dot, return it.  */
	      char *dot = strchr (locale, '.');

	      if (dot)
		{
		  /* Look for the possible @... trailer and remove it,
		     if any.  */
		  char *codeset_start = dot + 1;
		  char const *modifier = strchr (codeset_start, '@');

		  if (! modifier)
		    codeset = codeset_start;
		  else
		    {
		      codesetlen = modifier - codeset_start;
		      if (codesetlen < sizeof buf)
			{
			  codeset = memcpy (buf, codeset_start, codesetlen);
			  codeset[codesetlen] = '\0';
			}
		    }
		}
	    }
	  /* If setlocale is successful, it returns the number of the
	     codepage, as a string.  Otherwise, fall back on Windows
	     API GetACP, which returns the locale's codepage as a
	     number (although this doesn't change according to what
	     the 'setlocale' call specified).  Either way, prepend
	     "CP" to make it a valid codeset name.  */
	  codesetlen = strlen (codeset);
	  if (0 < codesetlen && codesetlen < sizeof buf - 2)
	    memmove (buf + 2, codeset, codesetlen + 1);
	  else
	    sprintf (buf + 2, "%u", GetACP ());
	  codeset = memcpy (buf, "CP", 2);

	  return codeset;
	}
      default:
	return (char *) "";
    }
}
