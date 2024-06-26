/*
 * printf.c - All the code for sprintf/printf.
 */

/*
 * Copyright (C) 1986, 1988, 1989, 1991-2024,
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


#include "awk.h"

extern int max_args;
extern NODE **args_array;
extern FILE *output_fp;

#define DEFAULT_G_PRECISION 6

static size_t mbc_byte_count(const char *ptr, size_t numchars);
static size_t mbc_char_count(const char *ptr, size_t numbytes);
static void reverse(char *str);
static const char *add_thousands(const char *original, struct lconv *loc);

#ifdef HAVE_MPFR

/*
 * mpz2mpfr --- convert an arbitrary-precision integer to a float
 *	without any loss of precision. The returned value is only
 * 	good for temporary use.
 */


static mpfr_ptr
mpz2mpfr(mpz_ptr zi)
{
	size_t prec;
	static mpfr_t mpfrval;
	static bool inited = false;
	int tval;

	/* estimate minimum precision for exact conversion */
	prec = mpz_sizeinbase(zi, 2);	/* most significant 1 bit position starting at 1 */
	prec -= (size_t) mpz_scan1(zi, 0);	/* least significant 1 bit index starting at 0 */
	if (prec < MPFR_PREC_MIN)
		prec = MPFR_PREC_MIN;
	else if (prec > MPFR_PREC_MAX)
		prec = MPFR_PREC_MAX;

	if (! inited) {
		mpfr_init2(mpfrval, prec);
		inited = true;
	} else
		mpfr_set_prec(mpfrval, prec);
	tval = mpfr_set_z(mpfrval, zi, ROUND_MODE);
	IEEE_FMT(mpfrval, tval);
	return mpfrval;
}
#endif

/*
 * format_tree() formats arguments of sprintf,
 * and accordingly to a fmt_string providing a format like in
 * printf family from C library.  Returns a string node which value
 * is a formatted string.  Called by  sprintf function.
 *
 * It is one of the uglier parts of gawk.  Thanks to Michal Jaegermann
 * for taming this beast and making it compatible with ANSI C.
 */

NODE *
format_tree(
	const char *fmt_string,
	size_t n0,
	NODE **the_args,
	long num_args)
{
/* copy 'l' bytes from 's' to 'obufout' checking for space in the process */
/* difference of pointers should be of ptrdiff_t type, but let us be kind */
#define bchunk(s, l) if (l) { \
	while ((l) > ofre) { \
		size_t olen = obufout - obuf; \
		erealloc(obuf, char *, osiz * 2, "format_tree"); \
		ofre += osiz; \
		osiz *= 2; \
		obufout = obuf + olen; \
	} \
	memcpy(obufout, s, (size_t) (l)); \
	obufout += (l); \
	ofre -= (l); \
}

/* copy one byte from 's' to 'obufout' checking for space in the process */
#define bchunk_one(s) { \
	if (ofre < 1) { \
		size_t olen = obufout - obuf; \
		erealloc(obuf, char *, osiz * 2, "format_tree"); \
		ofre += osiz; \
		osiz *= 2; \
		obufout = obuf + olen; \
	} \
	*obufout++ = *s; \
	--ofre; \
}

/* Is there space for something L big in the buffer? */
#define chksize(l)  if ((l) >= ofre) { \
	size_t olen = obufout - obuf; \
	size_t delta = osiz+l-ofre; \
	erealloc(obuf, char *, osiz + delta, "format_tree"); \
	obufout = obuf + olen; \
	ofre += delta; \
	osiz += delta; \
}

	size_t cur_arg = 0;
	NODE *r = NULL;
	int i, nc;
	bool toofew = false;
	char *obuf, *obufout;
	size_t osiz, ofre, olen_final;
	const char *chbuf;
	const char *s0, *s1;
	int cs1;
	NODE *arg;
	long fw, prec, argnum;
	bool used_dollar;
	bool lj, alt, have_prec, need_format;
	long *cur = NULL;
	uintmax_t uval;
	bool sgn;
	int base;
	/*
	 * Although this is an array, the elements serve two different
	 * purposes. The first element is the general buffer meant
	 * to hold the entire result string.  The second one is a
	 * temporary buffer for large floating point values. They
	 * could just as easily be separate variables, and the
	 * code might arguably be clearer.
	 */
	struct {
		char *buf;
		size_t bufsize;
		char stackbuf[30];
	} cpbufs[2];
#define cpbuf	cpbufs[0].buf
	char *cend = &cpbufs[0].stackbuf[sizeof(cpbufs[0].stackbuf)];
	char *cp;
	const char *fill;
	AWKNUM tmpval = 0.0;
	char signchar = '\0';
	size_t len;
	bool zero_flag = false;
	bool quote_flag = false;
	int ii, jj;
	char *chp;
	size_t copy_count, char_count;
	char *nan_inf_val;
	bool magic_posix_flag;
#ifdef HAVE_MPFR
	mpz_ptr zi;
	mpfr_ptr mf;
#endif
	enum { MP_NONE = 0, MP_INT_WITH_PREC = 1, MP_INT_WITHOUT_PREC, MP_FLOAT } fmt_type;

	static const char sp[] = " ";
	static const char zero_string[] = "0";
	static const char lchbuf[] = "0123456789abcdef";
	static const char Uchbuf[] = "0123456789ABCDEF";
	static const char bad_modifiers[] = "hjlLtz";
	static bool warned[sizeof(bad_modifiers)-1];	// auto-init to zero

	bool modifier_seen[sizeof(bad_modifiers)-1];
#define modifier_index(c)  (strchr(bad_modifiers, c) - bad_modifiers)

#define INITIAL_OUT_SIZE	64
	emalloc(obuf, char *, INITIAL_OUT_SIZE, "format_tree");
	obufout = obuf;
	osiz = INITIAL_OUT_SIZE;
	ofre = osiz - 1;

	cur_arg = 1;

	{
		size_t k;
		for (k = 0; k < sizeof(cpbufs)/sizeof(cpbufs[0]); k++) {
			cpbufs[k].bufsize = sizeof(cpbufs[k].stackbuf);
			cpbufs[k].buf = cpbufs[k].stackbuf;
		}
	}

	/*
	 * The point of this goop is to grow the buffer
	 * holding the converted number, so that large
	 * values don't overflow a fixed length buffer.
	 */
#define PREPEND(CH) do {	\
	if (cp == cpbufs[0].buf) {	\
		char *prev = cpbufs[0].buf;	\
		emalloc(cpbufs[0].buf, char *, 2*cpbufs[0].bufsize, \
		 	"format_tree");	\
		memcpy((cp = cpbufs[0].buf+cpbufs[0].bufsize), prev,	\
		       cpbufs[0].bufsize);	\
		cpbufs[0].bufsize *= 2;	\
		if (prev != cpbufs[0].stackbuf)	\
			efree(prev);	\
		cend = cpbufs[0].buf+cpbufs[0].bufsize;	\
	}	\
	*--cp = (CH);	\
} while(0)

	/*
	 * Check first for use of `count$'.
	 * If plain argument retrieval was used earlier, choke.
	 *	Otherwise, return the requested argument.
	 * If not `count$' now, but it was used earlier, choke.
	 * If this format is more than total number of args, choke.
	 * Otherwise, return the current argument.
	 */
#define parse_next_arg() { \
	if (argnum > 0) { \
		if (cur_arg > 1) { \
			msg(_("fatal: must use `count$' on all formats or none")); \
			goto out; \
		} \
		arg = the_args[argnum]; \
	} else if (used_dollar) { \
		msg(_("fatal: must use `count$' on all formats or none")); \
		arg = 0; /* shutup the compiler */ \
		goto out; \
	} else if (cur_arg >= num_args) { \
		arg = 0; /* shutup the compiler */ \
		toofew = true; \
		break; \
	} else { \
		arg = the_args[cur_arg]; \
		cur_arg++; \
	} \
}

	need_format = false;
	used_dollar = false;

	s0 = s1 = fmt_string;
	while (n0-- > 0) {
		if (*s1 != '%') {
			s1++;
			continue;
		}
		need_format = true;
		bchunk(s0, s1 - s0);
		s0 = s1;
		cur = &fw;
		fw = 0;
		prec = 0;
		base = 0;
		argnum = 0;
		base = 0;
		have_prec = false;
		signchar = '\0';
		zero_flag = false;
		quote_flag = false;
		nan_inf_val = NULL;
#ifdef HAVE_MPFR
		mf = NULL;
		zi = NULL;
#endif
		fmt_type = MP_NONE;

		lj = alt = false;
		memset(modifier_seen, 0, sizeof(modifier_seen));
		magic_posix_flag = false;
		fill = sp;
		cp = cend;
		chbuf = lchbuf;
		s1++;

retry:
		if (n0-- == 0)	/* ran out early! */
			break;

		switch (cs1 = *s1++) {
		case (-1):	/* dummy case to allow for checking */
check_pos:
			if (cur != &fw)
				break;		/* reject as a valid format */
			goto retry;
		case '%':
			need_format = false;
			/*
			 * 29 Oct. 2002:
			 * The C99 standard pages 274 and 279 seem to imply that
			 * since there's no arg converted, the field width doesn't
			 * apply.  The code already was that way, but this
			 * comment documents it, at least in the code.
			 */
			if (do_lint) {
				const char *msg = NULL;

				if (fw && ! have_prec)
					msg = _("field width is ignored for `%%' specifier");
				else if (fw == 0 && have_prec)
					msg = _("precision is ignored for `%%' specifier");
				else if (fw && have_prec)
					msg = _("field width and precision are ignored for `%%' specifier");

				if (msg != NULL)
					lintwarn("%s", msg);
			}
			bchunk_one("%");
			s0 = s1;
			break;

		case '0':
			/*
			 * Only turn on zero_flag if we haven't seen
			 * the field width or precision yet.  Otherwise,
			 * screws up floating point formatting.
			 */
			if (cur == & fw)
				zero_flag = true;
			if (lj)
				goto retry;
			/* fall through */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (cur == NULL)
				break;
			if (prec >= 0)
				*cur = cs1 - '0';
			/*
			 * with a negative precision *cur is already set
			 * to -1, so it will remain negative, but we have
			 * to "eat" precision digits in any case
			 */
			while (n0 > 0 && *s1 >= '0' && *s1 <= '9') {
				--n0;
				*cur = *cur * 10 + *s1++ - '0';
			}
			if (prec < 0) 	/* negative precision is discarded */
				have_prec = false;
			if (cur == &prec)
				cur = NULL;
			if (n0 == 0)	/* badly formatted control string */
				continue;
			goto retry;
		case '$':
			if (do_traditional) {
				msg(_("fatal: `$' is not permitted in awk formats"));
				goto out;
			}

			if (cur == &fw) {
				argnum = fw;
				fw = 0;
				used_dollar = true;
				if (argnum <= 0) {
					msg(_("fatal: argument index with `$' must be > 0"));
					goto out;
				}
				if (argnum >= num_args) {
					msg(_("fatal: argument index %ld greater than total number of supplied arguments"), argnum);
					goto out;
				}
			} else {
				msg(_("fatal: `$' not permitted after period in format"));
				goto out;
			}

			goto retry;
		case '*':
			if (cur == NULL)
				break;
			if (! do_traditional && used_dollar && ! isdigit((unsigned char) *s1)) {
				fatal(_("fatal: must use `count$' on all formats or none"));
				break;	/* silence warnings */
			} else if (! do_traditional && isdigit((unsigned char) *s1)) {
				int val = 0;

				for (; n0 > 0 && *s1 && isdigit((unsigned char) *s1); s1++, n0--) {
					val *= 10;
					val += *s1 - '0';
				}
				if (*s1 != '$') {
					msg(_("fatal: no `$' supplied for positional field width or precision"));
					goto out;
				} else {
					s1++;
					n0--;
				}
				// val could be less than zero if someone provides a field width
				// so large that it causes integer overflow. Mainly fuzzers do this,
				// but let's try to be good anyway.
				if (val < 0 || val >= num_args) {
					toofew = true;
					break;
				}
				arg = the_args[val];
			} else {
				parse_next_arg();
			}
			(void) force_number(arg);
			*cur = get_number_si(arg);
			if (*cur < 0 && cur == &fw) {
				*cur = -*cur;
				lj = true;
			}
			if (cur == &prec) {
				if (*cur >= 0)
					have_prec = true;
				else
					have_prec = false;
				cur = NULL;
			}
			goto retry;
		case ' ':		/* print ' ' or '-' */
					/* 'space' flag is ignored */
					/* if '+' already present  */
			if (signchar != false)
				goto check_pos;
			/* FALL THROUGH */
		case '+':		/* print '+' or '-' */
			signchar = cs1;
			goto check_pos;
		case '-':
			if (prec < 0)
				break;
			if (cur == &prec) {
				prec = -1;
				goto retry;
			}
			fill = sp;      /* if left justified then other */
			lj = true;	/* filling is ignored */
			goto check_pos;
		case '.':
			if (cur != &fw)
				break;
			cur = &prec;
			have_prec = true;
			goto retry;
		case '#':
			alt = true;
			goto check_pos;
		case '\'':
#if defined(HAVE_LOCALE_H)
			quote_flag = true;
			goto check_pos;
#else
			goto retry;
#endif
		case 'h':
		case 'j':
		case 'l':
		case 'L':
		case 't':
		case 'z':
			if (modifier_seen[modifier_index(cs1)])
				break;
			else {
				int ind = modifier_index(cs1);

				if (do_lint && ! warned[ind]) {
					lintwarn(_("`%c' is meaningless in awk formats; ignored"), cs1);
					warned[ind] = true;
				}
				if (do_posix) {
					msg(_("fatal: `%c' is not permitted in POSIX awk formats"), cs1);
					goto out;
				}
			}
			modifier_seen[modifier_index(cs1)] = true;
			goto retry;

		case 'P':
			if (magic_posix_flag)
				break;
			magic_posix_flag = true;
			goto retry;
		case 'c':
			need_format = false;
			parse_next_arg();
			/* user input that looks numeric is numeric */
			fixtype(arg);
			if ((arg->flags & NUMBER) != 0) {
				uval = get_number_uj(arg);
				if (gawk_mb_cur_max > 1) {
					char buf[100];
					wchar_t wc;
					mbstate_t mbs;
					size_t count;

					memset(& mbs, 0, sizeof(mbs));

					/* handle systems with too small wchar_t */
					if (sizeof(wchar_t) < 4 && uval > 0xffff) {
						if (do_lint)
							lintwarn(
						_("[s]printf: value %g is too big for %%c format"),
									arg->numbr);

						goto out0;
					}

					wc = uval;

					count = wcrtomb(buf, wc, & mbs);
					if (count == 0
					    || count == (size_t) -1) {
						if (do_lint)
							lintwarn(
						_("[s]printf: value %g is not a valid wide character"),
									arg->numbr);

						goto out0;
					}

					memcpy(cpbuf, buf, count);
					prec = count;
					cp = cpbuf;
					goto pr_tail;
				}
out0:
				;
				/* else,
					fall through */

				cpbuf[0] = uval;
				prec = 1;
				cp = cpbuf;
				goto pr_tail;
			}
			/*
			 * As per POSIX, only output first character of a
			 * string value.  Thus, we ignore any provided
			 * precision, forcing it to 1.  (Didn't this
			 * used to work? 6/2003.)
			 */
			cp = arg->stptr;
			prec = 1;
			/*
			 * First character can be multiple bytes if
			 * it's a multibyte character. Grr.
			 */
			if (gawk_mb_cur_max > 1) {
				mbstate_t state;
				size_t count;

				memset(& state, 0, sizeof(state));
				count = mbrlen(cp, arg->stlen, & state);
				if (count != (size_t) -1 && count != (size_t) -2 && count > 0) {
					prec = count;
					/* may need to increase fw so that padding happens, see pr_tail code */
					if (fw > 0)
						fw += count - 1;
				}
			}
			goto pr_tail;
		case 's':
			need_format = false;
			parse_next_arg();
			arg = force_string(arg);
			if (fw == 0 && ! have_prec)
				prec = arg->stlen;
			else {
				char_count = mbc_char_count(arg->stptr, arg->stlen);
				if (! have_prec || prec > char_count)
					prec = char_count;
			}
			cp = arg->stptr;
			goto pr_tail;
		case 'd':
		case 'i':
			need_format = false;
			parse_next_arg();
			(void) force_number(arg);

			/*
			 * Check for Nan or Inf.
			 */
			if (out_of_range(arg))
				goto out_of_range;
#ifdef HAVE_MPFR
			if (is_mpg_float(arg))
				goto mpf0;
			else if (is_mpg_integer(arg))
				goto mpz0;
			else
#endif
			tmpval = double_to_int(arg->numbr);

			/*
			 * ``The result of converting a zero value with a
			 * precision of zero is no characters.''
			 */
			if (have_prec && prec == 0 && tmpval == 0)
				goto pr_tail;

			if (tmpval < 0) {
				tmpval = -tmpval;
				sgn = true;
			} else {
				if (tmpval == -0.0)
					/* avoid printing -0 */
					tmpval = 0.0;
				sgn = false;
			}
			/*
			 * Use snprintf return value to tell if there
			 * is enough room in the buffer or not.
			 */
			while ((i = snprintf(cpbufs[1].buf,
					     cpbufs[1].bufsize, "%.0f",
					     tmpval)) >=
			       cpbufs[1].bufsize) {
				if (cpbufs[1].buf == cpbufs[1].stackbuf)
					cpbufs[1].buf = NULL;
				if (i > 0) {
					cpbufs[1].bufsize += ((i > cpbufs[1].bufsize) ?
							      i : cpbufs[1].bufsize);
				}
				else
					cpbufs[1].bufsize *= 2;
				assert(cpbufs[1].bufsize > 0);
				erealloc(cpbufs[1].buf, char *,
					 cpbufs[1].bufsize, "format_tree");
			}
			if (i < 1)
				goto out_of_range;
#if defined(HAVE_LOCALE_H)
			quote_flag = (quote_flag && loc.thousands_sep[0] != 0);
#endif
			chp = &cpbufs[1].buf[i-1];
			ii = jj = 0;
			do {
				PREPEND(*chp);
				chp--; i--;
#if defined(HAVE_LOCALE_H)
				if (quote_flag && loc.grouping[ii] && ++jj == loc.grouping[ii]) {
					if (i) {	/* only add if more digits coming */
						int k;
						const char *ts = loc.thousands_sep;

						for (k = strlen(ts) - 1; k >= 0; k--) {
							PREPEND(ts[k]);
						}
					}
					if (loc.grouping[ii+1] == 0)
						jj = 0;		/* keep using current val in loc.grouping[ii] */
					else if (loc.grouping[ii+1] == CHAR_MAX)
						quote_flag = false;
					else {
						ii++;
						jj = 0;
					}
				}
#endif
			} while (i > 0);

			/* add more output digits to match the precision */
			if (have_prec) {
				while (cend - cp < prec)
					PREPEND('0');
			}

			if (sgn)
				PREPEND('-');
			else if (signchar)
				PREPEND(signchar);
			/*
			 * When to fill with zeroes is of course not simple.
			 * First: No zero fill if left-justifying.
			 * Next: There seem to be two cases:
			 * 	A '0' without a precision, e.g. %06d
			 * 	A precision with no field width, e.g. %.10d
			 * Any other case, we don't want to fill with zeroes.
			 */
			if (! lj
			    && ((zero_flag && ! have_prec)
				 || (fw == 0 && have_prec)))
				fill = zero_string;
			if (prec > fw)
				fw = prec;
			prec = cend - cp;
			if (fw > prec && ! lj && fill != sp
			    && (*cp == '-' || signchar)) {
				bchunk_one(cp);
				cp++;
				prec--;
				fw--;
			}
			goto pr_tail;
		case 'X':
			chbuf = Uchbuf;	/* FALL THROUGH */
		case 'x':
			base += 6;	/* FALL THROUGH */
		case 'u':
			base += 2;	/* FALL THROUGH */
		case 'o':
			base += 8;
			need_format = false;
			parse_next_arg();
			(void) force_number(arg);

			if (out_of_range(arg))
				goto out_of_range;
#ifdef HAVE_MPFR
			if (is_mpg_integer(arg)) {
mpz0:
				zi = arg->mpg_i;

				if (cs1 != 'd' && cs1 != 'i') {
					if (mpz_sgn(zi) <= 0) {
						/*
						 * Negative value or 0 requires special handling.
						 * Unlike MPFR, GMP does not allow conversion
						 * to (u)intmax_t. So we first convert GMP type to
						 * a MPFR type.
						 */
						mf = mpz2mpfr(zi);
						goto mpf1;
					}
					signchar = '\0';	/* Don't print '+' */
				}

				/* See comments above about when to fill with zeros */
				zero_flag = (! lj
						    && ((zero_flag && ! have_prec)
							 || (fw == 0 && have_prec)));

 				fmt_type = have_prec ? MP_INT_WITH_PREC : MP_INT_WITHOUT_PREC;
				goto fmt0;

			} else if (is_mpg_float(arg)) {
mpf0:
				mf = arg->mpg_numbr;
				if (! mpfr_number_p(mf)) {
					/* inf or NaN */
					cs1 = 'g';
					fmt_type = MP_FLOAT;
					goto fmt1;
				}

				if (cs1 != 'd' && cs1 != 'i') {
mpf1:
					/*
					 * The output of printf("%#.0x", 0) is 0 instead of 0x, hence <= in
					 * the comparison below.
					 */
					if (mpfr_sgn(mf) <= 0) {
						if (! mpfr_fits_intmax_p(mf, ROUND_MODE)) {
							/* -ve number is too large */
							cs1 = 'g';
							fmt_type = MP_FLOAT;
							goto fmt1;
						}

						tmpval = uval = (uintmax_t) mpfr_get_sj(mf, ROUND_MODE);
						if (! alt && have_prec && prec == 0 && tmpval == 0)
							goto pr_tail;	/* printf("%.0x", 0) is no characters */
						goto int0;
					}
					signchar = '\0';	/* Don't print '+' */
				}

				/* See comments above about when to fill with zeros */
				zero_flag = (! lj
						    && ((zero_flag && ! have_prec)
							 || (fw == 0 && have_prec)));

				(void) mpfr_get_z(mpzval, mf, MPFR_RNDZ);	/* convert to GMP integer */
 				fmt_type = have_prec ? MP_INT_WITH_PREC : MP_INT_WITHOUT_PREC;
				zi = mpzval;
				goto fmt0;
			} else
#endif
				tmpval = arg->numbr;

			/*
			 * ``The result of converting a zero value with a
			 * precision of zero is no characters.''
			 *
			 * If I remember the ANSI C standard, though,
			 * it says that for octal conversions
			 * the precision is artificially increased
			 * to add an extra 0 if # is supplied.
			 * Indeed, in C,
			 * 	printf("%#.0o\n", 0);
			 * prints a single 0.
			 */
			if (! alt && have_prec && prec == 0 && tmpval == 0)
				goto pr_tail;

			if (tmpval < 0) {
				uval = (uintmax_t) (intmax_t) tmpval;
				if ((AWKNUM)(intmax_t)uval != double_to_int(tmpval))
					goto out_of_range;
			} else {
				uval = (uintmax_t) tmpval;
				if ((AWKNUM)uval != double_to_int(tmpval))
					goto out_of_range;
			}
#ifdef HAVE_MPFR
	int0:
#endif
#if defined(HAVE_LOCALE_H)
			quote_flag = (quote_flag && loc.thousands_sep[0] != 0);
#endif
			/*
			 * When to fill with zeroes is of course not simple.
			 * First: No zero fill if left-justifying.
			 * Next: There seem to be two cases:
			 * 	A '0' without a precision, e.g. %06d
			 * 	A precision with no field width, e.g. %.10d
			 * Any other case, we don't want to fill with zeroes.
			 */
			if (! lj
			    && ((zero_flag && ! have_prec)
				 || (fw == 0 && have_prec)))
				fill = zero_string;
			ii = jj = 0;
			do {
				PREPEND(chbuf[uval % base]);
				uval /= base;
#if defined(HAVE_LOCALE_H)
				if (base == 10 && quote_flag && loc.grouping[ii] && ++jj == loc.grouping[ii]) {
					if (uval) {	/* only add if more digits coming */
						int k;
						const char *ts = loc.thousands_sep;

						for (k = strlen(ts) - 1; k >= 0; k--) {
							PREPEND(ts[k]);
						}
					}
					if (loc.grouping[ii+1] == 0)
						jj = 0;     /* keep using current val in loc.grouping[ii] */
					else if (loc.grouping[ii+1] == CHAR_MAX)
						quote_flag = false;
					else {
						ii++;
						jj = 0;
					}
				}
#endif
			} while (uval > 0);

			/* add more output digits to match the precision */
			if (have_prec) {
				while (cend - cp < prec)
					PREPEND('0');
			}

			if (alt && tmpval != 0) {
				if (base == 16) {
					PREPEND(cs1);
					PREPEND('0');
					if (fill != sp) {
						bchunk(cp, 2);
						cp += 2;
						fw -= 2;
					}
				} else if (base == 8)
					PREPEND('0');
			}
			base = 0;
			if (prec > fw)
				fw = prec;
			prec = cend - cp;
	pr_tail:
			if (! lj) {
				while (fw > prec) {
			    		bchunk_one(fill);
					fw--;
				}
			}
			copy_count = prec;
			if (fw == 0 && ! have_prec)
				;
			else if (gawk_mb_cur_max > 1) {
				if (cs1 == 's') {
					assert(cp == arg->stptr || cp == cpbuf);
					copy_count = mbc_byte_count(arg->stptr, prec);
				}
				/* prec was set by code for %c */
				/* else
					copy_count = prec; */
			}
			bchunk(cp, copy_count);
			while (fw > prec) {
				bchunk_one(fill);
				fw--;
			}
			s0 = s1;
			break;

     out_of_range:
			/*
			 * out of range - emergency use of %g format,
			 * or format NaN and INF values.
			 */
			nan_inf_val = format_nan_inf(arg, cs1);
			if (do_posix || magic_posix_flag || nan_inf_val == NULL) {
				if (do_lint && ! do_posix && ! magic_posix_flag)
					lintwarn(_("[s]printf: value %g is out of range for `%%%c' format"),
								(double) tmpval, cs1);
				tmpval = arg->numbr;
				if (strchr("aAeEfFgG", cs1) == NULL)
					cs1 = 'g';
				goto fmt1;
			} else {
				if (do_lint)
					lintwarn(_("[s]printf: value %s is out of range for `%%%c' format"),
								nan_inf_val, cs1);
				bchunk(nan_inf_val, strlen(nan_inf_val));
				s0 = s1;
				break;
			}

		case 'F':
#if ! defined(PRINTF_HAS_F_FORMAT) || PRINTF_HAS_F_FORMAT != 1
			cs1 = 'f';
			/* FALL THROUGH */
#endif
		case 'g':
		case 'G':
		case 'e':
		case 'f':
		case 'E':
#if defined(PRINTF_HAS_A_FORMAT) && PRINTF_HAS_A_FORMAT == 1
		case 'A':
		case 'a':
		{
			static bool warned = false;

			if (do_lint && tolower(cs1) == 'a' && ! warned) {
				warned = true;
				lintwarn(_("%%%c format is POSIX standard but not portable to other awks"), cs1);
			}
		}
#endif
			need_format = false;
			parse_next_arg();
			(void) force_number(arg);

			if (! is_mpg_number(arg))
				tmpval = arg->numbr;
#ifdef HAVE_MPFR
			else if (is_mpg_float(arg)) {
				mf = arg->mpg_numbr;
				fmt_type = MP_FLOAT;
			} else {
				/* arbitrary-precision integer, convert to MPFR float */
				assert(mf == NULL);
				mf = mpz2mpfr(arg->mpg_i);
				fmt_type = MP_FLOAT;
			}
#endif
			if (out_of_range(arg))
				goto out_of_range;

     fmt1:
			if (! have_prec)
				prec = DEFAULT_G_PRECISION;
#ifdef HAVE_MPFR
     fmt0:
#endif
			chksize(fw + prec + 11);	/* 11 == slop */
			cp = cpbuf;
			*cp++ = '%';
			if (lj)
				*cp++ = '-';
			if (signchar)
				*cp++ = signchar;
			if (alt)
				*cp++ = '#';
			if (zero_flag)
				*cp++ = '0';
			if (quote_flag)
				*cp++ = '\'';

#if defined(LC_NUMERIC)
			if (quote_flag && ! use_lc_numeric)
				setlocale(LC_NUMERIC, "");
#endif

			bool need_to_add_thousands = false;
			switch (fmt_type) {
#ifdef HAVE_MPFR
			case MP_INT_WITH_PREC:
				sprintf(cp, "*.*Z%c", cs1);
				while ((nc = mpfr_snprintf(obufout, ofre, cpbuf,
					     (int) fw, (int) prec, zi)) >= (int) ofre)
					chksize(nc)
				need_to_add_thousands = true;
				break;
			case MP_INT_WITHOUT_PREC:
				sprintf(cp, "*Z%c", cs1);
				while ((nc = mpfr_snprintf(obufout, ofre, cpbuf,
					     (int) fw, zi)) >= (int) ofre)
					chksize(nc)
				need_to_add_thousands = true;
				break;
			case MP_FLOAT:
				sprintf(cp, "*.*R*%c", cs1);
				while ((nc = mpfr_snprintf(obufout, ofre, cpbuf,
					     (int) fw, (int) prec, ROUND_MODE, mf)) >= (int) ofre)
					chksize(nc)
				break;
#endif
			default:
				if (have_prec || tolower(cs1) != 'a') {
					sprintf(cp, "*.*%c", cs1);
					while ((nc = snprintf(obufout, ofre, cpbuf,
						     (int) fw, (int) prec,
						     (double) tmpval)) >= (int) ofre)
						chksize(nc)
				} else {
					// For %a and %A, use the default precision if it
					// wasn't supplied by the user.
					sprintf(cp, "*%c", cs1);
					while ((nc = snprintf(obufout, ofre, cpbuf,
						     (int) fw,
						     (double) tmpval)) >= (int) ofre)
						chksize(nc)
				}
			}

#if defined(LC_NUMERIC)
			if (quote_flag && ! use_lc_numeric)
				setlocale(LC_NUMERIC, "C");
#endif
			len = strlen(obufout);
			if (quote_flag && need_to_add_thousands) {
				const char *new_text = add_thousands(obufout, & loc);

				len = strlen(new_text);
				chksize(len)
				strcpy(obufout, new_text);
				free((void *) new_text);
			}

			ofre -= len;
			obufout += len;
			s0 = s1;
			break;
		default:
			if (do_lint && is_alpha(cs1))
				lintwarn(_("ignoring unknown format specifier character `%c': no argument converted"), cs1);
			break;
		}
		if (toofew) {
			msg("%s\n\t`%s'\n\t%*s%s",
			      _("fatal: not enough arguments to satisfy format string"),
			      fmt_string, (int) (s1 - fmt_string - 1), "",
			      _("^ ran out for this one"));
			goto out;
		}
	}
	if (do_lint) {
		if (need_format)
			lintwarn(
			_("[s]printf: format specifier does not have control letter"));
		if (cur_arg < num_args)
			lintwarn(
			_("too many arguments supplied for format string"));
	}
	bchunk(s0, s1 - s0);
	olen_final = obufout - obuf;
#define GIVE_BACK_SIZE (INITIAL_OUT_SIZE * 2)
	if (ofre > GIVE_BACK_SIZE)
		erealloc(obuf, char *, olen_final + 1, "format_tree");
	r = make_str_node(obuf, olen_final, ALREADY_MALLOCED);
	obuf = NULL;
out:
	{
		size_t k;
		size_t count = sizeof(cpbufs)/sizeof(cpbufs[0]);
		for (k = 0; k < count; k++) {
			if (cpbufs[k].buf != cpbufs[k].stackbuf)
				efree(cpbufs[k].buf);
		}
		if (obuf != NULL)
			efree(obuf);
	}

	if (r == NULL)
		gawk_exit(EXIT_FATAL);
	return r;
}


/* printf_common --- common code for sprintf and printf */

static NODE *
printf_common(int nargs)
{
	int i;
	NODE *r, *tmp;

	assert(nargs > 0 && nargs <= max_args);
	for (i = 1; i <= nargs; i++) {
		tmp = args_array[nargs - i] = POP();
		if (tmp->type == Node_var_array) {
			while (--i > 0)
				DEREF(args_array[nargs - i]);
			fatal(_("attempt to use array `%s' in a scalar context"), array_vname(tmp));
		}
	}

	args_array[0] = force_string(args_array[0]);
	if (do_lint && (fixtype(args_array[0])->flags & STRING) == 0)
		lintwarn(_("%s: received non-string format string argument"), "printf/sprintf");
	r = format_tree(args_array[0]->stptr, args_array[0]->stlen, args_array, nargs);
	for (i = 0; i < nargs; i++)
		DEREF(args_array[i]);
	return r;
}

/* do_sprintf --- perform sprintf */

NODE *
do_sprintf(int nargs)
{
	NODE *r;

	if (nargs == 0)
		fatal(_("sprintf: no arguments"));

	r = printf_common(nargs);
	if (r == NULL)
		gawk_exit(EXIT_FATAL);
	return r;
}


/* do_printf --- perform printf, including redirection */

void
do_printf(int nargs, int redirtype)
{
	FILE *fp = NULL;
	NODE *tmp;
	struct redirect *rp = NULL;
	int errflg = 0;
	NODE *redir_exp = NULL;

	if (nargs == 0) {
		if (do_traditional) {
			if (do_lint)
				lintwarn(_("printf: no arguments"));
			if (redirtype != 0) {
				redir_exp = TOP();
				if (redir_exp->type != Node_val)
					fatal(_("attempt to use array `%s' in a scalar context"), array_vname(redir_exp));
				rp = redirect(redir_exp, redirtype, & errflg, true);
				DEREF(redir_exp);
				decr_sp();
			}
			return;	/* bwk accepts it silently */
		}
		fatal(_("printf: no arguments"));
	}

	if (redirtype != 0) {
		redir_exp = PEEK(nargs);
		if (redir_exp->type != Node_val)
			fatal(_("attempt to use array `%s' in a scalar context"), array_vname(redir_exp));
		rp = redirect(redir_exp, redirtype, & errflg, true);
		if (rp != NULL) {
			if ((rp->flag & RED_TWOWAY) != 0 && rp->output.fp == NULL) {
				if (is_non_fatal_redirect(redir_exp->stptr, redir_exp->stlen)) {
					update_ERRNO_int(EBADF);
					return;
				}
				(void) close_rp(rp, CLOSE_ALL);
				fatal(_("printf: attempt to write to closed write end of two-way pipe"));
			}
			fp = rp->output.fp;
		}
		else if (errflg) {
			update_ERRNO_int(errflg);
			return;
		}
	} else if (do_debug)	/* only the debugger can change the default output */
		fp = output_fp;
	else
		fp = stdout;

	tmp = printf_common(nargs);
	if (redir_exp != NULL) {
		DEREF(redir_exp);
		decr_sp();
	}
	if (tmp != NULL) {
		if (fp == NULL) {
			DEREF(tmp);
			return;
		}
		efwrite(tmp->stptr, sizeof(char), tmp->stlen, fp, "printf", rp, true);
		if (rp != NULL && (rp->flag & RED_TWOWAY) != 0)
			rp->output.gawk_fflush(rp->output.fp, rp->output.opaque);
		DEREF(tmp);
	} else
		gawk_exit(EXIT_FATAL);
}

/* mbc_byte_count --- return number of bytes for corresponding numchars multibyte characters */

static size_t
mbc_byte_count(const char *ptr, size_t numchars)
{
	mbstate_t cur_state;
	size_t sum = 0;
	int mb_len;

	memset(& cur_state, 0, sizeof(cur_state));

	assert(gawk_mb_cur_max > 1);
	mb_len = mbrlen(ptr, numchars * gawk_mb_cur_max, &cur_state);
	if (mb_len <= 0)
		return numchars;	/* no valid m.b. char */

	for (; numchars > 0; numchars--) {
		mb_len = mbrlen(ptr, numchars * gawk_mb_cur_max, &cur_state);
		if (mb_len <= 0)
			break;
		sum += mb_len;
		ptr += mb_len;
	}

	return sum;
}

/* mbc_char_count --- return number of m.b. chars in string, up to numbytes bytes */

static size_t
mbc_char_count(const char *ptr, size_t numbytes)
{
	mbstate_t cur_state;
	size_t sum = 0;
	int mb_len;

	if (gawk_mb_cur_max == 1)
		return numbytes;

	memset(& cur_state, 0, sizeof(cur_state));

	mb_len = mbrlen(ptr, numbytes, &cur_state);
	if (mb_len <= 0)
		return numbytes;	/* no valid m.b. char */

	while (numbytes > 0) {
		mb_len = mbrlen(ptr, numbytes, &cur_state);
		if (mb_len <= 0)
			break;
		sum++;
		ptr += mb_len;
		numbytes -= mb_len;
	}

	return sum;
}

/* out_of_range --- return true if a value is out of range */

bool
out_of_range(NODE *n)
{
#ifdef HAVE_MPFR
	if (is_mpg_integer(n))
		return false;
	else if (is_mpg_float(n))
		return (! mpfr_number_p(n->mpg_numbr));
	else
#endif
		return (isnan(n->numbr) || isinf(n->numbr));
}

/* format_nan_inf --- format NaN and INF values */

char *
format_nan_inf(NODE *n, char format)
{
	static char buf[100];
	double val = n->numbr;

#ifdef HAVE_MPFR
	if (is_mpg_integer(n))
		return NULL;
	else if (is_mpg_float(n)) {
		if (mpfr_nan_p(n->mpg_numbr)) {
			strcpy(buf, mpfr_signbit(n->mpg_numbr) != 0 ? "-nan" : "+nan");

			goto fmt;
		} else if (mpfr_inf_p(n->mpg_numbr)) {
			strcpy(buf, mpfr_signbit(n->mpg_numbr) ? "-inf" : "+inf");

			goto fmt;
		} else
			return NULL;
	}
	/* else
		fallthrough */
#endif

	if (isnan(val)) {
		strcpy(buf, signbit(val) != 0 ? "-nan" : "+nan");

		// fall through to end
	} else if (isinf(val)) {
		strcpy(buf, val < 0 ? "-inf" : "+inf");

		// fall through to end
	} else
		return NULL;

#ifdef HAVE_MPFR
fmt:
#endif
	if (isupper(format)) {
		int i;

		for (i = 0; buf[i] != '\0'; i++)
			buf[i] = toupper(buf[i]);
	}
	return buf;
}



/* reverse --- reverse the contents of a string in place */

static void
reverse(char *str)
{
	int i, j;
	char tmp;

	for (i = 0, j = strlen(str) - 1; j > i; i++, j--) {
		tmp = str[i];
		str[i] = str[j];
		str[j] = tmp;
	}
}

/* add_thousands --- add the thousands separator. Needed for MPFR %d format */

/*
 * Copy the source string into the destination string, backwards,
 * adding the thousands separator at the right points. Then reverse
 * the string when done. This gives us much cleaner code than trying
 * to work through the string backwards. (We tried it, it was yucky.)
 */

static const char *
add_thousands(const char *original, struct lconv *loc)
{
	size_t orig_len = strlen(original);
	size_t new_len = orig_len + (orig_len * strlen(loc->thousands_sep)) + 1; 	// worst case
	char *newbuf;
	char decimal_point = '\0';
	const char *dec = NULL;
	const char *src;
	char *dest;

	emalloc(newbuf, char *, new_len, "add_thousands");
	memset(newbuf, '\0', new_len);

	src = original + strlen(original) - 1;
	dest = newbuf;

	if (loc->decimal_point[0] != '\0') {
		decimal_point = loc->decimal_point[0];
		if ((dec = strchr(original, decimal_point)) != NULL) {
			while (src >= dec)
				*dest++ = *src--;
		}
	}


	int ii = 0;
	int jj = 0;
	do {
		*dest++ = *src--;
		if (loc->grouping[ii] && ++jj == loc->grouping[ii]) {
			if (src >= original) {	/* only add if more digits coming */
				const char *ts = loc->thousands_sep;
				int k;

				for (k = strlen(ts) - 1; k >= 0; k--)
					*dest++ = ts[k];
			}
			if (loc->grouping[ii+1] == 0)
				jj = 0;		/* keep using current val in loc.grouping[ii] */
			else if (loc->grouping[ii+1] == CHAR_MAX) {
				// copy in the rest and be done
				while (src >= original)
					*dest++ = *src--;
				break;
			} else {
				ii++;
				jj = 0;
			}
		}
	} while (src >= original);

	*dest++ = '\0';
	reverse(newbuf);

	return newbuf;
}
