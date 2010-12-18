/*
 * array.c - routines for associative arrays.
 */

/* 
 * Copyright (C) 1986, 1988, 1989, 1991-2010 the Free Software Foundation, Inc.
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

/*
 * Tree walks (``for (iggy in foo)'') and array deletions use expensive
 * linear searching.  So what we do is start out with small arrays and
 * grow them as needed, so that our arrays are hopefully small enough,
 * most of the time, that they're pretty full and we're not looking at
 * wasted space.
 *
 * The decision is made to grow the array if the average chain length is
 * ``too big''. This is defined as the total number of entries in the table
 * divided by the size of the array being greater than some constant.
 *
 * 11/2002: We make the constant a variable, so that it can be tweaked
 * via environment variable.
 */

static int AVG_CHAIN_MAX = 2;	/* 11/2002: Modern machines are bigger, cut this down from 10. */

#include "awk.h"

static size_t SUBSEPlen;
static char *SUBSEP;

static NODE *assoc_find(NODE *symbol, NODE *subs, unsigned long hash1);
static void grow_table(NODE *symbol);

static unsigned long gst_hash_string(const char *str, size_t len, unsigned long hsize, size_t *code);
static unsigned long scramble(unsigned long x);
static unsigned long awk_hash(const char *s, size_t len, unsigned long hsize, size_t *code);

unsigned long (*hash)(const char *s, size_t len, unsigned long hsize, size_t *code) = awk_hash;

/* array_init --- possibly temporary function for experimentation purposes */

void
array_init()
{
	const char *val;
	int newval;

	if ((val = getenv("AVG_CHAIN_MAX")) != NULL && isdigit(*val)) {
		for (newval = 0; *val && isdigit(*val); val++)
			newval = (newval * 10) + *val - '0';

		AVG_CHAIN_MAX = newval;
	}

	if ((val = getenv("AWK_HASH")) != NULL && strcmp(val, "gst") == 0)
		hash = gst_hash_string; 
}

/*
 * array_vname --- print the name of the array
 *
 * Returns a pointer to a statically maintained dynamically allocated string.
 * It's appropriate for printing the name once; if the caller wants
 * to save it, they have to make a copy.
 *
 * Setting MAX_LEN to a positive value (eg. 140) would limit the length
 * of the output to _roughly_ that length.
 *
 * If MAX_LEN == 0, which is the default, the whole stack is printed.
 */
#define	MAX_LEN 0

char *
array_vname(const NODE *symbol)
{
	static char *message = NULL;
	
	if (symbol->type != Node_array_ref || symbol->orig_array->type != Node_var_array)
		return symbol->vname;
	else {
		static size_t msglen = 0;
		char *s;
		size_t len;
		int n;
		const NODE *save_symbol = symbol;
		const char *from = _("from %s");

#if (MAX_LEN <= 0) || !defined(HAVE_SNPRINTF)
		/* This is the default branch. */

		/* First, we have to compute the length of the string: */
		len = strlen(symbol->vname) + 2;	/* "%s (" */
		n = 0;
		do {
			symbol = symbol->prev_array;
			len += strlen(symbol->vname);
			n++;
		} while	(symbol->type == Node_array_ref);
		/*
		 * Each node contributes by strlen(from) minus the length
		 * of "%s" in the translation (which is at least 2)
		 * plus 2 for ", " or ")\0"; this adds up to strlen(from).
		 */
		len += n * strlen(from);

		/* (Re)allocate memory: */
		if (message == NULL) {
			emalloc(message, char *, len, "array_vname");
			msglen = len;
		} else if (len > msglen) {
			erealloc(message, char *, len, "array_vname");
			msglen = len;
		} /* else
			current buffer can hold new name */

		/* We're ready to print: */
		symbol = save_symbol;
		s = message;
		/*
		 * Ancient systems have sprintf() returning char *, not int.
		 * Thus, `s += sprintf(s, from, name);' is a no-no.
		 */
		sprintf(s, "%s (", symbol->vname);
		s += strlen(s);
		for (;;) {
			symbol = symbol->prev_array;
			sprintf(s, from, symbol->vname);
			s += strlen(s);
			if (symbol->type != Node_array_ref)
				break;
			sprintf(s, ", ");
			s += strlen(s);
		}
		sprintf(s, ")");

#else /* MAX_LEN > 0 */

		/*
		 * The following check fails only on
		 * abnormally_long_variable_name.
		 */
#define PRINT_CHECK \
		if (n <= 0 || n >= len) \
			return save_symbol->vname; \
		s += n; len -= n
#define PRINT(str) \
		n = snprintf(s, len, str); \
		PRINT_CHECK
#define PRINT_vname(str) \
		n = snprintf(s, len, str, symbol->vname); \
		PRINT_CHECK

		if (message == NULL)
			emalloc(message, char *, MAX_LEN, "array_vname");

		s = message;
		len = MAX_LEN;

		/* First, print the vname of the node. */
		PRINT_vname("%s (");

		for (;;) {
			symbol = symbol->prev_array;
			/*
			 * When we don't have enough space and this is not
			 * the last node, shorten the list.
			 */
			if (len < 40 && symbol->type == Node_array_ref) {
				PRINT("..., ");
				symbol = symbol->orig_array;
			}
			PRINT_vname(from);
			if (symbol->type != Node_array_ref)
				break;
			PRINT(", ");
		}
		PRINT(")");

#undef PRINT_CHECK
#undef PRINT
#undef PRINT_vname
#endif /* MAX_LEN <= 0 */

		return message;
	}
}
#undef MAX_LEN

/* make_aname --- construct a sub-array name for multi-dimensional array */

char *
make_aname(NODE *array, NODE *subs)
{
	static char *aname = NULL;
	static size_t aname_len;
	size_t slen;

	slen = strlen(array->vname) + subs->stlen + 6;
	if (aname == NULL) {
		emalloc(aname, char *, slen, "make_aname");
		aname_len = slen;
	} else if (slen > aname_len) {
		erealloc(aname, char *, slen, "make_aname");
		aname_len = slen;
	}
	sprintf(aname, "%s[\"%.*s\"]", array->vname, (int) subs->stlen, subs->stptr);
	return aname;
}


/*
 *  get_array --- proceed to the actual Node_var_array,
 *	change Node_var_new to an array.
 *	If canfatal and type isn't good, die fatally,
 *	otherwise return the final actual value.
 */

NODE *
get_array(NODE *symbol, int canfatal)
{
	NODE *save_symbol = symbol;
	int isparam = FALSE;

	if (symbol->type == Node_param_list && (symbol->flags & FUNC) == 0) {
		save_symbol = symbol = GET_PARAM(symbol->param_cnt);
		isparam = TRUE;
		if (symbol->type == Node_array_ref)
			symbol = symbol->orig_array;
	}

	switch (symbol->type) {
	case Node_var_new:
		symbol->type = Node_var_array;
		symbol->var_array = NULL;
		/* fall through */
	case Node_var_array:
		break;

	case Node_array_ref:
	case Node_param_list:
		if ((symbol->flags & FUNC) == 0)
			cant_happen();
		/* else
			fall through */

	default:
		/* notably Node_var but catches also e.g. FS[1] = "x" */
		if (canfatal) {
			if (symbol->type == Node_val)
				fatal(_("attempt to use a scalar value as array"));

			if ((symbol->flags & FUNC) != 0)
				fatal(_("attempt to use function `%s' as an array"),
								save_symbol->vname);
			else if (isparam)
				fatal(_("attempt to use scalar parameter `%s' as an array"),
								save_symbol->vname);
			else
				fatal(_("attempt to use scalar `%s' as array"),
								save_symbol->vname);
		} else
			break;
	}

	return symbol;
}


/* set_SUBSEP --- update SUBSEP related variables when SUBSEP assigned to */
                                
void
set_SUBSEP()
{
	SUBSEP = force_string(SUBSEP_node->var_value)->stptr;
	SUBSEPlen = SUBSEP_node->var_value->stlen;
}                     

/* concat_exp --- concatenate expression list into a single string */

NODE *
concat_exp(int nargs, int do_subsep)
{
	/* do_subsep is false for Node-concat */
	NODE *r;
	char *str;
	char *s;
	long len;	/* NOT size_t, which is unsigned! */
	size_t subseplen = 0;
	int i;
	extern NODE **args_array;
	
	if (nargs == 1)
		return POP_STRING();

	if (do_subsep)
		subseplen = SUBSEPlen;

	len = -subseplen;
	for (i = 1; i <= nargs; i++) {
		r = POP();
		if (r->type == Node_var_array) {
			while (--i > 0)
				DEREF(args_array[i]);	/* avoid memory leak */
			fatal(_("attempt to use array `%s' in a scalar context"), array_vname(r));
		} 
		args_array[i] = force_string(r);
		len += r->stlen + subseplen;
	}

	emalloc(str, char *, len + 2, "concat_exp");

	r = args_array[nargs];
	memcpy(str, r->stptr, r->stlen);
	s = str + r->stlen;
	DEREF(r);
	for (i = nargs - 1; i > 0; i--) {
		if (subseplen == 1)
			*s++ = *SUBSEP;
		else if (subseplen > 0) {
			memcpy(s, SUBSEP, subseplen);
			s += subseplen;
		}
		r = args_array[i];
		memcpy(s, r->stptr, r->stlen);
		s += r->stlen;
		DEREF(r);
	}

	return make_str_node(str, len, ALREADY_MALLOCED);
}

/* ahash_unref --- remove reference to a Node_ahash */

void
ahash_unref(NODE *tmp)
{
	if (tmp == NULL)
		return;

	assert(tmp->type == Node_ahash);
		
	if (tmp->ahname_ref > 1)
		tmp->ahname_ref--;
	else {
		efree(tmp->ahname_str);
		freenode(tmp);
	}
}

/* assoc_clear --- flush all the values in symbol[] before doing a split() */

void
assoc_clear(NODE *symbol)
{
	long i;
	NODE *bucket, *next;

	if (symbol->var_array == NULL)
		return;
	for (i = 0; i < symbol->array_size; i++) {
		for (bucket = symbol->var_array[i]; bucket != NULL; bucket = next) {
			next = bucket->ahnext;
			if (bucket->ahvalue->type == Node_var_array) {
				NODE *r = bucket->ahvalue;
				assoc_clear(r);		/* recursively clear all sub-arrays */
				efree(r->vname);			
				freenode(r);
			} else
				unref(bucket->ahvalue);
			ahash_unref(bucket);	/* unref() will free the ahname_str */
		}
		symbol->var_array[i] = NULL;
	}
	efree(symbol->var_array);
	symbol->var_array = NULL;
	symbol->array_size = symbol->table_size = 0;
	symbol->flags &= ~ARRAYMAXED;
}

/* awk_hash --- calculate the hash function of the string in subs */

static unsigned long
awk_hash(const char *s, size_t len, unsigned long hsize, size_t *code)
{
	unsigned long h = 0;

	/*
	 * This is INCREDIBLY ugly, but fast.  We break the string up into
	 * 8 byte units.  On the first time through the loop we get the
	 * "leftover bytes" (strlen % 8).  On every other iteration, we
	 * perform 8 HASHC's so we handle all 8 bytes.  Essentially, this
	 * saves us 7 cmp & branch instructions.  If this routine is
	 * heavily used enough, it's worth the ugly coding.
	 *
	 * OZ's original sdbm hash, copied from Margo Seltzers db package.
	 */

	/*
	 * Even more speed:
	 * #define HASHC   h = *s++ + 65599 * h
	 * Because 65599 = pow(2, 6) + pow(2, 16) - 1 we multiply by shifts
	 */
#define HASHC   htmp = (h << 6);  \
		h = *s++ + htmp + (htmp << 10) - h

	unsigned long htmp;

	h = 0;

#if defined(VAXC)
	/*	
	 * This was an implementation of "Duff's Device", but it has been
	 * redone, separating the switch for extra iterations from the
	 * loop. This is necessary because the DEC VAX-C compiler is
	 * STOOPID.
	 */
	switch (len & (8 - 1)) {
	case 7:		HASHC;
	case 6:		HASHC;
	case 5:		HASHC;
	case 4:		HASHC;
	case 3:		HASHC;
	case 2:		HASHC;
	case 1:		HASHC;
	default:	break;
	}

	if (len > (8 - 1)) {
		size_t loop = len >> 3;
		do {
			HASHC;
			HASHC;
			HASHC;
			HASHC;
			HASHC;
			HASHC;
			HASHC;
			HASHC;
		} while (--loop);
	}
#else /* ! VAXC */
	/* "Duff's Device" for those who can handle it */
	if (len > 0) {
		size_t loop = (len + 8 - 1) >> 3;

		switch (len & (8 - 1)) {
		case 0:
			do {	/* All fall throughs */
				HASHC;
		case 7:		HASHC;
		case 6:		HASHC;
		case 5:		HASHC;
		case 4:		HASHC;
		case 3:		HASHC;
		case 2:		HASHC;
		case 1:		HASHC;
			} while (--loop);
		}
	}
#endif /* ! VAXC */
	if (code != NULL)
		*code = h;

	if (h >= hsize)
		h %= hsize;
	return h;
}

/* assoc_find --- locate symbol[subs] */

static NODE *				/* NULL if not found */
assoc_find(NODE *symbol, NODE *subs, unsigned long hash1)
{
	NODE *bucket;
	const char *s1_str;
	size_t s1_len;
	NODE *s2;

	for (bucket = symbol->var_array[hash1]; bucket != NULL;
			bucket = bucket->ahnext) {
		/*
		 * This used to use cmp_nodes() here.  That's wrong.
		 * Array indexes are strings; compare as such, always!
		 */
		s1_str = bucket->ahname_str;
		s1_len = bucket->ahname_len;
		s2 = subs;

		if (s1_len == s2->stlen) {
			if (s1_len == 0		/* "" is a valid index */
			    || memcmp(s1_str, s2->stptr, s1_len) == 0)
				return bucket;
		}
	}
	return NULL;
}

/* in_array --- test whether the array element symbol[subs] exists or not,
 * 		return pointer to value if it does.
 */

NODE *
in_array(NODE *symbol, NODE *subs)
{
	unsigned long hash1;
	NODE *ret;

	assert(symbol->type == Node_var_array);

	if (symbol->var_array == NULL)
		return NULL;

	hash1 = hash(subs->stptr, subs->stlen, (unsigned long) symbol->array_size, NULL);
	ret = assoc_find(symbol, subs, hash1);
	return (ret ? ret->ahvalue : NULL);
}

/*
 * assoc_lookup:
 * Find SYMBOL[SUBS] in the assoc array.  Install it with value "" if it
 * isn't there. Returns a pointer ala get_lhs to where its value is stored.
 *
 * SYMBOL is the address of the node (or other pointer) being dereferenced.
 * SUBS is a number or string used as the subscript. 
 */

NODE **
assoc_lookup(NODE *symbol, NODE *subs, int reference)
{
	unsigned long hash1;
	NODE *bucket;
	size_t code;

	assert(symbol->type == Node_var_array);

	(void) force_string(subs);

	if (symbol->var_array == NULL) {
		symbol->array_size = symbol->table_size = 0;	/* sanity */
		symbol->flags &= ~ARRAYMAXED;
		grow_table(symbol);
		hash1 = hash(subs->stptr, subs->stlen,
				(unsigned long) symbol->array_size, & code);
	} else {
		hash1 = hash(subs->stptr, subs->stlen,
				(unsigned long) symbol->array_size, & code);
		bucket = assoc_find(symbol, subs, hash1);
		if (bucket != NULL)
			return &(bucket->ahvalue);
	}

	if (do_lint && reference) {
		lintwarn(_("reference to uninitialized element `%s[\"%.*s\"]'"),
		      array_vname(symbol), (int)subs->stlen, subs->stptr);
	}

	/* It's not there, install it. */
	if (do_lint && subs->stlen == 0)
		lintwarn(_("subscript of array `%s' is null string"),
			array_vname(symbol));

	/* first see if we would need to grow the array, before installing */
	symbol->table_size++;
	if ((symbol->flags & ARRAYMAXED) == 0
	    && (symbol->table_size / symbol->array_size) > AVG_CHAIN_MAX) {
		grow_table(symbol);
		/* have to recompute hash value for new size */
		hash1 = code % (unsigned long) symbol->array_size;
	}

	getnode(bucket);
	bucket->type = Node_ahash;

	/*
	 * Freeze this string value --- it must never
	 * change, no matter what happens to the value
	 * that created it or to CONVFMT, etc.
	 *
	 * One day: Use an atom table to track array indices,
	 * and avoid the extra memory overhead.
	 */
	bucket->flags |= MALLOC;
	bucket->ahname_ref = 1;

	emalloc(bucket->ahname_str, char *, subs->stlen + 2, "assoc_lookup");
	bucket->ahname_len = subs->stlen;
	memcpy(bucket->ahname_str, subs->stptr, subs->stlen);
	bucket->ahname_str[bucket->ahname_len] = '\0';
	bucket->ahvalue = Nnull_string;
 
	bucket->ahnext = symbol->var_array[hash1];
	bucket->ahcode = code;
	symbol->var_array[hash1] = bucket;
	return &(bucket->ahvalue);
}

/* do_delete --- perform `delete array[s]' */

/*
 * `symbol' is array
 * `nsubs' is no of subscripts
 */

void
do_delete(NODE *symbol, int nsubs)
{
	unsigned long hash1;
	NODE *bucket, *last;
	NODE *subs;

	assert(symbol->type == Node_var_array);

#define free_subs(n)    do {                                    \
    NODE *s = PEEK(n - 1);                                      \
    if (s->type == Node_val) {                                  \
        (void) force_string(s);	/* may have side effects ? */   \
        DEREF(s);                                               \
    }                                                           \
} while (--n > 0)

	if (nsubs == 0) {	/* delete array */
		assoc_clear(symbol);
		return;
	}

	/* NB: subscripts are in reverse order on stack */
	subs = PEEK(nsubs - 1);
	if (subs->type != Node_val) {
		if (--nsubs > 0)
			free_subs(nsubs);
		fatal(_("attempt to use array `%s' in a scalar context"), array_vname(subs));
	}
	(void) force_string(subs);

	last = NULL;	/* shut up gcc -Wall */
	hash1 = 0;	/* ditto */

	if (symbol->var_array != NULL) {
		hash1 = hash(subs->stptr, subs->stlen,
				(unsigned long) symbol->array_size, NULL);
		last = NULL;
		for (bucket = symbol->var_array[hash1]; bucket != NULL;
				last = bucket, bucket = bucket->ahnext) {
			/*
			 * This used to use cmp_nodes() here.  That's wrong.
			 * Array indexes are strings; compare as such, always!
			 */
			const char *s1_str;
			size_t s1_len;
			NODE *s2;

			s1_str = bucket->ahname_str;
			s1_len = bucket->ahname_len;
			s2 = subs;
	
			if (s1_len == s2->stlen) {
				if (s1_len == 0		/* "" is a valid index */
				    || memcmp(s1_str, s2->stptr, s1_len) == 0)
					break;
			}
		}
	} else
		bucket = NULL;	/* The array is empty.  */

	if (bucket == NULL) {
		if (do_lint)
			lintwarn(_("delete: index `%s' not in array `%s'"),
				subs->stptr, array_vname(symbol));
		DEREF(subs);

		/* avoid memory leak, free rest of the subs */
		if (--nsubs > 0)
			free_subs(nsubs);
		return;
	}

	DEREF(subs);
	if (bucket->ahvalue->type == Node_var_array) {
		NODE *r = bucket->ahvalue;
		do_delete(r, nsubs - 1);
		if (r->var_array != NULL || nsubs > 1)
			return;
		/* else
				cleared a sub_array, free index */
	} else if (--nsubs > 0) {
		/* e.g.: a[1] = 1; delete a[1][1] */
		free_subs(nsubs);
		fatal(_("attempt to use scalar `%s[\"%.*s\"]' as an array"),
					symbol->vname, bucket->ahname_len, bucket->ahname_str);
	} else
		unref(bucket->ahvalue);

	if (last != NULL)
		last->ahnext = bucket->ahnext;
	else
		symbol->var_array[hash1] = bucket->ahnext;

	ahash_unref(bucket);	/* unref() will free the ahname_str */
	symbol->table_size--;
	if (symbol->table_size <= 0) {
		symbol->table_size = symbol->array_size = 0;
		symbol->flags &= ~ARRAYMAXED;
		if (symbol->var_array != NULL) {
			efree((char *) symbol->var_array);
			symbol->var_array = NULL;
		}
	}

#undef free_subs
}

/* do_delete_loop --- simulate ``for (iggy in foo) delete foo[iggy]'' */

/*
 * The primary hassle here is that `iggy' needs to have some arbitrary
 * array index put in it before we can clear the array, we can't
 * just replace the loop with `delete foo'.
 */

void
do_delete_loop(NODE *symbol, NODE **lhs)
{
	long i;

	assert(symbol->type == Node_var_array);

	if (symbol->var_array == NULL)
		return;

	/* get first index value */
	for (i = 0; i < symbol->array_size; i++) {
		if (symbol->var_array[i] != NULL) {
			unref(*lhs);
			*lhs = make_string(symbol->var_array[i]->ahname_str,
					symbol->var_array[i]->ahname_len);
			break;
		}
	}

	/* blast the array in one shot */
	assoc_clear(symbol);
}

/* grow_table --- grow a hash table */

static void
grow_table(NODE *symbol)
{
	NODE **old, **new, *chain, *next;
	int i, j;
	unsigned long hash1;
	unsigned long oldsize, newsize, k;
	/*
	 * This is an array of primes. We grow the table by an order of
	 * magnitude each time (not just doubling) so that growing is a
	 * rare operation. We expect, on average, that it won't happen
	 * more than twice.  The final size is also chosen to be small
	 * enough so that MS-DOG mallocs can handle it. When things are
	 * very large (> 8K), we just double more or less, instead of
	 * just jumping from 8K to 64K.
	 */
	static const long sizes[] = { 13, 127, 1021, 8191, 16381, 32749, 65497,
				131101, 262147, 524309, 1048583, 2097169,
				4194319, 8388617, 16777259, 33554467, 
				67108879, 134217757, 268435459, 536870923,
				1073741827
	};

	/* find next biggest hash size */
	newsize = oldsize = symbol->array_size;
	for (i = 0, j = sizeof(sizes)/sizeof(sizes[0]); i < j; i++) {
		if (oldsize < sizes[i]) {
			newsize = sizes[i];
			break;
		}
	}

	if (newsize == oldsize) {	/* table already at max (!) */
		symbol->flags |= ARRAYMAXED;
		return;
	}

	/* allocate new table */
	emalloc(new, NODE **, newsize * sizeof(NODE *), "grow_table");
	memset(new, '\0', newsize * sizeof(NODE *));

	/* brand new hash table, set things up and return */
	if (symbol->var_array == NULL) {
		symbol->table_size = 0;
		goto done;
	}

	/* old hash table there, move stuff to new, free old */
	old = symbol->var_array;
	for (k = 0; k < oldsize; k++) {
		if (old[k] == NULL)
			continue;

		for (chain = old[k]; chain != NULL; chain = next) {
			next = chain->ahnext;
			hash1 = chain->ahcode % newsize;

			/* remove from old list, add to new */
			chain->ahnext = new[hash1];
			new[hash1] = chain;
		}
	}
	efree(old);

done:
	/*
	 * note that symbol->table_size does not change if an old array,
	 * and is explicitly set to 0 if a new one.
	 */
	symbol->var_array = new;
	symbol->array_size = newsize;
}

/* pr_node --- print simple node info */

static void
pr_node(NODE *n)
{
	if ((n->flags & (NUMCUR|NUMBER)) != 0)
		printf("%s %g p: %p", flags2str(n->flags), n->numbr, n);
	else
		printf("%s %.*s p: %p", flags2str(n->flags), (int) n->stlen, n->stptr, n);
}


static void
indent(int indent_level)
{
	int k;
	for (k = 0; k < indent_level; k++)
		printf("\t");
}

/* assoc_dump --- dump the contents of an array */

NODE *
assoc_dump(NODE *symbol, int indent_level)
{
	long i;
	NODE *bucket;

	indent(indent_level);
	if (symbol->var_array == NULL) {
		printf(_("%s: empty (null)\n"), symbol->vname);
		return make_number((AWKNUM) 0);
	}

	if (symbol->table_size == 0) {
		printf(_("%s: empty (zero)\n"), symbol->vname);
		return make_number((AWKNUM) 0);
	}

	printf(_("%s: table_size = %d, array_size = %d\n"), symbol->vname,
			(int) symbol->table_size, (int) symbol->array_size);

	for (i = 0; i < symbol->array_size; i++) {
		for (bucket = symbol->var_array[i]; bucket != NULL;
				bucket = bucket->ahnext) {
			indent(indent_level);
			printf("%s: I: [len %d <%.*s> p: %p] V: [",
				symbol->vname,
				(int) bucket->ahname_len,
				(int) bucket->ahname_len,
				bucket->ahname_str,
				bucket->ahname_str);
			if (bucket->ahvalue->type == Node_var_array) {
				printf("\n");
				assoc_dump(bucket->ahvalue, indent_level + 1);
				indent(indent_level);
			} else
				pr_node(bucket->ahvalue);
			printf("]\n");
		}
	}

	return make_number((AWKNUM) 0);
}

/* do_adump --- dump an array: interface to assoc_dump */

NODE *
do_adump(int nargs)
{
	NODE *r, *a;

	a = POP();
	if (a->type == Node_param_list) {
		printf(_("%s: is parameter\n"), a->vname);
		a = GET_PARAM(a->param_cnt);
	}
	if (a->type == Node_array_ref) {
		printf(_("%s: array_ref to %s\n"), a->vname,
					a->orig_array->vname);
		a = a->orig_array;
	}
	if (a->type != Node_var_array)
		fatal(_("adump: argument not an array"));
	r = assoc_dump(a, 0);
	return r;
}

/*
 * The following functions implement the builtin
 * asort function.  Initial work by Alan J. Broder,
 * ajb@woti.com.
 */

/* dup_table --- duplicate input symbol table "symbol" */

static void
dup_table(NODE *symbol, NODE *newsymb)
{
	NODE **old, **new, *chain, *bucket;
	long i;
	unsigned long cursize;

	/* find the current hash size */
	cursize = symbol->array_size;

	new = NULL;

	/* input is a brand new hash table, so there's nothing to copy */
	if (symbol->var_array == NULL)
		newsymb->table_size = 0;
	else {
		/* old hash table there, dupnode stuff into a new table */

		/* allocate new table */
		emalloc(new, NODE **, cursize * sizeof(NODE *), "dup_table");
		memset(new, '\0', cursize * sizeof(NODE *));

		/* do the copying/dupnode'ing */
		old = symbol->var_array;
		for (i = 0; i < cursize; i++) {
			if (old[i] != NULL) {
				for (chain = old[i]; chain != NULL;
						chain = chain->ahnext) {
					/* get a node for the linked list */
					getnode(bucket);
					bucket->type = Node_ahash;
					bucket->flags |= MALLOC;
					bucket->ahname_ref = 1;

					/*
					 * copy the corresponding name and
					 * value from the original input list
					 */
					emalloc(bucket->ahname_str, char *, chain->ahname_len + 2, "dup_table");
					bucket->ahname_len = chain->ahname_len;

					memcpy(bucket->ahname_str, chain->ahname_str, chain->ahname_len);
					bucket->ahname_str[bucket->ahname_len] = '\0';

					if (chain->ahvalue->type == Node_var_array) {
						NODE *r;
						char *aname;
						size_t aname_len;
						getnode(r);
						r->type = Node_var_array;
						aname_len = strlen(newsymb->vname) + chain->ahname_len + 4;
						emalloc(aname, char *, aname_len + 2, "dup_table");
						sprintf(aname, "%s[\"%.*s\"]", newsymb->vname, (int) chain->ahname_len, chain->ahname_str);
						r->vname = aname;
						dup_table(chain->ahvalue, r);
						bucket->ahvalue = r;
					} else
						bucket->ahvalue = dupnode(chain->ahvalue);

					/*
					 * put the node on the corresponding
					 * linked list in the new table
					 */
					bucket->ahnext = new[i];
					new[i] = bucket;
				}
			}
		}
		newsymb->table_size = symbol->table_size;
	}

	newsymb->var_array = new;
	newsymb->array_size = cursize;
}

/* merge --- do a merge of two sorted lists */

static NODE *
merge(NODE *left, NODE *right)
{
	NODE *ans, *cur;

	/*
	 * The use of cmp_nodes() here means that IGNORECASE influences the
	 * comparison.  This is OK, but it may be surprising.  This comment
	 * serves to remind us that we know about this and that it's OK.
	 */
	if (cmp_nodes(left->ahvalue, right->ahvalue) <= 0) {
		ans = cur = left;
		left = left->ahnext;
	} else {
		ans = cur = right;
		right = right->ahnext;
	}

	while (left != NULL && right != NULL) {
		if (cmp_nodes(left->ahvalue, right->ahvalue) <= 0) {
			cur->ahnext = left;
			cur = left;
			left  = left->ahnext;
		} else {
			cur->ahnext = right;
			cur = right;
			right = right->ahnext;
		}
	}

	cur->ahnext = (left != NULL ? left : right);

	return ans;
}

/* merge_sort --- recursively sort the left and right sides of a list */

static NODE *
merge_sort(NODE *left, unsigned long size)
{
	NODE *right, *tmp;
	int i, half;

	if (size <= 1)
		return left;

	/* walk down the list, till just one before the midpoint */
	tmp = left;
	half = size / 2;
	for (i = 0; i < half-1; i++)
		tmp = tmp->ahnext;

	/* split the list into two parts */
	right = tmp->ahnext;
	tmp->ahnext = NULL;

	/* sort the left and right parts of the list */
	left  = merge_sort(left,       half);
	right = merge_sort(right, size-half);

	/* merge the two sorted parts of the list */
	return merge(left, right);
}


/*
 * assoc_from_list -- Populate an array with the contents of a list of NODEs, 
 * using increasing integers as the key.
 */

static void
assoc_from_list(NODE *symbol, NODE *list)
{
	NODE *next;
	unsigned long i = 0;
	unsigned long hash1;
	char buf[100];

	for (; list != NULL; list = next) {
		size_t code;

		next = list->ahnext;

		/* make an int out of i++ */
		i++;
		sprintf(buf, "%lu", i);
		assert(list->ahname_str == NULL);
		assert(list->ahname_ref == 1);
		emalloc(list->ahname_str, char *, strlen(buf) + 2, "assoc_from_list");
		list->ahname_len = strlen(buf);
		strcpy(list->ahname_str, buf);

		/* find the bucket where it belongs */
		hash1 = hash(list->ahname_str, list->ahname_len,
				symbol->array_size, & code);
		list->ahcode = code;

		/* link the node into the chain at that bucket */
		list->ahnext = symbol->var_array[hash1];
		symbol->var_array[hash1] = list;
	}
}

/*
 * assoc_sort_inplace --- sort all the values in symbol[], replacing
 * the sorted values back into symbol[], indexed by integers starting with 1.
 */

typedef enum asort_how { VALUE, INDEX } ASORT_TYPE;

static NODE *
assoc_sort_inplace(NODE *symbol, ASORT_TYPE how)
{
	unsigned long i, num;
	NODE *bucket, *next, *list;

	if (symbol->var_array == NULL
	    || symbol->array_size <= 0
	    || symbol->table_size <= 0)
		return make_number((AWKNUM) 0);

	/* build a linked list out of all the entries in the table */
	if (how == VALUE) {
		list = NULL;
		num = 0;
		for (i = 0; i < symbol->array_size; i++) {
			for (bucket = symbol->var_array[i]; bucket != NULL; bucket = next) {
				if (bucket->ahvalue->type == Node_var_array)
					fatal(_("attempt to use array in a scalar context"));
				next = bucket->ahnext;
				if (bucket->ahname_ref == 1) {
					efree(bucket->ahname_str);
					bucket->ahname_str = NULL;
					bucket->ahname_len = 0;
				} else {
					NODE *r;

					getnode(r);
					*r = *bucket;
					ahash_unref(bucket);
					bucket = r;
					bucket->flags |= MALLOC;
					bucket->ahname_ref = 1;
					bucket->ahname_str = NULL;
					bucket->ahname_len = 0;
				}
				bucket->ahnext = list;
				list = bucket;
				num++;
			}
			symbol->var_array[i] = NULL;
		}
	} else {	/* how == INDEX */
		list = NULL;
		num = 0;
		for (i = 0; i < symbol->array_size; i++) {
			for (bucket = symbol->var_array[i]; bucket != NULL; bucket = next) {
				next = bucket->ahnext;

				/* toss old value */
				if (bucket->ahvalue->type == Node_var_array) {
					NODE *r = bucket->ahvalue;
					assoc_clear(r);
					efree(r->vname);
					freenode(r);
				} else
					unref(bucket->ahvalue);

				/* move index into value */
				if (bucket->ahname_ref == 1) {
					bucket->ahvalue = make_str_node(bucket->ahname_str,
								bucket->ahname_len, ALREADY_MALLOCED);
					bucket->ahname_str = NULL;
					bucket->ahname_len = 0;
				} else {
					NODE *r;

					bucket->ahvalue = make_string(bucket->ahname_str, bucket->ahname_len);
					getnode(r);
					*r = *bucket;
					ahash_unref(bucket);
					bucket = r;
					bucket->flags |= MALLOC;
					bucket->ahname_ref = 1;
					bucket->ahname_str = NULL;
					bucket->ahname_len = 0;
				}

				bucket->ahnext = list;
				list = bucket;
				num++;
			}
			symbol->var_array[i] = NULL;
		}
	}

	/*
	 * Sort the linked list of NODEs.
	 * (The especially nice thing about using a merge sort here is that
	 * we require absolutely no additional storage. This is handy if the
	 * array has grown to be very large.)
	 */
	list = merge_sort(list, num);

	/*
	 * now repopulate the original array, using increasing
	 * integers as the key
	 */
	assoc_from_list(symbol, list);

	return make_number((AWKNUM) num);
}

/* asort_actual --- do the actual work to sort the input array */

static NODE *
asort_actual(int nargs, ASORT_TYPE how)
{
	NODE *dest = NULL;
	NODE *array;

	if (nargs == 2) {  /* 2nd optional arg */
		dest = POP_PARAM();
		if (dest->type != Node_var_array) {
			fatal(how == VALUE ?
				_("asort: second argument not an array") :
				_("asorti: second argument not an array"));
		}
		assoc_clear(dest);
	}

	array = POP_PARAM();
	if (array->type != Node_var_array) {
		fatal(how == VALUE ?
			_("asort: first argument not an array") :
			_("asorti: first argument not an array"));
	}

	if (dest != NULL) {
		dup_table(array, dest);
		array = dest;
	}

	return assoc_sort_inplace(array, how);
}

/* do_asort --- sort array by value */

NODE *
do_asort(int nargs)
{
	return asort_actual(nargs, VALUE);
}

/* do_asorti --- sort array by index */

NODE *
do_asorti(int nargs)
{
	return asort_actual(nargs, INDEX);
}

/*
From bonzini@gnu.org  Mon Oct 28 16:05:26 2002
Date: Mon, 28 Oct 2002 13:33:03 +0100
From: Paolo Bonzini <bonzini@gnu.org>
To: arnold@skeeve.com
Subject: Hash function
Message-ID: <20021028123303.GA6832@biancaneve>

Here is the hash function I'm using in GNU Smalltalk.  The scrambling is
needed if you use powers of two as the table sizes.  If you use primes it
is not needed.

To use double-hashing with power-of-two size, you should use the
_gst_hash_string(str, len) as the primary hash and
scramble(_gst_hash_string (str, len)) | 1 as the secondary hash.

Paolo

*/
/*
 * ADR: Slightly modified to work w/in the context of gawk.
 */

static unsigned long
gst_hash_string(const char *str, size_t len, unsigned long hsize, size_t *code)
{
	unsigned long hashVal = 1497032417;    /* arbitrary value */
	unsigned long ret;

	while (len--) {
		hashVal += *str++;
		hashVal += (hashVal << 10);
		hashVal ^= (hashVal >> 6);
	}

	ret = scramble(hashVal);

	if (code != NULL)
		*code = ret;

	if (ret >= hsize)
		ret %= hsize;

	return ret;
}

static unsigned long
scramble(unsigned long x)
{
	if (sizeof(long) == 4) {
		int y = ~x;

		x += (y << 10) | (y >> 22);
		x += (x << 6)  | (x >> 26);
		x -= (x << 16) | (x >> 16);
	} else {
		x ^= (~x) >> 31;
		x += (x << 21) | (x >> 11);
		x += (x << 5) | (x >> 27);
		x += (x << 27) | (x >> 5);
		x += (x << 31);
	}

	return x;
}
