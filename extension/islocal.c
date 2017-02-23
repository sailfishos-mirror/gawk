/*
 * islocal.c - Return true if a parameter came from a local variable.
 */

/*
 * Copyright (C) 2017 the Free Software Foundation, Inc.
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "gawkapi.h"

#include "gettext.h"
#define _(msgid)  gettext(msgid)
#define N_(msgid) msgid

static const gawk_api_t *api;	/* for convenience macros to work */
static awk_ext_id_t *ext_id;
static const char *ext_version = "islocal extension: version 1.0";
static awk_bool_t (*init_func)(void) = NULL;

int plugin_is_GPL_compatible;

/*  do_islocal --- check if a variable is local or not */

static awk_value_t *
do_islocal(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
	awk_value_t param;
	int ret = -1;

	assert(result != NULL);

	if (get_argument(0, AWK_UNDEFINED, &param)) {
		ret = param.is_local;
	}

	/* Set the return value */
	return make_number(ret, result);
}

static awk_ext_func_t func_table[] = {
	{ "islocal", do_islocal, 1, 1, awk_false, NULL },
};

/* define the dl_load function using the boilerplate macro */

dl_load_func(func_table, islocal, "")
