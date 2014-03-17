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
\brief Y-lib core support functions
*/
#ifndef _Y_CORE_C_
//! \cond include_protection
#define _Y_CORE_C_
//! \endcond include_protection

/*!
\file y_core.c
\brief Contains core ylib support functions needed for the functioning of the library.

This file contains two types of functions:
1) Functions that are needed to make the rest of y_lib work properly
2) Functions that are deemed important enough that almost all scripts are expected to use them to some extend or other.
*/

#include "vugen.h"

// Reserved space to hold lr_whoami() output.
int y_virtual_user_id = 0;                         // virtual user id
char* y_virtual_user_group = NULL;                 // virtual user group
int y_scid;                                        // pointer to scenario or session step identifier. See "lr_whoami()";


/*!
\def RAND_MAX
\brief RAND_MAX constant for use with rand() - 15 bits integer.

Loadrunner does not give you full C headers, so the 'RAND_MAX' \#define from <stdlib.h> is missing. 
We define it here mostly for documentation, as we do not have access to the header files themselves and therefore cannot change this. 
\author Floris Kraak
*/
#define RAND_MAX 32767

/*!
\def Y_RAND_MAX 
\brief Alternate RAND_MAX constant for use with y_rand.

y_rand() provides for a far bigger ceiling to the random number generator: 31 bits, instead of 15.
\author Floris Kraak
*/
#define Y_RAND_MAX 1073741823


/*!   
\brief Ylib setup - determines and stores the identity of the virtual user.

This runs lr_whoami and sets y_virtual_user_id and y_virtual_user_group as global variables.
Called y_rand() (for it's seed), y_is_vugen_run() and others dynamically.

\return void
\author Floris Kraak
\warning Only call this if you need the y_virtual_user_id and y_virtual_group variables to be set.
Ylib functions that need this will call it when required.
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
}

/*!
\brief Test if this script is running in vugen (debug mode)
\return 1 if running in vugen, zero otherwise.

Recommended practice:
Use this to create script debugging code that will hit all of the functional code inside the script when run in vugen, 
but the full (semi-randomized) realistic scenario when it runs as part of a loadtest.

\note This relies on the y_virtual_user_id variable as an indication of where the script is running.
Inside vugen that variable should be -1; Otherwise, it contains a non-negative number.
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
*/
int y_is_vugen_run()
{
    y_setup();
    return (y_virtual_user_id == -1);
}


// --------------------------------------------------------------------------------------------------
//// Random number generator control ////


/*!
\brief Generate a random (integer) number between 0 and Y_RAND_MAX (31 bit maxint).
Seeds the random number generator - but only the first time this function is called.
\return Random number (integer) between 0 and Y_RAND_MAX: 31-bit maxint - slightly over 1 billion.

Example:
\code
int random_number;
random_number=y_rand();
\endcode
\author Floris Kraak
*/
long y_rand()
{
   // Have we initialized the random seed yet?
   static int _y_random_seed_initialized = 0;

   if(!_y_random_seed_initialized)
   {
      int seed, tm, rnd;
      y_setup();

      // Seed the random number generator for later use.
      // To make it random enough for our purposes mix in the vuser id and the adress of the vuser group name.
      // In case the script itself already initialized the random number generator, use a random number from 
      // there as well.

      tm = (int)time(NULL);
      rnd = rand();

      //lr_log_message("Seed values - time: %d, y_virtual_user_id: %d, y_virtual_user_group: %d, rand: %d", tm, y_virtual_user_id, (int)y_virtual_user_group, rnd);
      seed = tm%10000 + y_virtual_user_id + ((int)y_virtual_user_group)%1000 + rnd%1000;
      //lr_log_message("Initialising random seed: %d", seed);
      srand( seed );
      _y_random_seed_initialized = 1;
   }

   {
       // Because rand() does not return numbers above 32767 and we want to get at least 30 of the 31 bits
       // of randomness that a long affords us we are going to roll multiple numbers and basically 
       // concatenate them together using bit shifts.
       // 
       // ( If we were to go to 32 bits this function would return negative numbers, which would be undesirable
       // because it will break people's expectations of what rand() does.)

       long result = rand() << 15 | rand(); 
       //lr_log_message("y_rand: 30 random bits = %d, Y_RAND_MAX = %d", result, Y_RAND_MAX);

       // Doing a third call to rand() just to get 1 bit of entropy isn't really efficiënt ..
       //result = (result << 1) | (rand() & 0x0000000000000001); // add another bit and we're done.
       //lr_log_message("y_rand: final random roll = %x, Y_RAND_MAX = %d", result, Y_RAND_MAX);

       return result;
   }
}


/*!
\brief Ylib wrapper for malloc()

Allocates a block of memory.
Adds some simple checks to catch common errors.

\param [in] size Number of bytes required.
\returns A pre-zeroed block of memory of the requisite size allocated using calloc()
\warning The memory resulting from this call will need to be freed using free().

\b Example:
\code
char *example_string = "some_text";
int size = strlen(example_string)+1;
char *example_string_copy = y_mem_alloc(size);
snprintf(example_string_copy, size, "%s", example_string);
lr_log_message("Copy of example string contains: %s", example_string_copy);
free(test);
\endcode
*/
char *y_mem_alloc(const int size)
{
    char *buff;
    int mem = size * sizeof(char);
    
    if(mem <= 0)
    {
        lr_error_message("Requested non positive amounts (%d) of memory! Bailing out ..", mem);
        return NULL;
    }
    //lr_output_message("Dynamic allocation of %d bytes of memory", mem);
    
    if ((buff = (char *)malloc(mem)) == NULL)
    {
        lr_error_message("Insufficient memory available, requested %d", mem);
        // If this happens you're pretty much screwed anyway.
        lr_abort(); 
    }
    return buff;
}


/*!
\brief Allocates a character array and initializes all elements to zero
As y_mem_alloc(), but using the 'calloc' function, rather than 'malloc().
Adds some simple checks to catch common errors.

\param [in] length Expected number of characters.
\param [in] bytesPerChar How much space a single character requires. Usually this should contain "sizeof(char)".
\returns A pre-zeroed block of memory of the requisite size allocated using calloc().
\warning The memory resulting from this call will need to be freed using free().
*/
char *y_array_alloc(int length, int bytesPerChar)
{
    char *buff;
    int size = bytesPerChar ; // * sizeof(char);
    int mem = length * size;

    if(mem <= 0)
    {
        lr_error_message("Requested non positive amounts (%d) of memory! Bailing out ..", mem);
        return NULL;
    }
    //lr_output_message("Dynamic allocation of %d bytes of memory", mem);
    
    if ((buff = (char *)calloc(length, size)) == NULL)
    {
        // Fixme: implement some generic error handling facility to send this stuff to.
        lr_error_message("Insufficient memory available, requested %d", mem);
        // If this happens you're pretty much screwed anyway.
        lr_abort();
    }
    return buff;
}


/*!
\brief Obtain the string required to fetch the contents of a parameter through lr_eval_string().
\param [in] param_name The parameter name to construct the eval text for.
\returns a char* allocated with y_mem_alloc()
\warning The return argument will need to be freed via a call to free()
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
\returns 0 if the parameter is empty, a non-zero number otherwise.
\author Floris Kraak
*/
int y_is_empty_parameter(const char *param_name)
{
    char* param_eval_string = y_get_parameter_eval_string(param_name);
    char* param = lr_eval_string(param_eval_string);
    
    int result = strlen(param) == 0 || strcmp(param, param_eval_string) == 0;
    free(param_eval_string);

    return result;
}

/*!
\brief Get the content of a parameter and return it as a char *

This is useful mostly for code that wants to manipulate parameter contents but not care about the name of the parameter itself.
(Something which applies to most of ylib ..)

\param [in] param_name The name of the parameter to fetch.
\returns A char* buffer containing the contents of the parameter, allocated by lr_eval_string().
\warning This returns memory allocated by lr_eval_string(). It is likely to disappear (get freed) at the end of the iteration.

\b Example:
\code
char *test;
lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
test=y_get_parameter("TestParam");
lr_message("Test: %s", test);
\endcode
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

    if(!exists)
        return NULL;
    else
        return param;
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
@author Floris Kraak
*/
char* y_get_parameter_with_malloc_or_null(const char *src_param)
{
    char *result;
    char *src = y_get_parameter_or_null(src_param);
    //lr_log_message("Copying source data: %s", src);
    if(src == NULL)
        return NULL;

    result = y_mem_alloc( strlen(src) +1);
    strcpy(result, src);

    return result;
    //lr_log_message("Copied result: %s", result);
}

//! \cond function_removal
#define y_get_parameter_malloc_string 0_please_use_y_get_parameter_with_malloc_or_null
//! \endcond function_removal

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




/*!
\brief Get the content of a parameter without embedded null bytes (\0 characters) from the named parameter, if any.
In some cases we want to fetch the content of a parameter but the parameter contains embedded NULL characters which make further processing harder. 
This will fetch a parameter but "cleanse" it from such contamination, leaving the rest of the data unaltered before returning it.

\param [in] param_name The parameter to cleanse of nulls.
\param [in] replacement A character that replaces any embedded nulls found.
\returns The resulting parameter content.

\b Example:
\code
{
   char buffer[11] = { '\0', 'b', '\0', 'r','o', '\0', 'k', 'e', 'n', '\0', '\0' };
   char *tmp;
   lr_save_var(buffer, 11, 0, "broken");
   tmp = y_get_cleansed_parameter("broken", '!');
   lr_log_message("Result: %s", tmp); // Prints "Result: !b!ro!ken!!".
}
\endcode
*/
char* y_get_cleansed_parameter(const char* param_name, char replacement)
{
   char* result;
   unsigned long result_size;
   size_t param_eval_size = strlen(param_name) +3; // parameter name + "{}" + '\0' (end of string)
   char* param_eval_string = y_mem_alloc(param_eval_size);
   //lr_log_message("y_cleanse_parameter(%s)", param_name );

   // Get the contents of the parameter using lr_eval_string_ext() - we can't use the
   // regular version if we expect to find NULL in there.
   snprintf( param_eval_string, param_eval_size, "{%s}", param_name );
   lr_eval_string_ext(param_eval_string, param_eval_size-1, &result, &result_size, 0, 0, -1);
   if( strcmp(param_eval_string, result) == 0 )
   {
       lr_error_message("y_get_cleansed_parameter: Parameter %s does not exist.", param_name);
       lr_abort();
   }
   free(param_eval_string);

   //lr_log_message("Cleansing param %s, result starts with '%-*.*s' and contains %d bytes.", param_name, result_size, result_size, result, result_size);
   {
      size_t result_strlen;
      // Now replace NULL bytes (NULL) in the input with something else..
      for( result_strlen = strlen(result); result_strlen < result_size; result_strlen = strlen(result))
      {
         result[result_strlen] = replacement;
         //lr_log_message("Cleansing param %s, result now '%-*.*s' and contains %d bytes.", param_name, result_size, result_size, result, result_size);
      }
   }
   return result;
}

/*!
\brief Clean a parameter by replacing any embedded NULL (null) characters with a replacement character.

This would normally only happen if you have used to web_reg_save_param() and the result contains one or more null-character(s).
Any such characters are replaced with replacement_char and the result is stored in the original parameter.
When no null-character is found, the result is unaltered.

\param [in] param_name The parameter to cleanse of nulls.
\param [in] replacement A character that replaces any embedded nulls found.
\warning Since this changes existing parameters be careful what types of parameters you use this on.

\b Example:
\code
{
   char buffer[11] = { '\0', 'b', '\0', 'r','o', '\0', 'k', 'e', 'n', '\0', '\0' };
   lr_save_var(buffer, 11, 0, "broken");
   y_cleanse_parameter_ext("broken", '!'); // will save "!b!ro!ken!!" into the "broken" parameter.
}
\endcode
*/
void y_cleanse_parameter_ext(const char* param_name, char replacement)
{
    if( param_name && strlen(param_name) )
    {
        char* result = y_get_cleansed_parameter(param_name, replacement);
        lr_save_string(result, param_name);
        lr_eval_string_ext_free(&result);
    }
    else
    {
        lr_error_message("Empty or NULL parameter name passed to y_cleanse_parameter_ext(): %s", param_name);
        lr_abort();
    }
}

/*!
\brief Clean a parameter by replacing any embedded NULL (null) characters with a space.
This is identical to y_cleanse_parameter_ext() with " " (a single space) selected as the replacement character.

\param [in] param_name The parameter to cleanse of nulls.
\warning Since this changes existing parameters be careful what types of parameters you use this on.

\b Example:
\code
{
   char buffer[11] = { '\0', 'b', '\0', 'r','o', '\0', 'k', 'e', 'n', '\0', '\0' };
   lr_save_var(buffer, 11, 0, "broken");
   y_cleanse_parameter("broken"); // will save " b ro ken  " into the "broken" parameter.
}
\endcode
*/
void y_cleanse_parameter(const char* param_name)
{
    y_cleanse_parameter_ext(param_name, ' ');
}



// --------------------------------------------------------------------------------------------------
#endif // _Y_CORE_C_

