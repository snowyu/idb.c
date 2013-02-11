/* Basic filename support macros.
   Copyright (C) 2001-2004, 2007-2012 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

# ifndef _FILENAME_H
# define _FILENAME_H

# include <stdbool.h>

# include "pathmax.h"
# ifndef PATH_MAX
#  define PATH_MAX 8192
# endif

# ifdef __cplusplus
extern "C" {
# endif

// Path Seperator:
# define PATH_SEP_STR  "/"
# define PATH_SEP  (PATH_SEP_STR[0])
//# define PATH_SEP '/'

/* Pathname support.
   ISSLASH(C)           tests whether C is a directory separator character.
   IS_ABSOLUTE_PATH(P)  tests whether P is an absolute path.  If it is not,
                        it may be concatenated to a directory pathname.
   IS_PATH_WITH_DIR(P)  tests whether P contains a directory specification.
 */
#if (defined _WIN32 || defined __WIN32__ ||     \
     defined __MSDOS__ || defined __CYGWIN__ || \
     defined __EMX__ || defined __DJGPP__)
  /* Native Windows, Cygwin, OS/2, DOS */
   /* This internal macro assumes ASCII, but all hosts that support drive
      letters use ASCII.  */
# define _IS_DRIVE_LETTER(C) (((unsigned int) (C) | ('a' - 'A')) - 'a'  \
                              <= 'z' - 'a')
//# define _IS_DRIVE_LETTER(C) ((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z'))

# define HAS_DEVICE(P) (_IS_DRIVE_LETTER((P)[0]) && (P)[1] == ':')

# define FILE_SYSTEM_PREFIX_LEN(Filename) \
          (_IS_DRIVE_LETTER ((Filename)[0]) && (Filename)[1] == ':' ? 2 : 0)
# ifndef __CYGWIN__
#  define FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE 1
# endif

# define ISSLASH(C) ((C) == '/' || (C) == '\\')
//# define IS_ABSOLUTE_PATH(P) (ISSLASH ((P)[0]) || HAS_DEVICE (P))
# define IS_PATH_WITH_DIR(P) \
    (strchr (P, '/') != NULL || strchr (P, '\\') != NULL || HAS_DEVICE (P))
# define FILE_SYSTEM_PREFIX_LEN(P) (HAS_DEVICE (P) ? 2 : 0)
#else
  /* Unix */
# define ISSLASH(C) ((C) == '/')
//# define IS_ABSOLUTE_PATH(P) ISSLASH ((P)[0])
# define IS_PATH_WITH_DIR(P) (strchr (P, '/') != NULL)
# define FILE_SYSTEM_PREFIX_LEN(P) 0
#endif

#ifndef FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE
# define FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE 0
#endif

#if FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE
#  define IS_ABSOLUTE_FILE_NAME(F) ISSLASH ((F)[FILE_SYSTEM_PREFIX_LEN (F)])
# else
#  define IS_ABSOLUTE_FILE_NAME(F)                              \
     (ISSLASH ((F)[0]) || FILE_SYSTEM_PREFIX_LEN (F) != 0)
#endif
#define IS_RELATIVE_FILE_NAME(F) (! IS_ABSOLUTE_FILE_NAME (F))

# ifndef DOUBLE_SLASH_IS_DISTINCT_ROOT
#  define DOUBLE_SLASH_IS_DISTINCT_ROOT 0
# endif


//check file_name whether be dot or dotdot.
static inline bool
dot_or_dotdot (char const *file_name)
{
  if (file_name[0] == '.')
    {
      char sep = file_name[(file_name[1] == '.') + 1];
      return (! sep || ISSLASH (sep));
    }
  else
    return false;
}

# if GNULIB_DIRNAME
char *base_name (char const *file);
char *dir_name (char const *file);
# endif

//char *mdir_name (char const *file);
size_t base_len (char const *file) ;//_GL_ATTRIBUTE_PURE;
//size_t dir_len (char const *file) _GL_ATTRIBUTE_PURE;
char *last_component (char const *file);// _GL_ATTRIBUTE_PURE;

//bool strip_trailing_slashes (char *file);

#ifdef __cplusplus
}
#endif

#endif /* _FILENAME_H */
