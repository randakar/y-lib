/* vugen.h - A.U. Luyer - 2012-03-27
 * Last update 2014-01-09
 * This header file contains the most used 'standard C functions' in Vugen scripts.
 * By declaring these functions properly the compiler can check if these functions 
 * are used correctly and apply implicit conversions when necessary.
 * This header file 'de-strips' the compiler in Vugen.
 * It contains all functions mentioned in the HP LoadRunner Online Function Reference.
 * NOTE: Sites like http://www.cplusplus.com/reference/clibrary/ explain the standard C functions
 * better than the HP LoadRunner Online Function Reference.
 *
 * Usage: add vugen.h to the script and add #include "vugen.h" to globals.h.
 * Or: add to include directory and add the include to lhrun.h on all Vugen machines and load generators.
 */

/*!
\file vugen.h
\brief Standard C function headers
*/

#ifndef _VUGEN_H_
#define _VUGEN_H_

/* typedef unsigned long size_t; */
/* WARNING: lrun.h contains #define size_t int, which is incorrect */

/***** String functions *****/

int snprintf(char *buffer, size_t n, const char *format_string, ...); /* Feature introduced by the latest revision of the C++ standard (2011). Older compilers may not support it. */
int sprintf(char *buffer, const char *format_string, ...); /* you should prefer snprintf over sprintf */
int sscanf(const char *buffer, const char *format_string, ...);
char *strchr(const char *string, int c);
int strcmp(const char *string1, const char *string2);
char *strcpy(char *dest, const char *source);
char *strdup(const char *string);
int stricmp(const char *string1, const char *string2);
size_t strlen(const char *string);
char *strlwr(char *string);
char *strncat(char *to_string, const char *from_string, size_t n);
int strncmp(const char *string1, const char *string2, size_t n);
int strnicmp(const char *string1, const char *string2, size_t num);
char *strrchr(const char *string, int c);
char *strset(char *string1, int character);
size_t *strspn(const char *string, const char *skipset);
char *strstr(const char *string1, const char *string2);
char *strtok(char *string, const char *delimiters);
char *strupr(char *string);

/* Force a compile time error when strcat() is used. Because the use of strcat() is a very
 * strong indicator for poorly written code being compiled -- there is always a better solution!
 * Unfortunately the use of #error is not possible here.
 * Original declaration: char *strcat(char *to, const char *from);
 */
#define strcat(to, from) 0_DO_NOT_USE_strcat_THERE_IS_ALWAYS_A_BETTER_SOLUTION

double atof(const char *string);
int atoi(const char *string);
long atol(const char *string);
int itoa(int value, char *str, int radix);
long strtol(const char *string, char **endptr, int radix);
unsigned long int strtoul(const char *str, char **endptr, int base);
double strtod(const char *str, char **endptr);

/* ctype.h -- http://www.acm.uiuc.edu/webmonkeys/book/c_guide/ */
int tolower(int character);
int toupper(int character);
int isalnum(int character);
int isalpha(int character);
int iscntrl(int character);
int isdigit(int character);
int isgraph(int character);
int islower(int character);
int isprint(int character);
int ispunct(int character);
int isspace(int character);
int isupper(int character);
int isxdigit(int character);

void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
/* NOTE: The operation of memcpy is not guaranteed on overlap, use memmove instead */
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *buffer, int c, size_t n);

/***** Time structures and functions *****/

typedef long time_t;
struct tm {
    int tm_sec; /* seconds after the minute - [0,59] */
    int tm_min; /* minutes after the hour - [0,59] */
    int tm_hour; /* hours since midnight - [0,23] */
    int tm_mday; /* day of the month - [1,31] */
    int tm_mon; /* months since January - [0,11] */
    int tm_year; /* years since 1900 */
    int tm_wday; /* days since Sunday - [0,6] */
    int tm_yday; /* days since January 1 - [0,365] */
    int tm_isdst; /* daylight savings time flag */
    #ifdef LINUX
        int tm_gmtoff;
        const char *tm_zone;
    #endif
};

struct _timeb { 
    time_t time; 
    unsigned short millitm; 
    short timezone; 
    short dstflag; 
}; 

time_t time(time_t *timeptr);
char *ctime(const time_t *calTime);
void ftime(struct _timeb *time1);
struct tm *gmtime(const time_t *calTime);
char *asctime(const struct tm *tmTime);
size_t *strftime(char *string, size_t maxlen, const char *format, const struct tm *timestruct);
time_t mktime(struct tm * timeptr);
struct tm *localtime(const time_t *timer);
void tzset(void);

/***** File functions *****/
/* NOTE: the file functions use (FILE *), but because they are declared as
 * (long) in the on-line help, we use it here also...
 * So 'long file_pointer' instead of 'FILE *file_pointer'.
 * This 'happens' to be ok because sizeof(void *) == sizeof(long)
 */
int fclose(long file_pointer);
int feof(long file_pointer);
int ferror(long file_pointer);
int fgetc(long file_pointer);
char *fgets(char *string, int maxchar, long file_pointer);
int fputs(const char *str, long file_pointer);
long fopen(const char *filename, const char *access_mode);
int fprintf(long file_pointer, const char *format_string, ...);
int fputc(int c, long file_pointer);
size_t fread(void *buffer, size_t size, size_t count, long file_pointer);
int fscanf(long file_pointer, const char *format_string, ...);
int fseek(long file_pointer, long offset, int origin);
long ftell(long file_pointer);
size_t fwrite(const void *buffer, size_t size, size_t count, long file_pointer);
void rewind(long file_pointer);
long tmpfile(void);
char *tmpnam(char *str);
int setvbuf(long file_pointer, char * buffer, int mode, size_t size);

#define FILENAME_MAX 1024
#define L_tmpnam FILENAME_MAX

#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif
#define _IOFBF 0  /* setvbuf should set fully buffered */
#define _IOLBF 1  /* setvbuf should set line buffered */
#define _IONBF 2  /* setvbuf should set unbuffered */

#define EOF (-1)

/*
 * Routines in POSIX 1003.1:2001.
 */
int getw(long file_pointer);
int pclose(long file_pointer);
long popen(const char *command, const char *access_mode);
int putw(int word, long file_pointer);

/***** File system functions *****/

int chdir(const char *path);
int chdrive(int drive);
char *getcwd(char *path, int numchars);
int getdrive(void);
int remove(const char *path);
int rename(const char *oldname, const char *newname);
int rmdir(const char *path);

/***** locale.h functions *****/
/* BEWARE: Vugen executes a setlocale(LC_ALL, "") before the start -- so the system's default locale
 * is active instead of the minimal locale and the default for all C compilers: setlocale(LC_ALL, "C").
 * This may cause functions like sscanf to behave unexpectedly, for instance when the decimal point
 * is a comma (as in many European languages) sscanf(buf , "%lf", &double_var) will not yield the correct
 * number.
 * To print the currently active locale use: lr_log_message("%s", setlocale(LC_ALL, NULL));
 */
#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MESSAGES 6
char *setlocale(int category, const char *locale);

struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
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
struct lconv *localeconv(void);

/***** Process Control Functions *****/

char *getenv(const char *varname);
int putenv(const char *envstring);
int system(const char *string); /* popen function is more useful */

/***** Memory Allocation Functions *****/

void *calloc(size_t num_elems, size_t elem_size);
void free(void *mem_address);
void *malloc(size_t num_bytes);
void *realloc(void *mem_address, size_t size);
/* NOTE: realloc(NULL, size) === malloc(size) */ 

/***** Mathematic Functions *****/

int abs(int n);
double fabs(double x);
double ceil(double x);
double floor(double x);
double fmod(double numerator, double denominator);
double modf(double x, double *intpart);
double exp(double x);
double sqrt(double x);
double pow(double base, double exponent);
double log(double x);
double log10(double x);
double sin(double x);
double cos(double x);

int rand(void);
int srand(unsigned int seed);

/***** Windows API Functions *****/

void sleep(DWORD dwMilliseconds); /* you should use lr_force_think_time() or lr_usleep() instead */
/* alternative: #define sleep(msec) lr_force_think_time(msec / 1000.) */

#endif /* _VUGEN_H_ */
