/*
 * ctype.c - Provide ctype functions for gawk.
 *
 * February 2026.
 */

/*
 * Copyright (C) 2026
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
#include <stdlib.h>
#include <assert.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gawkapi.h"

#include "gettext.h"
#define _(msgid)  gettext(msgid)
#define N_(msgid) msgid

static awk_bool_t ctype_init(void);

static const gawk_api_t *api;	/* for convenience macros to work */
static awk_ext_id_t ext_id;
static const char *ext_version = "ctype extension: version 1.0";
static awk_bool_t (*init_func)(void) = ctype_init;

int plugin_is_GPL_compatible;

static int mb_cur_max = 0;

/* ctype_init --- initialize the module */

static awk_bool_t
ctype_init(void)
{
	mb_cur_max = MB_CUR_MAX;

	return awk_true;
}

/*  do_XXX --- provide isXXX() ctype function for gawk */

#define DEF_FUNC(sbfunc, wcfunc) \
static awk_value_t * \
do_ ## sbfunc(int nargs, awk_value_t *result, struct awk_ext_func *unused) \
{ \
	int ret = 0; \
 \
	assert(result != NULL); \
 \
	awk_value_t the_char; \
 \
	if (! get_argument(0, AWK_NUMBER, & the_char)) { \
		warning(ext_id, _(# sbfunc ": could not get argument")); \
		goto out; \
	} \
 \
	int char_val = the_char.num_value; \
 \
	if (mb_cur_max == 1) { \
		char_val &= 0xff; \
		ret = sbfunc(char_val); \
	} else \
		ret = wcfunc(char_val); \
 \
	ret = !! ret;	/* force to 0 or 1 */ \
out: \
	return make_number(ret, result); \
}

DEF_FUNC(isalnum, iswalnum)
DEF_FUNC(isalpha, iswalpha)
DEF_FUNC(isblank, iswblank)
DEF_FUNC(iscntrl, iswcntrl)
DEF_FUNC(isdigit, iswdigit)
DEF_FUNC(isgraph, iswgraph)
DEF_FUNC(islower, iswlower)
DEF_FUNC(isprint, iswprint)
DEF_FUNC(ispunct, iswpunct)
DEF_FUNC(isspace, iswspace)
DEF_FUNC(isupper, iswupper)
DEF_FUNC(isxdigit, iswxdigit)

static awk_ext_func_t func_table[] = {
	{ "isalnum", do_isalnum, 0, 0, awk_false, NULL },
	{ "isalpha", do_isalpha, 0, 0, awk_false, NULL },
	{ "isblank", do_isblank, 0, 0, awk_false, NULL },
	{ "iscntrl", do_iscntrl, 0, 0, awk_false, NULL },
	{ "isdigit", do_isdigit, 0, 0, awk_false, NULL },
	{ "isgraph", do_isgraph, 0, 0, awk_false, NULL },
	{ "islower", do_islower, 0, 0, awk_false, NULL },
	{ "ispunct", do_ispunct, 0, 0, awk_false, NULL },
	{ "isprint", do_isprint, 0, 0, awk_false, NULL },
	{ "isspace", do_isspace, 0, 0, awk_false, NULL },
	{ "isupper", do_isupper, 0, 0, awk_false, NULL },
	{ "isxdigit", do_isxdigit, 0, 0, awk_false, NULL },
};

/* define the dl_load function using the boilerplate macro */

dl_load_func(func_table, ctype, "")
