/*
 * awkmem.h - memory management routines for gawk
 */

/*
 * Copyright (C) 2011-2015, 2017-2020, 2022, 2023, 2025, 2026
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// This file should be included by awk.h

#ifdef USE_REAL_MALLOC
#define	emalloc(var,ty,x)	(void) (var = (ty) malloc((size_t)(x)))
#define	ezalloc(var,ty,x)	(void) (var = (ty) calloc((size_t)(x), 1))
#define	erealloc(var,ty,x)	(void) (var = (ty) realloc((void *) var, (size_t)(x)))
#else
#define	emalloc(var,ty,x)	(void) (var = (ty) emalloc_real((size_t)(x), __func__, #var, __FILE__, __LINE__))
#define	ezalloc(var,ty,x)	(void) (var = (ty) ezalloc_real((size_t)(x), __func__, #var, __FILE__, __LINE__))
#define	erealloc(var,ty,x)	(void) (var = (ty) erealloc_real((void *) var, (size_t)(x), __func__, #var, __FILE__, __LINE__))
#endif

/* emalloc_real --- malloc with error checking */

static inline void *
emalloc_real(size_t count, const char *where, const char *var, const char *file, int line)
{
	void *ret;

	if (count == 0)
		fatal("%s:%d: emalloc called with zero bytes", file, line);

	ret = (void *) malloc(count);
	if (ret == NULL)
		fatal(_("%s:%d:%s: %s: cannot allocate %zu bytes of memory: %s"),
			file, line, where, var, count, strerror(errno));

	return ret;
}

/* ezalloc_real --- malloc zero-filled bytes with error checking */

static inline void *
ezalloc_real(size_t count, const char *where, const char *var, const char *file, int line)
{
	void *ret;

	if (count == 0)
		fatal("%s:%d: ezalloc called with zero bytes", file, line);

	ret = (void *) calloc(1, count);
	if (ret == NULL)
		fatal(_("%s:%d:%s: %s: cannot allocate %zu bytes of memory: %s"),
			file, line, where, var, count, strerror(errno));

	return ret;
}

/* erealloc_real --- realloc with error checking */

static inline void *
erealloc_real(void *ptr, size_t count, const char *where, const char *var, const char *file, int line)
{
	void *ret;

	if (count == 0)
		fatal("%s:%d: erealloc called with zero bytes", file, line);

	ret = (void *) realloc(ptr, count);
	if (ret == NULL)
		fatal(_("%s:%d:%s: %s: cannot reallocate %zu bytes of memory: %s"),
			file, line, where, var, count, strerror(errno));

	return ret;
}
