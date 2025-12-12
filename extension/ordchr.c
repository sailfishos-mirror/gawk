/*
 * ordchr.c - Builtin functions that provide ord() and chr() functions.
 *
 * Arnold Robbins
 * arnold@skeeve.com
 * 8/2001
 * Revised 6/2004
 * Revised 5/2012
 * Revised 6/2025
 */

/*
 * Copyright (C) 2001, 2004, 2011, 2012, 2013, 2018, 2020, 2021, 2025,
 * the Free Software Foundation, Inc.
 *
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 *
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <langinfo.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "gawkapi.h"

#include "gettext.h"
#define _(msgid)  gettext(msgid)
#define N_(msgid) msgid

static const gawk_api_t *api;	/* for convenience macros to work */
static awk_ext_id_t ext_id;
static const char *ext_version = "ordchr extension: version 2.0";
static awk_bool_t (*init_func)(void) = NULL;

int plugin_is_GPL_compatible;

#ifdef __MINGW32__
/* Replacement for the missing nl_langinfo.  Only CODESET is currently
   supported.  */
#include <locale.h>
#include <windows.h>

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
#endif

/*  do_ord --- return numeric value of first char of string */

static awk_value_t *
do_ord(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
	awk_value_t str;
	double ret = 0xFFFD;	// unicode bad char
	mbstate_t mbs;
	const char *src;

	assert(result != NULL);

	memset(& mbs, 0, sizeof(mbs));

	if (get_argument(0, AWK_STRING, & str)) {
		if (MB_CUR_MAX == 1) {
			ret = str.str_value.str[0] & 0xff;
		} else {
			wchar_t wc[2];
			size_t res;

			src = str.str_value.str;
			res = mbsrtowcs(wc, & src, 1, & mbs);
			if (res == 0 || res == (size_t) -1 || res == (size_t) -2) {
				// mimic gawk's behavior
				char *codeset = nl_langinfo(CODESET);
				if (strcmp(codeset, "UTF-8") == 0
				    /* MS-Windows' UTF-8 codepage */
				    || strcmp(codeset, "CP65001") == 0)
					ret = 0xFFFD;	// unicode bad char
				else
					ret = src[0] & 0xFF; 
			} else
				ret = wc[0];
		}
	} else if (do_lint)
		lintwarn(ext_id, _("ord: first argument is not a string"));

	/* Set the return value */
	return make_number(ret, result);
}

/*  do_chr --- turn numeric value into a string */

static awk_value_t *
do_chr(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
	awk_value_t num;
	unsigned int ret = 0;
	double val = 0.0;
	wchar_t str[2];
	char buf[20] = { '\0', '\0' };

	str[0] = str[1] = L'\0';

	assert(result != NULL);
;

	str[0] = str[1] = L'\0';

	assert(result != NULL);

	if (get_argument(0, AWK_NUMBER, & num)) {
		val = num.num_value;
		ret = val;	/* convert to int */
		if (MB_CUR_MAX == 1) {
			buf[0] = ret & 0xff;
			goto done;
		} else {
			str[0] = ret;
		}
	} else if (do_lint)
		lintwarn(ext_id, _("chr: first argument is not a number"));

	if (get_argument(0, AWK_NUMBER, & num)) {
		val = num.num_value;
		ret = val;	/* convert to int */
		if (MB_CUR_MAX == 1) {
			buf[0] = ret & 0xff;
			goto done;
		} else {
			str[0] = ret;
		}
	} else if (do_lint)
		lintwarn(ext_id, _("chr: first argument is not a number"));

	mbstate_t mbs;
	size_t res;
	const wchar_t *src = str;

	memset(& mbs, 0, sizeof(mbs));
	res = wcsrtombs(buf, & src, sizeof(buf)-1, & mbs);
	if (res == 0 || res == (size_t)-1 || res == (size_t) -2)
		buf[0] = buf[1] = '\0';

done:
	/* Set the return value */
	return make_const_string(buf, strlen(buf), result);
}

static awk_ext_func_t func_table[] = {
	{ "ord", do_ord, 1, 1, awk_false, NULL },
	{ "chr", do_chr, 1, 1, awk_false, NULL },
};

/* define the dl_load function using the boilerplate macro */

dl_load_func(func_table, ord_chr, "")
