/*!
 * \file
 * \brief Standard C function headers
 *
 * This header file contains the most used 'standard C functions' in Vugen scripts. \n
 * By declaring these functions properly the compiler can check if these functions 
 * are used correctly and apply implicit conversions when necessary. \n
 * This header file 'de-strips' the compiler in Vugen. \n
 * It contains all functions mentioned in the HP LoadRunner Online Function Reference.
 * \note Sites like http://www.cplusplus.com/reference/clibrary/ explain the standard C functions
 * better than the HP LoadRunner Online Function Reference and more.
 *
 * \b Usage: Automatically included as part of y-lib. To use this stand-alone add vugen.h to the
 * script and add the following to globals.h or vuser_init.c:
 * \code #include "vugen.h" \endcode
 * Or: add to include-directory and add the \#include to lhrun.h on all Vugen machines and load generators. \n
 * Initial version 2012-03-27, last update 2014-01-09
 * \author A.U. Luyer
 * \warning lrun.h contains: \code #define size_t int \endcode causing size_t to be defined as \b signed integer, which is incorrect.
 */

#ifndef _VUGEN_H_
#define _VUGEN_H_

/* typedef unsigned long size_t; */

/***** String functions *****/
/*! \defgroup string Standard C string functions
 * \brief Standard C functions using null terminated C strings (defined in stdio.h, cstring.h)
 * \{
 */
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/snprintf/. \n
//! This function was introduced by the latest revision of the C++ standard (2011). Older compilers may not support it.
int snprintf(char *buffer, size_t n, const char *format_string, ...);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/sprintf/. \n You should prefer ::snprintf over sprintf.
int sprintf(char *buffer, const char *format_string, ...); 
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/sscanf/.
int sscanf(const char *buffer, const char *format_string, ...);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strchr/.
char *strchr(const char *string, int c);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strcmp/.
int strcmp(const char *string1, const char *string2);
//! \brief Documented at http://pubs.opengroup.org/onlinepubs/007904975/functions/strdup.html .
char *strdup(const char *string);
//! \brief Documented at http://pic.dhe.ibm.com/infocenter/iseries/v7r1m0/topic/rtref/stricmp.htm .
int stricmp(const char *string1, const char *string2);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strlen/.
size_t strlen(const char *string);
//! \brief Documented at http://www.qnx.com/developers/docs/6.5.0/topic/com.qnx.doc.neutrino_lib_ref/s/strlwr.html .
char *strlwr(char *string);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strncat/.
char *strncat(char *to_string, const char *from_string, size_t n);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strncmp/.
int strncmp(const char *string1, const char *string2, size_t n);
//! \brief Documented at http://pic.dhe.ibm.com/infocenter/iseries/v7r1m0/topic/rtref/strnicmp.htm.
int strnicmp(const char *string1, const char *string2, size_t num);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strncpy/.
char *strncpy(char *destination, const char *source, size_t num);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strrchr/.
char *strrchr(const char *string, int c);
//! \brief Documented at http://www.qnx.com/developers/docs/6.5.0/topic/com.qnx.doc.neutrino_lib_ref/s/strset.html .
char *strset(char *string1, int character);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strspn/.
size_t *strspn(const char *string, const char *skipset);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strstr/.
char *strstr(const char *string1, const char *string2);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/strtok/.
char *strtok(char *string, const char *delimiters);
//! \brief Documented at http://www.qnx.com/developers/docs/6.5.0/topic/com.qnx.doc.neutrino_lib_ref/s/strupr.html .
char *strupr(char *string);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/atof/.
double atof(const char *string);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/atoi/.
int atoi(const char *string);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/atol/.
long atol(const char *string);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/itoa/.
int itoa(int value, char *str, int radix);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/strtol/.
long strtol(const char *string, char **endptr, int radix);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/strtoul/.
unsigned long int strtoul(const char *str, char **endptr, int base);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/strtod/.
double strtod(const char *str, char **endptr);

/*!
 * \def strcat
 * \brief Force a compile time error when strcat() is used. \n
 * Because the use of strcat() is a <b>very strong</b> indicator for poorly written code being compiled -- there is always a better solution!
 *
 * Unfortunately the use of \#error is not possible here.
 * \note Original declaration: char *strcat(char *to, const char *from);
 !*/
#define strcat(to, from) 0_DO_NOT_USE_strcat_THERE_IS_ALWAYS_A_BETTER_SOLUTION

/*!
 * \def strcpy
 * \brief Force a compile time error when strcpy() is used. \n
 * The use of strcpy is banned because in practice strcpy is only used by novice coders doing unnecessary duplicating.
 * (Typically when trying to avoid using pointers when actually using pointers.) \n
 * Evaluate your coding solution and avoid duplication. If you must, use the safer ::strncpy or ::snprintf.
 * 
 * \note Original declaration: char *strcpy(char *dest, const char *source);
 !*/
#define strcpy(dest, source) 0_DO_NOT_USE_strcpy_DUPLICATION_IS_NOT_REQUIRED
//! \}

/* ctype.h -- http://www.acm.uiuc.edu/webmonkeys/book/c_guide/ */
/*! \defgroup char Standard C character functions
 * \brief Standard C functions using character defined in ctype.h
 * \{
*/
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isalnum/.
int isalnum(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isalpha/.
int isalpha(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/iscntrl/.
int iscntrl(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isdigit/.
int isdigit(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isgraph/.
int isgraph(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/islower/.
int islower(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isprint/.
int isprint(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/ispunct/.
int ispunct(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isspace/.
int isspace(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isupper/.
int isupper(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/isxdigit/.
int isxdigit(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/tolower/.
int tolower(int character);
//! \brief Documented at http://www.cplusplus.com/reference/cctype/toupper/.
int toupper(int character);
//! \}

/* ctype.h -- http://www.acm.uiuc.edu/webmonkeys/book/c_guide/ */
/*! \defgroup memory Standard C memory functions
 * \brief Standard C memory functions (defined in cstring.h, stdlib.h)
 * \{
*/
//! \brief Documented at http://www.cplusplus.com/reference/cstring/memchr/.
void *memchr(const void *s, int c, size_t n);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/memcmp/.
int memcmp(const void *s1, const void *s2, size_t n);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/memcpy/. \note The operation of memcpy is not guaranteed on overlap, use ::memmove instead
void *memcpy(void *dest, const void *src, size_t n);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/memmove/.
void *memmove(void *dest, const void *src, size_t n);
//! \brief Documented at http://www.cplusplus.com/reference/cstring/memset/.
void *memset(void *buffer, int c, size_t n);

/***** Memory Allocation Functions *****/
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/calloc/.
void *calloc(size_t num_elems, size_t elem_size);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/free/.
void free(void *mem_address);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/malloc/.
void *malloc(size_t num_bytes);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/realloc/. \note ::{realloc}(NULL, size) === ::{malloc}(size)
void *realloc(void *mem_address, size_t size);
//! \}

/***** Time structures and functions *****/
/*! \defgroup time Standard C time structures and functions
 * \brief Standard C structures and functions to handle date and time (defined in ctime.h)
 * \{
*/
//! \brief Type time_t can hold a Unix timestamp
typedef long time_t;

//! \brief Documented at http://www.cplusplus.com/reference/ctime/tm/. \n
//! Values outside of valid ranges can be used to calculate a new date/time using ::mktime. E.g. tm_mday = 0 means last day of previous month.
struct tm {
	//! \brief seconds after the minute - [0,61] (or [0,59] when \a leap \a seconds are not supported)
    int tm_sec;
	//! \brief minutes after the hour - [0,59]
    int tm_min;
	//! \brief hours since midnight - [0,23]
    int tm_hour;
	//! \brief day of the month - [1,31]
    int tm_mday;
	//! \brief months since January - [0,11]
    int tm_mon;
	//! \brief years since 1900
    int tm_year;
 	//! \brief days since Sunday - [0,6] (ignored by mktime)
    int tm_wday;
	//! \brief days since January 1 - [0,365] (ignored by mktime)
    int tm_yday;
	//! \brief daylight savings time flag (>0 in effect, 0 not in effect, <0 unknown)
    int tm_isdst;
    #ifdef LINUX
        int tm_gmtoff;
        const char *tm_zone;
    #endif
};

//! \brief Documented at http://www.cplusplus.com/reference/ctime/time/.
time_t time(time_t *timeptr);
//! \brief Documented at http://www.cplusplus.com/reference/ctime/ctime/.
char *ctime(const time_t *calTime);
//! \brief Documented at http://www.cplusplus.com/reference/ctime/gmtime/.
struct tm *gmtime(const time_t *calTime);
//! \brief Documented at http://www.cplusplus.com/reference/ctime/asctime/.
char *asctime(const struct tm *tmTime);
//! \brief Documented at http://www.cplusplus.com/reference/ctime/strftime/.
size_t *strftime(char *string, size_t maxlen, const char *format, const struct tm *timestruct);
//! \brief Documented at http://www.cplusplus.com/reference/ctime/mktime/.
time_t mktime(struct tm * timeptr);
//! \brief Documented at http://www.cplusplus.com/reference/ctime/localtime/.
struct tm *localtime(const time_t *timer);
//! \brief Documented at http://www.cplusplus.com/reference/ctime/tzset/.
void tzset(void);

//! \brief Used by ::ftime. Defined as _timeb (instead of timeb) just as in the on-line Help
//! \note On Windows accurate to 1/64 of a second (15.6 msec).
struct _timeb {
	//! \brief Time, in seconds, since the Unix Epoch, 1 January 1970 00:00:00 Coordinated Universal Time (UTC).
    time_t time; 
	//! \brief Milliseconds. Actual accuracy may be lower.
    unsigned short millitm; 
	//! \brief Difference in \b minutes of the timezone from UTC.
    short timezone; 
	//! \brief Nonzero if in daylight savings time.
    short dstflag; 
};

//! \brief Documented at http://www.qnx.com/developers/docs/6.5.0/topic/com.qnx.doc.neutrino_lib_ref/f/ftime.html .
void ftime(struct _timeb *time);
//! \}

/***** File functions *****/
/*! \defgroup file Standard C time file functions
 * \brief Standard C structures and functions to handle files (defined in stdio.h)
 * \note The file functions use (FILE *), but because they are declared as
 * (long) in the on-line help, we use it here also... \n
 *
 * So 'long file_pointer' instead of 'FILE *file_pointer'. \n
 * This 'happens' to be ok because sizeof(void *) == sizeof(long)
 * \{
*/
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fclose/.
int fclose(long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/feof/.
int feof(long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/ferror/.
int ferror(long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fgetc/.
int fgetc(long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fgets/.
char *fgets(char *string, int maxchar, long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fputs/.
int fputs(const char *str, long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fopen/.
long fopen(const char *filename, const char *access_mode);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fprintf/.
int fprintf(long file_pointer, const char *format_string, ...);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fputc/.
int fputc(int c, long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fread/.
size_t fread(void *buffer, size_t size, size_t count, long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fscanf/.
int fscanf(long file_pointer, const char *format_string, ...);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fseek/.
int fseek(long file_pointer, long offset, int origin);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/ftell/.
long ftell(long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/fwrite/.
size_t fwrite(const void *buffer, size_t size, size_t count, long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/rewind/.
void rewind(long file_pointer);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/tmpfile/.
long tmpfile(void);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/tmpnam/.
char *tmpnam(char *str);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/setvbuf/.
int setvbuf(long file_pointer, char * buffer, int mode, size_t size);

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif
#ifndef L_tmpnam
#define L_tmpnam FILENAME_MAX
#endif

//! \brief set file offset to offset
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
//! \brief set file offset to current plus offset
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
//! \brief set file offset to EOF \b plus offset
#ifndef SEEK_END
#define SEEK_END 2
#endif
//! \brief setvbuf should set fully buffered
#define _IOFBF 0
//! \brief setvbuf should set line buffered
#define _IOLBF 1
//! \brief setvbuf should set unbuffered
#define _IONBF 2
//! \brief the End-Of-File constant
#define EOF (-1)

//! \}
/*
 * Routines in POSIX 1003.1:2001.
 */
/*! \defgroup process POSIX and Standard C process and file system functions
 * \brief POSIX and Standard C process and file system functions (defined in stdio.h, stdlib.h)
 * \{
*/ 
//! \brief Documented at http://man7.org/linux/man-pages/man3/getw.3.html .
int getw(long file_pointer);
//! \brief Documented at http://man7.org/linux/man-pages/man3/pclose.3.html .
int pclose(long file_pointer);
//! \brief Documented at http://man7.org/linux/man-pages/man3/popen.3.html .
long popen(const char *command, const char *access_mode);
//! \brief Documented at http://man7.org/linux/man-pages/man3/putw.3.html .
int putw(int word, long file_pointer);

/***** Process Control Functions *****/
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/getenv/.
char *getenv(const char *varname);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/putenv/.
int putenv(const char *envstring);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/system/. \n The ::popen function is more useful in LR scripts
int system(const char *string);

/***** File system functions *****/
//! \brief Documented at http://man7.org/linux/man-pages/man2/chdir.2.html .
int chdir(const char *path);
//! \brief See on-line help
int chdrive(int drive);
//! \brief See on-line help
char *getcwd(char *path, int numchars);
//! \brief See on-line help
int getdrive(void);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/remove/.
int remove(const char *path);
//! \brief Documented at http://www.cplusplus.com/reference/cstdio/rename/.
int rename(const char *oldname, const char *newname);
//! \brief See on-line help
int rmdir(const char *path);
//! \}

/***** locale.h functions *****/
/*! \defgroup locale Standard C locale structures and functions
 * \brief Standard C structures and functions to handle the locale (defined in locale.h)
 * \warning Vugen executes a setlocale(LC_ALL, "") before the start -- so the system's default locale
 * is active instead of the minimal locale and the default for all C compilers: setlocale(LC_ALL, "C"). \n
 * This may cause functions like sscanf to behave unexpectedly, for instance when the decimal point
 * is a comma (as in many European languages) sscanf(buf , "%lf", &double_var) will not yield the correct
 * number. \n
 * To print the currently active locale use: lr_log_message("%s", setlocale(LC_ALL, NULL));
 * \{
*/
#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MESSAGES 6
//! \brief Documented at http://www.cplusplus.com/reference/clocale/setlocale/.
char *setlocale(int category, const char *locale);

//! \brief Returned by ::localeconv. Documented at http://www.cplusplus.com/reference/clocale/lconv/.
struct lconv {
	//! \brief Decimal-point separator used for non-monetary quantities.
    char *decimal_point;
	//! \brief Separators used to delimit groups of digits to the left of the decimal point for non-monetary quantities.
    char *thousands_sep;
	//! \brief Specifies the amount of digits that form each of the groups to be separated by thousands_sep separator for non-monetary quantities.
    char *grouping;
	//! \brief International currency symbol. This is formed by the three-letter ISO-4217 entry code for the currency.
    char *int_curr_symbol;
	//! \brief Local currency symbol.
    char *currency_symbol;
	//! \brief Decimal-point separator used for monetary quantities.
    char *mon_decimal_point;
	//! \brief Separators used to delimit groups of digits to the left of the decimal point for monetary quantities.
    char *mon_thousands_sep;
	//! \brief Specifies the amount of digits that form each of the groups to be separated by mon_thousands_sep separator for monetary quantities.
    char *mon_grouping;
	//! \brief Sign to be used for positive or zero monetary quantities.
    char *positive_sign;
	//! \brief Sign to be used for negative monetary quantities.
    char *negative_sign;
	//! \brief Amount of fractional digits to the right of the decimal point for monetary quantities in the international format
    char int_frac_digits;
	//! \brief Amount of fractional digits to the right of the decimal point for monetary quantities in the local format.
    char frac_digits;
	//! \brief Boolean whether the currency symbol should precede positive or zero monetary quantities.
    char p_cs_precedes;
	//! \brief Boolean whether the currency symbol should precede negative monetary quantities.
    char p_sep_by_space;
	//! \brief Boolean whether a space should appear between the currency symbol and positive or zero monetary quantities.
    char n_cs_precedes;
	//! \brief Boolean whether a space should appear between the currency symbol and negative monetary quantities.
    char n_sep_by_space;
	//! \brief Position of the sign for positive or zero monetary quantities.
    char p_sign_posn;
	//! \brief Position of the sign for negative monetary quantities.
    char n_sign_posn;
/*
The members below are not yet supported (not complying with the C standard of 1999 or later).
    char int_n_cs_precedes;
    char int_n_sep_by_space;
    char int_n_sign_posn;
    char int_p_cs_precedes;
    char int_p_sep_by_space;
    char int_p_sign_posn;
*/
};
//! \brief Documented at http://www.cplusplus.com/reference/clocale/localeconv/.
struct lconv *localeconv(void);
//! \}

/***** Mathematic Functions *****/
/*! \defgroup math Standard C mathematic functions
 * \brief Mathematic functions (defined in cmath.h)
 * \{
*/ 
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/abs/.
int abs(int n);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/fabs/.
double fabs(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/ceil/.
double ceil(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/floor/.
double floor(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/fmod/.
double fmod(double numerator, double denominator);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/modf/.
double modf(double x, double *intpart);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/exp/.
double exp(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/sqrt/.
double sqrt(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/pow/.
double pow(double base, double exponent);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/log/.
double log(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/log10/.
double log10(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/log2/.
double log2(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/sin/.
double sin(double x);
//! \brief Documented at http://www.cplusplus.com/reference/cmath/cos/.
double cos(double x);
//! \}

//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/rand/.
int rand(void);
//! \brief Documented at http://www.cplusplus.com/reference/cstdlib/srand/.
int srand(unsigned int seed);
#ifdef WINNT
//! \brief Documented at https://msdn.microsoft.com/en-us/library/sxtz2fa8.aspx
int rand_s(unsigned int *randomValue); 
#endif

/***** Windows API Functions *****/
//! \brief Windows API uninterruptable sleep in LR, better to use \b lr_force_think_time() or \b lr_usleep() instead.
void sleep(DWORD dwMilliseconds);
/* alternative: #define sleep(msec) lr_force_think_time(msec / 1000.) */

// doxygen info: http://www.stack.nl/~dimitri/doxygen/manual/commands.html
#endif /* _VUGEN_H_ */
