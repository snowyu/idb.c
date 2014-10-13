/*
 * =====================================================================================
 *
 *       Filename:  isdk_string.c
 *
 *    Description:  the string funcs
 *
 *        Version:  1.0
 *        Created:  2013/03/08
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Riceball LEE(snowyu.lee@gmail.com)
 *        Company:
 *
 * =====================================================================================
 */

#include <assert.h>
#include <string.h>
//#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#include "isdk_string.h"

/* Compare two strings with case insensitive evaluation. */
int stricmp(const char *astr, const char *bstr)
{
  assert(astr && bstr);
  while(*astr != '\0'){
    if(*bstr == '\0') return 1;
    int ac = (*astr >= 'A' && *astr <= 'Z') ? *astr + ('a' - 'A') : *(unsigned char *)astr;
    int bc = (*bstr >= 'A' && *bstr <= 'Z') ? *bstr + ('a' - 'A') : *(unsigned char *)bstr;
    if(ac != bc) return ac - bc;
    astr++;
    bstr++;
  }
  return (*bstr == '\0') ? 0 : -1;
}

/* Check whether a string begins with a key. */
bool strBeginWith(const char *str, const char *key)
{
  assert(str && key);
  while(*key != '\0'){
    if(*str != *key || *str == '\0') return false;
    key++;
    str++;
  }
  return true;
}

/* Check whether a string begins with a key with case insensitive evaluation. */
bool striBeginWith(const char *str, const char *key)
{
  assert(str && key);
  while(*key != '\0'){
    if(*str == '\0') return false;
    int sc = *str;
    if(sc >= 'A' && sc <= 'Z') sc += 'a' - 'A';
    int kc = *key;
    if(kc >= 'A' && kc <= 'Z') kc += 'a' - 'A';
    if(sc != kc) return false;
    key++;
    str++;
  }
  return true;
}

/* Check whether a string ends with a key. */
bool strEndWith(const char *str, const char *key)
{
  assert(str && key);
  int slen = strlen(str);
  int klen = strlen(key);
  for(int i = 1; i <= klen; i++){
    if(i > slen || str[slen-i] != key[klen-i]) return false;
  }
  return true;
}

/* Check whether a string ends with a key with case insensitive evaluation. */
bool striEndWith(const char *str, const char *key)
{
  assert(str && key);
  int slen = strlen(str);
  int klen = strlen(key);
  for(int i = 1; i <= klen; i++){
    if(i > slen) return false;
    int sc = str[slen-i];
    if(sc >= 'A' && sc <= 'Z') sc += 'a' - 'A';
    int kc = key[klen-i];
    if(kc >= 'A' && kc <= 'Z') kc += 'a' - 'A';
    if(sc != kc) return false;
  }
  return true;
}

/* Convert the letters of a string into upper case. */
char *strToUpperCase(char *str)
{
  assert(str);
  char *wp = str;
  while(*wp != '\0'){
    if(*wp >= 'a' && *wp <= 'z') *wp -= 'a' - 'A';
    wp++;
  }
  return str;
}

/* Convert the letters of a string into lower case. */
char *strToLowerCase(char *str)
{
  assert(str);
  char *wp = str;
  while(*wp != '\0'){
    if(*wp >= 'A' && *wp <= 'Z') *wp += 'a' - 'A';
    wp++;
  }
  return str;
}

/* trim space characters at head and tail of a string. */
char *strTrim(char *str)
{
  assert(str);
  const char *rp = str;
  char *wp = str;
  //trim head
  bool head = true;
  while(*rp != '\0'){
    if(*rp > '\0' && *rp <= ' '){
      if(!head) *(wp++) = *rp;
    } else {
      *(wp++) = *rp;
      head = false;
    }
    rp++;
  }
  *wp = '\0';
  //trim tail:
  while(wp > str && wp[-1] > '\0' && wp[-1] <= ' '){
    *(--wp) = '\0';
  }
  /*
  for(wp--; wp >= str; wp--){
    if(*wp > 0 && *wp <= ' '){
      *wp = '\0';
    } else {
      break;
    }
  }
  */
  return str;
}

/* Substitute characters in a string. 
 * 
 * if find the char in the searchCharset will replace it within the replacedCharset
 * eg, strReplaceChars("hello", "eol", "sco") => "hsooc" 
 *
 * */
char *strReplaceChars(char *str, const char *searchCharset, const char *replacedCharset)
{
  assert(str && searchCharset && replacedCharset);
  int slen = strlen(replacedCharset);
  char *wp = str;
  for(int i = 0; str[i] != '\0'; i++){
    const char *p = strchr(searchCharset, str[i]);
    if(p){
      int idx = p - searchCharset;
      if(idx < slen) *(wp++) = replacedCharset[idx];
    } else {
      *(wp++) = str[i];
    }
  }
  *wp = '\0';
  return str;
}

/* Count the number of characters in a string of UTF-8. */
int utf8StrLen(const char *str)
{
  assert(str);
  const unsigned char *rp = (unsigned char *)str;
  int cnt = 0;
  while(*rp != '\0'){
    if((*rp & 0x80) == 0x00 || (*rp & 0xe0) == 0xc0 ||
       (*rp & 0xf0) == 0xe0 || (*rp & 0xf8) == 0xf0) cnt++;
    rp++;
  }
  return cnt;
}

/* Convert a string to an int64. */
int64_t strToInt64(const char *str)
{
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  int sign = 1;
  int64_t num = 0;
  if(*str == '-'){
    str++;
    sign = -1;
  } else if(*str == '+'){
    str++;
  }
  while(*str != '\0'){
    if(*str < '0' || *str > '9') break;
    num = num * 10 + *str - '0';
    str++;
  }
  return num * sign;
}

/* Convert a string with a metric prefix to an integer. 
 *
 * eg, "1024K", "500G"
 *
 *
 * */
int64_t strToInt64x(const char *str)
{
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  int sign = 1;
  if(*str == '-'){
    str++;
    sign = -1;
  } else if(*str == '+'){
    str++;
  }
  long double num = 0;
  while(*str != '\0'){
    if(*str < '0' || *str > '9') break;
    num = num * 10 + *str - '0';
    str++;
  }
  if(*str == '.'){
    str++;
    long double base = 10;
    while(*str != '\0'){
      if(*str < '0' || *str > '9') break;
      num += (*str - '0') / base;
      str++;
      base *= 10;
    }
  }
  num *= sign;
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  if(*str == 'k' || *str == 'K'){
    num *= 1LL << 10;
  } else if(*str == 'm' || *str == 'M'){
    num *= 1LL << 20;
  } else if(*str == 'g' || *str == 'G'){
    num *= 1LL << 30;
  } else if(*str == 't' || *str == 'T'){
    num *= 1LL << 40;
  } else if(*str == 'p' || *str == 'P'){
    num *= 1LL << 50;
  } else if(*str == 'e' || *str == 'E'){
    num *= 1LL << 60;
  }
  if(num > INT64_MAX) return INT64_MAX;
  if(num < INT64_MIN) return INT64_MIN;
  return num;
}

//#define LONG_DOUBLE_MAX_CHARS   16                // maximum number of columns of the long double
/* Convert a string to a real number. */
/*
double tcatof(const char *str){
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  int sign = 1;
  if(*str == '-'){
    str++;
    sign = -1;
  } else if(*str == '+'){
    str++;
  }
  if(striBeginWith(str, "inf")) return HUGE_VAL * sign;
  if(striBeginWith(str, "nan")) return nan("");
  long double num = 0;
  int col = 0;
  while(*str != '\0'){
    if(*str < '0' || *str > '9') break;
    num = num * 10 + *str - '0';
    str++;
    if(num > 0) col++;
  }
  if(*str == '.'){
    str++;
    long double fract = 0.0;
    long double base = 10;
    while(col < LONG_DOUBLE_MAX_CHARS && *str != '\0'){
      if(*str < '0' || *str > '9') break;
      fract += (*str - '0') / base;
      str++;
      col++;
      base *= 10;
    }
    num += fract;
  }
  if(*str == 'e' || *str == 'E'){
    str++;
    num *= pow(10, strToInt64(str));
  }
  return num * sign;
}
*/

/* 
   Description:	Replaces in the string str all the occurrences of the source string old with the destination string new. The lengths of the strings old and new may differ. The string new may be of any length, but the string "old" must be of non-zero length - the penalty for providing an empty string for the "old" parameter is an infinite loop. In addition, none of the three parameters may be NULL.
   Returns:	The post-replacement string, or NULL if memory for the new string could not be allocated. Does not modify the original string. The memory for the returned post-replacement string may be deallocated with the standard library function free() when it is no longer required.
   Dependencies:	For this function to compile, you will need to also #include the following files: <string.h>, <stdlib.h> and <stddef.h>.
   Licence:	Public domain. You may use this code in any way you see fit, optionally crediting its author (me, Laird Shaw, with assistance and inspiration from comp.lang.c and stackoverflow).

 * */
char *replace_str2(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t oldlen = strlen(old);
	size_t count, retlen, newlen = strlen(new);
	int samesize = (oldlen == newlen);

	if (!samesize) {
		for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
			count++;
		/* This is undefined if p - str > PTRDIFF_MAX */
		retlen = p - str + strlen(p) + count * (newlen - oldlen);
	} else
		retlen = strlen(str);

	if ((ret = malloc(retlen + 1)) == NULL)
		return NULL;

	r = ret, p = str;
	while (1) {
		/* If the old and new strings are different lengths - in other
		 * words we have already iterated through with strstr above,
		 * and thus we know how many times we need to call it - then we
		 * can avoid the final (potentially lengthy) call to strstr,
		 * which we already know is going to return NULL, by
		 * decrementing and checking count.
		 */
		if (!samesize && !count--)
			break;
		/* Otherwise i.e. when the old and new strings are the same
		 * length, and we don't know how many times to call strstr,
		 * we must check for a NULL return here (we check it in any
		 * event, to avoid further conditions, and because there's
		 * no harm done with the check even when the old and new
		 * strings are different lengths).
		 */
		if ((q = strstr(p, old)) == NULL)
			break;
		/* This is undefined if q - p > PTRDIFF_MAX */
		ptrdiff_t l = q - p;
		memcpy(r, p, l);
		r += l;
		memcpy(r, new, newlen);
		r += newlen;
		p = q + oldlen;
	}
	strcpy(r, p);

	return ret;
}

char *replace_str(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t oldlen = strlen(old);
	size_t count, retlen, newlen = strlen(new);

	if (oldlen != newlen) {
		for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
			count++;
		/* this is undefined if p - str > PTRDIFF_MAX */
		retlen = p - str + strlen(p) + count * (newlen - oldlen);
	} else
		retlen = strlen(str);

	if ((ret = malloc(retlen + 1)) == NULL)
		return NULL;

	for (r = ret, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen) {
		/* this is undefined if q - p > PTRDIFF_MAX */
		ptrdiff_t l = q - p;
		memcpy(r, p, l);
		r += l;
		memcpy(r, new, newlen);
		r += newlen;
	}
	strcpy(r, p);

	return ret;
}

int strReplaceChar(char *str, char oldChar, char replacedChar)
{
    assert(str);
    int size = 0;
    while (str[size] != '\0') {
        if (str[size] == oldChar) str[size] = replacedChar;
        size++;
    }
    return size;
}

size_t strSplit(char *str, char aDelimiter, char ***result)
{
    assert(str);
    size_t  count = strCountChar(str, aDelimiter);
    if (count && !*result) {
        count++;
        *result = malloc(sizeof(char*) * count);
        char *s = str;
        if (*result) {
            char *item = s;
            size_t i = 0;
            while (*s != '\0') {
                if (*s == aDelimiter) {
                    *s= '\0';
                    *(*result+i) = item;
                    item = s+1;
                    i++;
                }
                s++;
            }
            s--;
            if (*s != '\0') {
                *(*result+i) = item;
                i++;
            }
            else {
                count--;
            }
            assert(i == count);
        }
    }
    return count;
    //
}

inline size_t strCountChar(const char *str, char aChar)
{
    size_t result = 0;
    while (*str != '\0') {
        if (*str == aChar) ++result;
        str++;
    }
    return result;
}

#ifdef ISDK_STRING_TEST_MAIN
#include <stdio.h>
#include "testhelp.h"
#include "sds.h"

//gcc --std=c99 -I. -Ideps  -o test -DISDK_STRING_TEST_MAIN isdk_string.c deps/sds.c deps/zmalloc.c

sds mkJoinString(char **aStrs, size_t count, char *aDelimiter)
{
    sds result = sdsempty();
    for (size_t i = 0; i < count;    i++)
    {
        result = sdscat(result, aStrs[i]);
        result = sdscat(result, aDelimiter);
    }
    return result;
}

int main(void) {
    {
        puts("strSplit(\"test\\nit\\njack\", '\\n', &strs)");
        puts("----------------------------");
        char *splitedStrs[]= {"test", "it", "jack"};
        char **strs = 0;
        sds s = sdsnew("test\nit\njack");
        size_t c = strSplit(s, '\n', &strs);
        test_cond("Count should be 3", c == 3);
        for(int i=0; i< c; i++) {
            //printf("%s\n", result[i]);
            test_cond(strs[i],  strcmp(strs[i], splitedStrs[i])==0);
        }
        sdsfree(s);
        free(strs);
        puts("----------------------------");
        puts("strSplit(\"test\\nit\\njack\\n\", '\\n', &strs)");
        puts("----------------------------");
        s = mkJoinString(splitedStrs, 3, "\n");
        strs = 0;
        c = strSplit(s, '\n', &strs);
        test_cond("Count should be 3", c == 3);
        for(int i=0; i< c; i++) {
            //printf("%s\n", result[i]);
            test_cond(strs[i],  strcmp(strs[i], splitedStrs[i])==0);
        }
        sdsfree(s);
        free(strs);
        puts("----------------------------");
    }
}

#endif
