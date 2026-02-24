/*
 * locale.c - Provide setlocale function and variables for gawk.
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
#include <string.h>
#include <assert.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gawkapi.h"

#include "gettext.h"
#define _(msgid)  gettext(msgid)
#define N_(msgid) msgid

static awk_bool_t locale_init(void);

static const gawk_api_t *api;	/* for convenience macros to work */
static awk_ext_id_t ext_id;
static const char *ext_version = "locale extension: version 1.0";
static awk_bool_t (*init_func)(void) = locale_init;

int plugin_is_GPL_compatible;

/* locale_init --- initialize the module */

static awk_bool_t
locale_init(void)
{
	static struct {
		const char *name;
		int value;
	} valtab[] = {
		{ "LC_ALL", LC_ALL },
		{ "LC_COLLATE", LC_COLLATE },
		{ "LC_CTYPE", LC_CTYPE },
		{ "LC_MONETARY", LC_MONETARY },
		{ "LC_NUMERIC", LC_NUMERIC },
		{ "LC_TIME", LC_TIME },
#ifdef LC_MESSAGES
		{ "LC_MESSAGES", LC_MESSAGES },
#else
		{ "LC_MESSAGES", -1 },
#endif
#ifdef LC_ADDRESS
		{ "LC_ADDRESS", LC_ADDRESS },
#else
		{ "LC_ADDRESS", -1 },
#endif
#ifdef LC_IDENTIFICATION
		{ "LC_IDENTIFICATION", LC_IDENTIFICATION },
#else
		{ "LC_IDENTIFICATION", -1 },
#endif
#ifdef LC_MEASUREMENT
		{ "LC_MEASUREMENT", LC_MEASUREMENT },
#else
		{ "LC_MEASUREMENT", -1 },
#endif
#ifdef LC_NAME
		{ "LC_NAME", LC_NAME },
#else
		{ "LC_NAME", -1 },
#endif
#ifdef LC_PAPER
		{ "LC_PAPER", LC_PAPER },
#else
		{ "LC_PAPER", -1 },
#endif
#ifdef LC_TELEPHONE
		{ "LC_TELEPHONE", LC_TELEPHONE },
#else
		{ "LC_TELEPHONE", -1 },
#endif
		{ NULL, 0 },
	};

	awk_value_t new_val;
	for (int i = 0; valtab[i].name != NULL; i++) {
		make_number(valtab[i].value, & new_val);
		if (! sym_update(valtab[i].name, & new_val)) {
			warning(ext_id, _("locale: failed to install %s"),
					valtab[i].name);
			return awk_false;
		}
	}

	make_number(MB_CUR_MAX, & new_val);
	if (! sym_update("MB_CUR_MAX", & new_val)) {
		warning(ext_id, _("locale: failed to install MB_CUR_MAX"));
		return awk_false;
	}

	return awk_true;
}

/* do_setlocale --- set the locale and update MB_CUR_MAX */

static awk_value_t *
do_setlocale(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
	assert(result != NULL);

	awk_value_t category, new_locale;
	if (! get_argument(0, AWK_NUMBER, & category)) {
		warning(ext_id, _("setlocale: could not get category argument"));
		make_const_string("", 0, result);
		goto out;
	}
	if (! get_argument(1, AWK_STRING, & new_locale)) {
		warning(ext_id, _("setlocale: could not get locale argument"));
		make_const_string("", 0, result);
		goto out;
	}

	const char *old_locale;

	// value can be "" or a locale name
	old_locale = setlocale(category.num_value, new_locale.str_value.str);

	awk_value_t mb_cur_max;
	make_number(MB_CUR_MAX, & mb_cur_max);
	if (! sym_update("MB_CUR_MAX", & mb_cur_max))
		return awk_false;

	make_const_string(old_locale, strlen(old_locale), result);

out:
	return result;
}


/* do_getlocale --- quety the locale */

static awk_value_t *
do_getlocale(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
	assert(result != NULL);

	awk_value_t category;
	if (! get_argument(0, AWK_NUMBER, & category)) {
		warning(ext_id, _("getlocale: could not get category argument"));
		make_const_string("", 0, result);
		goto out;
	}

	const char *old_locale;

	// value can be "" or a locale name
	old_locale = setlocale(category.num_value, NULL);

	make_const_string(old_locale, strlen(old_locale), result);

out:
	return result;
}

static awk_ext_func_t func_table[] = {
	{ "setlocale", do_setlocale, 2, 2, awk_false, NULL },
	{ "getlocale", do_getlocale, 1, 1, awk_false, NULL },
};

/* define the dl_load function using the boilerplate macro */

dl_load_func(func_table, locale, "")
