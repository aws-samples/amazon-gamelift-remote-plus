// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include <limits.h>
#include "misc.h"
#include "malloc.h"
#include <string.h>


//177
//void modalfatalbox(char *fmt, ...) {}
#define modalfatalbox printf

/* 281----------------------------------------------------------------------
* String handling routines.
*/

char *dupstr(const char *s)
{
	char *p = NULL;
	if (s) {
		int len = strlen(s);
		p = snewn(len + 1, char);
		strcpy(p, s);
	}
	return p;
}

//338
int toint(unsigned u)
{
	/*
	* Convert an unsigned to an int, without running into the
	* undefined behaviour which happens by the strict C standard if
	* the value overflows. You'd hope that sensible compilers would
	* do the sensible thing in response to a cast, but actually I
	* don't trust modern compilers not to do silly things like
	* assuming that _obviously_ you wouldn't have caused an overflow
	* and so they can elide an 'if (i < 0)' test immediately after
	* the cast.
	*
	* Sensible compilers ought of course to optimise this entire
	* function into 'just return the input value'!
	*/
	if (u <= (unsigned)INT_MAX)
		return (int)u;
	else if (u >= (unsigned)INT_MIN)   /* wrap in cast _to_ unsigned is OK */
		return INT_MIN + (int)(u - (unsigned)INT_MIN);
	else
		return INT_MIN; /* fallback; should never occur on binary machines */
}

/*452
* Read an entire line of text from a file. Return a buffer
* malloced to be as big as necessary (caller must free).
*/
char *fgetline(FILE *fp)
{
	char *ret = snewn(512, char);
	int size = 512, len = 0;
	while (fgets(ret + len, size - len, fp)) {
		len += strlen(ret + len);
		if (len > 0 && ret[len - 1] == '\n')
			break;		       /* got a newline, we're done */
		size = len + 512;
		ret = sresize(ret, size, char);
	}
	if (len == 0) {		       /* first fgets returned NULL */
		sfree(ret);
		return NULL;
	}
	ret[len] = '\0';
	return ret;
}

/* ----------------------------------------------------------------------
* Core base64 encoding and decoding routines.
*/

void base64_encode_atom(unsigned char *data, int n, char *out)
{
	static const char base64_chars[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned word;

	word = data[0] << 16;
	if (n > 1)
		word |= data[1] << 8;
	if (n > 2)
		word |= data[2];
	out[0] = base64_chars[(word >> 18) & 0x3F];
	out[1] = base64_chars[(word >> 12) & 0x3F];
	if (n > 1)
		out[2] = base64_chars[(word >> 6) & 0x3F];
	else
		out[2] = '=';
	if (n > 2)
		out[3] = base64_chars[word & 0x3F];
	else
		out[3] = '=';
}

int base64_decode_atom(char *atom, unsigned char *out)
{
	int vals[4];
	int i, v, len;
	unsigned word;
	char c;

	for (i = 0; i < 4; i++) {
		c = atom[i];
		if (c >= 'A' && c <= 'Z')
			v = c - 'A';
		else if (c >= 'a' && c <= 'z')
			v = c - 'a' + 26;
		else if (c >= '0' && c <= '9')
			v = c - '0' + 52;
		else if (c == '+')
			v = 62;
		else if (c == '/')
			v = 63;
		else if (c == '=')
			v = -1;
		else
			return 0;		       /* invalid atom */
		vals[i] = v;
	}

	if (vals[0] == -1 || vals[1] == -1)
		return 0;
	if (vals[2] == -1 && vals[3] != -1)
		return 0;

	if (vals[3] != -1)
		len = 3;
	else if (vals[2] != -1)
		len = 2;
	else
		len = 1;

	word = ((vals[0] << 18) |
		(vals[1] << 12) | ((vals[2] & 0x3F) << 6) | (vals[3] & 0x3F));
	out[0] = (word >> 16) & 0xFF;
	if (len > 1)
		out[1] = (word >> 8) & 0xFF;
	if (len > 2)
		out[2] = word & 0xFF;
	return len;
}


//678
/* ----------------------------------------------------------------------
* My own versions of malloc, realloc and free. Because I want
* malloc and realloc to bomb out and exit the program if they run
* out of memory, realloc to reliably call malloc if passed a NULL
* pointer, and free to reliably do nothing if passed a NULL
* pointer. We can also put trace printouts in, if we need to; and
* we can also replace the allocator with an ElectricFence-like
* one.
*/

#ifdef MINEFIELD
void *minefield_c_malloc(size_t size);
void minefield_c_free(void *p);
void *minefield_c_realloc(void *p, size_t size);
#endif

#ifdef MALLOC_LOG
static FILE *fp = NULL;

static char *mlog_file = NULL;
static int mlog_line = 0;

void mlog(char *file, int line)
{
	mlog_file = file;
	mlog_line = line;
	if (!fp) {
		fp = fopen("putty_mem.log", "w");
		setvbuf(fp, NULL, _IONBF, BUFSIZ);
	}
	if (fp)
		fprintf(fp, "%s:%d: ", file, line);
}
#endif

void *safemalloc(size_t n, size_t size)
{
	void *p;

	if (n > INT_MAX / size) {
		p = NULL;
	}
	else {
		size *= n;
		if (size == 0) size = 1;
#ifdef MINEFIELD
		p = minefield_c_malloc(size);
#else
		p = malloc(size);
#endif
	}

	if (!p) {
		char str[200];
#ifdef MALLOC_LOG
		sprintf(str, "Out of memory! (%s:%d, size=%d)",
			mlog_file, mlog_line, size);
		fprintf(fp, "*** %s\n", str);
		fclose(fp);
#else
		strcpy(str, "Out of memory!");
#endif
		modalfatalbox("%s", str);
	}
#ifdef MALLOC_LOG
	if (fp)
		fprintf(fp, "malloc(%d) returns %p\n", size, p);
#endif
	return p;
}

void *saferealloc(void *ptr, size_t n, size_t size)
{
	void *p;

	if (n > INT_MAX / size) {
		p = NULL;
	}
	else {
		size *= n;
		if (!ptr) {
#ifdef MINEFIELD
			p = minefield_c_malloc(size);
#else
			p = malloc(size);
#endif
		}
		else {
#ifdef MINEFIELD
			p = minefield_c_realloc(ptr, size);
#else
			p = realloc(ptr, size);
#endif
		}
	}

	if (!p) {
		char str[200];
#ifdef MALLOC_LOG
		sprintf(str, "Out of memory! (%s:%d, size=%d)",
			mlog_file, mlog_line, size);
		fprintf(fp, "*** %s\n", str);
		fclose(fp);
#else
		strcpy(str, "Out of memory!");
#endif
		modalfatalbox("%s", str);
	}
#ifdef MALLOC_LOG
	if (fp)
		fprintf(fp, "realloc(%p,%d) returns %p\n", ptr, size, p);
#endif
	return p;
}

void safefree(void *ptr)
{
	if (ptr) {
#ifdef MALLOC_LOG
		if (fp)
			fprintf(fp, "free(%p)\n", ptr);
#endif
#ifdef MINEFIELD
		minefield_c_free(ptr);
#else
		free(ptr);
#endif
	}
#ifdef MALLOC_LOG
	else if (fp)
		fprintf(fp, "freeing null pointer - no action taken\n");
#endif
}

#ifndef PLATFORM_HAS_SMEMCLR
/* 887
* Securely wipe memory.
*
* The actual wiping is no different from what memset would do: the
* point of 'securely' is to try to be sure over-clever compilers
* won't optimise away memsets on variables that are about to be freed
* or go out of scope. See
* https://buildsecurityin.us-cert.gov/bsi-rules/home/g1/771-BSI.html
*
* Some platforms (e.g. Windows) may provide their own version of this
* function.
*/
void smemclr(void *b, size_t n) {
	volatile char *vp;

	if (b && n > 0) {
		/*
		* Zero out the memory.
		*/
		memset(b, 0, n);

		/*
		* Perform a volatile access to the object, forcing the
		* compiler to admit that the previous memset was important.
		*
		* This while loop should in practice run for zero iterations
		* (since we know we just zeroed the object out), but in
		* theory (as far as the compiler knows) it might range over
		* the whole object. (If we had just written, say, '*vp =
		* *vp;', a compiler could in principle have 'helpfully'
		* optimised the memset into only zeroing out the first byte.
		* This should be robust.)
		*/
		vp = (volatile char*)b;
		while (*vp) vp++;
	}
}
#endif

// Windows implementation
//#include "windows.h"
//void smemclr(void *b, size_t n) {
//	if (b && n > 0)
//		SecureZeroMemory(b, n);
//}

// 1023
int smemeq(const void *av, const void *bv, size_t len)
{
	const unsigned char *a = (const unsigned char *)av;
	const unsigned char *b = (const unsigned char *)bv;
	unsigned val = 0;

	while (len-- > 0) {
		val |= *a++ ^ *b++;
	}
	/* Now val is 0 iff we want to return 1, and in the range
	* 0x01..0xFF iff we want to return 0. So subtracting from 0x100
	* will clear bit 8 iff we want to return 0, and leave it set iff
	* we want to return 1, so then we can just shift down. */
	return (0x100 - val) >> 8;
}

int strstartswith(const char *s, const char *t)
{
	return !memcmp(s, t, strlen(t));
}

int strendswith(const char *s, const char *t)
{
	size_t slen = strlen(s), tlen = strlen(t);
	return slen >= tlen && !strcmp(s + (slen - tlen), t);
}
