/*
 * vugen.h - A.U. Luyer - 27 maart 2012
 * Deze header file bevat de veel voorkomende 'standaard C functies' in vugen scripts
 * Door deze functies netjes te declareren kan de comnpiler checks uitvoeren of deze
 * functies correct worden aangeroepen.
 * Deze header file 'ont-stript' de compiler in Vugen.
 * Het bevat in ieder geval de functies die genoemd worden in de HP LoadRunner Online Function Reference.
 */

#ifndef _VUGEN_H_
#define _VUGEN_H_

// test test 

/***** 'extra' variabele types *****/
//typedef unsigned long size_t; // ook al in lrun.h

/***** String functies *****/

int sprintf(char *buffer, const char *format_string, ...);
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

/* Forceer een compiler fout als strcat() gebruikt wordt, omdat het gebruik van strcat() vrijwel
 * altijd een indicatie is van slecht programeerwerk -- er is altijd een betere oplossing!
 * Hier #error gebruiken kan niet...
 * officieel: char *strcat(char *to, const char *from);
 */
#define strcat(to, from) 0_strcat_NIET_GEBRUIKEN_ER_IS_ALTIJD_EEN_BETERE_OPLOSSING

double atof(const char *string);
int atoi(const char *string);
long atol(const char *string);
int itoa(int value, char *str, int radix);
long strtol(const char *string, char **endptr, int radix);
unsigned long int strtoul(const char *str, char **endptr, int base);
double strtod(const char *str, char **endptr);

int tolower(int c);
int toupper(int c);
int isdigit(int c);
int isalpha(int c);

void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
/* verkies memmove boven memcpy! werking memcpy niet gegarandeerd bij overlap src - dst */
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *buffer, int c, size_t n);

/***** Tijd structures en functies *****/

typedef long time_t;
struct tm {
	int tm_sec; // seconds after the minute - [0,59]
	int tm_min; // minutes after the hour - [0,59]
	int tm_hour; // hours since midnight - [0,23]
	int tm_mday; // day of the month - [1,31]
	int tm_mon; // months since January - [0,11]
	int tm_year; // years since 1900
	int tm_wday; // days since Sunday - [0,6]
	int tm_yday; // days since January 1 - [0,365]
	int tm_isdst; // daylight savings time flag
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
struct tm *localtime(const time_t * timer);
void tzset(void);

/***** File functies *****/
/* LET OP: f-functies gebruiken in werkelijkijkheid (FILE *) type maar omdat in de
 * Vugen help deze gedefineerd worden als (long) hier ook maar zo...
 * Dus long file_pointer ipv FILE *file_pointer
 * Dat gaat 'toevallig' goed omdat geldt: sizeof(void *) == sizeof(long)
 */
 
int fclose(long file_pointer);
int feof(long file_pointer);
int ferror(long file_pointer);
int fgetc(long file_pointer);
char *fgets(char *string, int maxchar, long file_pointer);
int fputs(const char *str, long file_pointer);
long fopen(const char *filename, const char *access_mode);
int fprintf(long file_pointer, const char *format_string, ...);
int fputc(int c,long file_pointer);
size_t fread(void *buffer, size_t size, size_t count, long file_pointer);
int fscanf(long file_pointer, const char *format_string, ...);
int fseek(long file_pointer, long offset, int origin);
long ftell(long file_pointer);
size_t fwrite(const void *buffer, size_t size, size_t count, long file_pointer);
void rewind(long file_pointer);
int setvbuf(long file_pointer, char * buffer, int mode, size_t size);

#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif
#define	_IOFBF	0		/* setvbuf should set fully buffered */
#define	_IOLBF	1		/* setvbuf should set line buffered */
#define	_IONBF	2		/* setvbuf should set unbuffered */

#define	EOF	(-1)

int chdir(const char *path);
int chdrive(int drive);
char *getcwd(char *path, int numchars);
int getdrive(void);
int remove(const char *path);
int rename(const char *oldname, const char *newname);
int rmdir(const char *path);

/*
 * Routines in POSIX 1003.1:2001.
 */
int	getw(long file_pointer);
int	pclose(long file_pointer);
long popen(const char *command, const char *access_mode);
int	putw(int, long file_pointer);

/***** locale.h functies *****/
/* LET OP: Vugen voert voor de start setlocale(LC_ALL, "") uit -- dus de system's default locale
 * In plaats van setlocale(LC_ALL, "C") -- the minimal locale en de default voor alle C compilers...
 * waardoor bv sscanf(buf , "%f", &double_var) niet goed werkt op Nederlandse Windows locale
 */
#define LC_ALL	    0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MESSAGES 6
char *setlocale(int category, const char *locale);

/***** Process Control Functions *****/

char *getenv(const char *varname);
int putenv(const char *envstring);
int system(const char *string);

/***** Memory Allocation Functions *****/

void *calloc(size_t num_elems, size_t elem_size);
void free(void *mem_address);
void *malloc(size_t num_bytes);
void *realloc(void *mem_address, size_t size);
/* Note: realloc(NULL, size) === malloc(size) */ 

/***** Mathematic Functions *****/

int abs(int n);
double fabs(double x);
double ceil(double x);
double floor(double x);
double fmod(double numerator, double denominator);
double modf(double x, double * intpart);
double exp(double x);
double sqrt(double x);
double pow(double base, double exponent);
double log(double x);
double log10(double x);
double sin(double x);
double cos(double x);

int rand(void);
int srand(unsigned int seed);

#endif /* _VUGEN_H_ */
