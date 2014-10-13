/*
 * =====================================================================================
 *
 *       Filename:  isdk_string.h
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

#ifndef __ISDK_STRING__
#define __ISDK_STRING__

#include <stdint.h>
#include <stdbool.h>

 #ifdef __cplusplus
 extern "C"
 {
 #endif

/* Compare two strings with case insensitive evaluation.
 *
 * Return Value
 *
 * stricmp returns a value indicating the relationship between the two strings, as follows:
 *
 * Less than 0
 *   astr less than bstr
 * 0
 *   astr equivalent to bstr
 * Greater than 0
 *   astr greater than bstr
 * 
 */
int stricmp(const char *astr, const char *bstr);
/* Check whether a string begins with a key. */
bool strBeginWith(const char *str, const char *key);
/* Check whether a string begins with a key with case insensitive evaluation. */
bool striBeginWith(const char *str, const char *key);
/* Check whether a string ends with a key. */
bool strEndWith(const char *str, const char *key);
/* Check whether a string ends with a key with case insensitive evaluation. */
bool striEndWith(const char *str, const char *key);
/* Convert the letters of a string into upper case. */
char *strToUpperCase(char *str);
/* Convert the letters of a string into lower case. */
char *strToLowerCase(char *str);
/* trim space characters at head and tail of a string. */
char *strTrim(char *str);
/* Substitute characters in a string. 
 * 
 * if find the char in the searchCharset will replace it within the replacedCharset
 * eg, strReplaceChars("hello", "eol", "sco") => "hsooc" 
 *
 * */
char *strReplaceChars(char *str, const char *searchCharset, const char *replacedCharset);
/* Count the number of characters in a string of UTF-8. */
int utf8StrLen(const char *str);
/* Convert a string to an int64. */
int64_t strToInt64(const char *str);
/* Convert a string with a metric prefix to an integer. 
 *
 * eg, "1024K", "500G"
 *
 *
 * */
int64_t strToInt64x(const char *str);

//repalce the oldChar with the new replacedChar.
//return the str's size 
int strReplaceChar(char *str, char oldChar, char replacedChar);

//if can split this return a new string array and size.
//NOTE: the *str MUST be a variable, NOT a constant string!!
//      the function would write to '\0' to replace the aDelimiter for saving space
size_t strSplit(char *str, char aDelimiter, char ***result);

inline size_t strCountChar(const char *str, char aChar);

 #ifdef __cplusplus
 }
 #endif

#endif
