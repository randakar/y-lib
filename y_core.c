/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2014 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
 * Copyright (C) 2009 Raymond de Jongh <ferretproof@gmail.com> | <rdjongh@ymor.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*! 
\file y_core.c
\brief Contains core ylib support functions needed for the functioning of the library.

This file contains two types of functions:
1) Functions that are needed to make the rest of y_lib work properly.
2) Functions that are deemed important enough that almost all scripts are expected to use them to some extend or other.
*/
#ifndef _Y_CORE_C_
//! \cond include_protection
#define _Y_CORE_C_
//! \endcond

#include "vugen.h"

/*! \defgroup core Core of Ylib
 * \{
*/
//! The virtual user id, as reported by lr_whoami(). \sa y_setup()
int y_virtual_user_id = 0;
//! The virtual user group, as reported by lr_whoami(). \sa y_setup()
char* y_virtual_user_group = NULL;
//! The virtual user scid, as reported by lr_whoami(). \sa y_setup()
int y_scid;
//! Boolean, true when running in Vugen. Not able to do this in pre-compile phase. \sa y_setup()
int y_is_vugen_run_bool = 0;

#ifndef RAND_MAX
/*!
\def RAND_MAX
\brief RAND_MAX constant for use with rand() - 15 bits integer.

Loadrunner does not give you full C headers, so the 'RAND_MAX' \#define from <stdlib.h> is missing. 
We define it here mostly for documentation, as we do not have access to the header files themselves and therefore cannot change this. 
\author Floris Kraak
*/
#define RAND_MAX 32767
#endif

/*!
\def Y_RAND_MAX 
\brief Alternate RAND_MAX constant for use with y_rand.

y_rand() provides for a far bigger ceiling to the random number generator: 30 bits, instead of 15.
\author Floris Kraak
*/
#define Y_RAND_MAX 1073741823


/*!   
\brief Ylib setup - determines and stores the identity of the virtual user.

This runs lr_whoami and sets y_virtual_user_id and y_virtual_user_group as global variables.
Called y_rand() (for it's seed), y_is_vugen_run() and others dynamically.

\return void
\warning Only call this if you need the y_virtual_user_id and y_virtual_group variables to be set.
Ylib functions that need this will call it when required.
\sa y_rand()
\author Floris Kraak
*/
void y_setup()
{
    // if this is filled y_setup() has already been called.
    if( y_virtual_user_group != NULL )
    {
        return;
    }

    // Loadrunner sets the locale to "", causing scripts running in locales other than en_US to misbehave.
    // Let's set it to something sensible, that actually works for people who don't want to mess with this stuff.
    setlocale(LC_ALL, "C");

    // Global variables, handle with care
    lr_whoami(&y_virtual_user_id, &y_virtual_user_group, &y_scid);
	y_is_vugen_run_bool = y_virtual_user_id == -1;
	
	srand(time(NULL) + y_virtual_user_id + ((int)y_virtual_user_group) & 1023);
}

//! \brief Hook to ensure that ::y_setup() is called at start-up. This allows many performance improvements in the library.
#define vuser_init() vuser_init() { y_setup(); return y_vuser_init(); } y_vuser_init()

/*!
\brief Test if this script is running in vugen (debug mode)
\return 1 if running in vugen, zero otherwise.

Recommended practice:
Use this to create script debugging code that will hit all of the functional code inside the script when run in Vugen, 
but the full (semi-randomized) realistic scenario when it runs as part of a load test.

\note This relies on the y_virtual_user_id_bool variable as an indication of where the script is running.
Inside Vugen that variable should be -1; Otherwise, it contains a non-negative number.
This can be manipulated to your advantage, but may break if HP ever changes that convention.

\b Example:
\code
debug_clickflow()
{
  do_stuff();
}

loadtest_clickflow()
{
   do_more_complicated_stuff();
}

Action()
{
   if( y_is_vugen_run() )
      debug_clickflow();
   else
      loadtest_clickflow();
}
\endcode
\sa y_setup()
*/
#define y_is_vugen_run() y_is_vugen_run_bool

/*! \brief Generate a random (integer) number between 0 and Y_RAND_MAX (30 bit maxint).
\return Random number (integer) between 0 and Y_RAND_MAX: 30-bit maxint - slightly over 1 billion.
\note Superseded by ::y_drand

\b Example:
\code
int random_number;
random_number=y_rand();
\endcode
\author Floris Kraak
\deprecated Superseded by ::y_drand
*/
long y_rand(void)
{
   // Because rand() does not return numbers above 32767 and we want to get at least 30 of the 31 bits
   // of randomness that a long affords us we are going to roll multiple numbers and basically 
   // concatenate them together using bit shifts.
   // 
   // ( If we were to go to 32 bits this function would return negative numbers, which would be undesirable
   // because it will break people's expectations of what rand() does.)
   return rand() << 15 | rand();
}

/*! \brief Generate a random number between 0 <= y_drand() < 1. This supersedes y_rand(). \n
Better distribution of the random numbers over the range than by using y_rand() with modulo (%) -- thus no skewed results.
Equal to Math.random in Java and JavaScript, Random.NextDouble in C#, etc.

\return Random number between 0 and 1 (exclusive).
Accuracy: 9 digits (30 bits).

\b Examples:
\code
if (y_drand() < 0.123) ... -> change 12.3% true
double value = min + y_drand() * (max - min); // to range example, unless max < min
int value = min + y_drand() * (max - min + 1); // for min <= value <= max
\endcode
\author A.U. Luyer
*/
double y_drand(void)
{
   return (rand() << 15 | rand())/1073741824.;
}


/*!
\brief Ylib wrapper for ::malloc()

Allocates a block of memory, but aborts the Vuser if that fails.

\param [in] size Number of bytes required.
\warning The memory resulting from this call will need to be freed using ::free().

\b Example:
\code
char *example_string = "some_text";
int size = strlen(example_string)+1;
char *example_string_copy = y_mem_alloc(size);
snprintf(example_string_copy, size, "%s", example_string);
lr_log_message("Copy of example string contains: %s", example_string_copy);
free(example_string_copy);
\endcode
\sa ::y_array_alloc(), ::malloc()
*/
char *y_mem_alloc(size_t size)
{
    char *buff;
	buff = malloc(size);
    if (!buff)
    {
        lr_error_message("Insufficient memory available, requested %u bytes", size);
        // If this happens you're pretty much screwed anyway.
        lr_abort(); 
    }
    return buff;
}


/*!
\brief Allocates a character array and initializes all elements to zero
As ::y_mem_alloc(), but using the '::calloc' function, rather than '::malloc()'.

\param [in] length Expected number of characters.
\param [in] size How much space a single character requires. Usually this should contain "sizeof char".
\returns A pre-zeroed block of memory of the requisite size allocated using ::calloc().
\warning The memory resulting from this call will need to be freed using ::free().
\sa ::y_mem_alloc(), ::calloc()
*/
char *y_array_alloc(size_t length, size_t size)
{
    char *buff;
	buff = calloc(length, size);
    if (!buff)
    {
        lr_error_message("Insufficient memory available, requested %u * %u bytes", length, size);
        // If this happens you're pretty much screwed anyway.
        lr_abort();
    }
    return buff;
}


/*!
\brief Copy a string into a ::malloc'd piece of memory using ::strdup(), and lr_abort() if the allocation fails.
See the ::strdup() C documentation for what it does. 
This is just a simple wrapper around it that catches the strdup return value and handles any errors by aborting the script.

\param [in] source The string to copy.
\returns A copy of the string, allocated via ::strdup().
\author Floris Kraak
*/
char* y_strdup(char* source)
{
    char* result = strdup(source);
    if (!result)
    {
        lr_error_message("Out of memory while calling strdup()");
        lr_abort();
    }
    return result;
}

/*!
\brief Obtain the string required to fetch the contents of a parameter through lr_eval_string().
\param [in] param_name The parameter name to construct the eval text for.
\returns a char* allocated with y_mem_alloc()
\warning The return argument will need to be freed via a call to free()
\sa y_get_parameter(), y_get_parameter_ext(), y_get_parameter_or_null()
\author Floris Kraak
*/
char* y_get_parameter_eval_string(const char *param_name)
{
    size_t size = strlen(param_name) +3; // parameter name + "{}" + '\0' (end of string)
    char *result = y_mem_alloc( size ); 

    snprintf(result, size, "{%s}", param_name );
    return result;
}

/*!
\brief Test if the given parameter is empty or not yet set.
(These are two different things..)
It would be nice if loadrunner had a builtin for this.
\param [in] param_name The name of the parameter to 
\returns non-zero (true) if the parameter is empty, zero (false) otherwise.
\sa y_get_parameter_eval_string()
\author Floris Kraak
*/
int y_is_empty_parameter(const char *param_name)
{
    char* param_eval_string = y_get_parameter_eval_string(param_name);
    char* param = lr_eval_string(param_eval_string);
    
    int result = *param == 0 || strcmp(param, param_eval_string) == 0;
    free(param_eval_string);

    return result;
}

/*!
\brief Get the content of a parameter and return it as a char *

This is useful mostly for code that wants to manipulate parameter contents but not care about the name of the parameter itself.
(Something which applies to most of ylib ..)

\param [in] param_name The name of the parameter to fetch.
\returns A char* buffer containing the contents of the parameter, allocated by lr_eval_string().
\warning This returns memory allocated by lr_eval_string(). It is freed at the end of the iteration.

\b Example:
\code
char *test;
lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
test=y_get_parameter("TestParam");
lr_message("Test: %s", test);
\endcode
\sa y_is_empty_parameter(), y_get_parameter_ext(), y_get_parameter_or_null()
\author Floris Kraak
*/
char* y_get_parameter(const char* param_name)
{
   char* tmp = y_get_parameter_eval_string(param_name);
   char* parameter = lr_eval_string(tmp);
   free(tmp);
   
   return parameter;
}


/*!
\brief Get the content of a parameter and return it as a char *, or return NULL if it wasn't set.

This will return null in the most typical case: A parameter saved with web_reg_save_param(), but never filled.
The actual check employed here is a test that looks if the parameter content matches the parameter name surrounded by brackets.

If the parameter was never filled, lr_eval_string() will return that. However, in many more elaborate cases we really need to know
if it was never filled to begin with. This function mimics the behaviour we really want to see in LR, but don't have.
(At least, not in LR 11.05, the version I'm working with.)

It would be really nice if there was a loadrunner built-in that did this.

\param [in] param_name The name of the parameter to fetch.
\returns A char* buffer containing the contents of the parameter, allocated by lr_eval_string(), or NULL.
\warning This returns memory allocated by lr_eval_string(). It is likely to disappear (get freed) at the end of the iteration.
\warning If the content of the parameter matches the name of the parameter surrounded by brackets this function will return NULL even if it's not empty.

\b Example:
\code
char *test;
lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
test=y_get_parameter("TestParam");
lr_message("Test: %s", test);
\endcode
\sa y_get_parameter(), y_is_empty_parameter()
\author Floris Kraak
*/
char* y_get_parameter_or_null(const char* param_name)
{
    char* param_eval_string = y_get_parameter_eval_string(param_name);
    char* param = lr_eval_string(param_eval_string);

    int exists = strcmp(param, param_eval_string) != 0; // Result doesn't match the param eval string (eg: '{param}')
    //lr_log_message("y_get_parameter_or_null for param_name %s, pre-eval string is %s, lr_eval_string result is %s, exists: %d", param_name, param_eval_string, param, exists);
    free(param_eval_string);
    //lr_abort();

	return exists ? param: NULL;
}


/*!
\brief Get the content of a parameter and return it as a char * (malloc version)

This is like y_get_parameter(), but the result will use memory allocated with y_mem_alloc(), instead of acquired from lr_eval_string().

\param [in] src_param The name of the parameter to fetch.
\returns A char* buffer containing the contents of the parameter, allocated with y_mem_alloc()
\warning Memory allocated in this manner must be freed using free() or it will linger.

\b Example:
\code
char *test;
lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
test=y_get_parameter_in_malloc_string("TestParam");
lr_message("Test: %s", test);
free(test);
\endcode
\author Floris Kraak
*/
char* y_get_parameter_with_malloc_or_null(const char *src_param)
{
    char *src = y_get_parameter_or_null(src_param);
    //lr_log_message("Copying source data: %s", src);
	return src ? y_strdup(src): NULL;
}

//! \cond function_removal
#define y_get_parameter_malloc_string 0_please_use_y_get_parameter_with_malloc_or_null
//! \endcond

/*!
\brief Get the content of a parameter and return it as a char * (lr_eval_string_ext() version)

Like y_get_parameter, but the result will use lr_eval_string_ext() to acquire it's memory,
rather than getting it from lr_eval_string.
This can be useful when you want your data to remain in memory instead of getting freed at the end of each iteration.
An example is the browser emulation code in y_emulate_browser.c, which sets up a linked list that has to stay allocated throughout the duration of the test.
(And therefore never needs to be freed. But I digress.)

\param [in] source_param The name of the parameter to fetch.
\returns A char* buffer containing the contents of the parameter, allocated with lr_eval_string_ext()
\warning Memory allocated in this manner must be freed using lr_eval_string_ext_free() or it will linger.

\b Example:
\code
char *test;
lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
test=y_get_parameter_ext("TestParam");
lr_message("Test: %s", test);
lr_eval_string_ext_free(test);
\endcode
\author Floris Kraak
\sa y_get_parameter(), lr_eval_string_ext()
*/
char* y_get_parameter_ext(const char *source_param)
{
    char* buffer;
    unsigned long size;
    char* source = y_get_parameter_eval_string(source_param); // Puts the parameter name into parameter seperators { }.
    lr_eval_string_ext(source, strlen(source), &buffer, &size, 0, 0, -1); // Evaluates the result and copy the data into buffer.
    free(source);                                             // Free the intermediate parameter name.
    return buffer;
}

//! \}

#endif // _Y_CORE_C_
