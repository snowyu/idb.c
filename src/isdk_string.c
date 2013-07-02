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
