/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2012 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

#ifndef _STRING_C
#define _STRING_C
// --------------------------------------------------------------------------------------------------

/*!
\file y_string.c
\brief Contains low level string and memory manipulation functions, insofar not provided by the C standard.

The philosophy of ylib is that the script engineer should not be required to worry about C-strings and C-like memory manipulation when parameters will suffice.
Most string manipulation functions in the y-lib library take loadrunner parameters as arguments and place their output in one or more of these parameters.
This usually makes it easy to correlate a value (capturing it in a parameter), process it, then pass it on to the next request (again as a parameter).
*/


#include "vugen.h"

/*!
\brief Ylib wrapper for malloc()

Allocates a block of memory.
Adds some simple checks to catch common errors.

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
// --------------------------------------------------------------------------------------------------


/*!
\brief Allocates a character array and initializes all elements to zero

As y_mem_alloc, but using the 'calloc' function, rather than 'malloc().
Adds some simple checks to catch common errors.
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
// --------------------------------------------------------------------------------------------------


/*!
\def y_get_int_from_char 
\brief Convert a *single* character 0-9 to an int

\b Example:
\code
int i =y_get_int_from_char('9');
lr_message("i = %d", i + 1);        // result is "i = 10"
\endcode
\author Floris Kraak
*/
/* int y_get_int_from_char(const char character)
{
    char tmp[2];
    tmp[0] = character;
    tmp[1] = '\0';
    return atoi(tmp);  
}*/
#define y_get_int_from_char(c) (isdigit(c) ? c - ‘0’: 0)

// --------------------------------------------------------------------------------------------------


//
// Determine how much space storing a number in a string requires.
// 
// example:
// {
//    int input = 12345;
//    lr_log_message("Length of %d = %d", input, y_int_strlen(input)); // Prints "Length of 12345 = 5"
//    input = -12345;
//    lr_log_message("Length of %d = %d", input, y_int_strlen(input)); // Prints "Length of -12345 = 6"
//    input = 0;
//    lr_log_message("Length of %d = %d", input, y_int_strlen(input)); // Prints "Length of 0 = 1"
// }
// 
size_t y_int_strlen(int number)
{
    size_t result = 1;
    int power = abs(number);

    // Negative numbers need more space.
    if(number < 0)
        result++;

    // Every power of 10 we need another digit to store the number.
    while(power = (power / 10))
        result++;
    return result;
}


// --------------------------------------------------------------------------------------------------
// Given a parameter name, obtain the string required to fetch the contents of that parameter through
// lr_eval_string().
// 
// Note: the return argument will need to be freed.
// 
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
// 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* y_get_parameter_eval_string(const char *param_name)
{
    size_t size = strlen(param_name) +3; // parameter name + "{}" + '\0' (end of string)
    char *result = y_mem_alloc( size ); 

    snprintf(result, size, "{%s}", param_name );
    return result;
}

// --------------------------------------------------------------------------------------------------
// Test if the given parameter is empty or not yet set (these are two different things..)
// 
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
// 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int y_is_empty_parameter(const char *param_name)
{
    char* param_eval_string = y_get_parameter_eval_string(param_name);
    char* param = lr_eval_string(param_eval_string);
    
    int result = strlen(param) == 0 || strcmp(param, param_eval_string) == 0;
    free(param_eval_string);

    return result;
}


// --------------------------------------------------------------------------------------------------
// Get the content of the parameter named "paramName" and return it as a char *
//
// Todo: Make a derivative function getParameterNoZeroes() which uses lr_eval_string_ext()
// and replaces all \x00 characters in the result with something else, like y_array_get_no_zeroes() does.
// Then every user of this function should start using it.
// Alternative: Make a function that cleans parameters of embedded null bytes and tell users to use that
// before doing anything else with possible tainted values.
// I think I like that one better to be honest. For the Array_ subset it's not that bad given the better
// memory management but for this one .. muh.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//                char *test;
//                lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
//                test=y_get_parameter("TestParam");
//                lr_message("Test: %s", test);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* y_get_parameter(const char* param_name)
{
   char* tmp = y_get_parameter_eval_string(param_name);
   char* parameter = lr_eval_string(tmp);
   free(tmp);
   
   return parameter;
}
// --------------------------------------------------------------------------------------------------


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

// --------------------------------------------------------------------------------------------------
// Get the content of the parameter named "src_param" and return it as a char *
//
// Like y_get_parameter, but the result will use memory allocated with malloc(), rather than gotten
// from loadrunner memory by calling lr_eval_string() on a parameter name.
// 
// WARNING: Memory allocated in this manner MUST be freed using "free()" if called more than once per
// loadtest, or you will run the virtual user out of memory.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//                char *test;
//                lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
//                test=y_get_parameter_in_malloc_string("TestParam");
//                lr_message("Test: %s", test);
//                free(test);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
#define y_get_parameter_malloc_string 0_please_use_y_get_parameter_with_malloc_or_null


// --------------------------------------------------------------------------------------------------
// Get the content of the parameter named "src_param" and return it as a char *
//
// Like y_get_parameter, but the result will use lr_eval_string_ext() underneath, rather than 
// calling lr_eval_string() on a parameter name.
// 
// WARNING: Memory allocated in this manner MUST be freed using "lr_eval_string_ext_free()" or you 
// may run the virtual user out of memory.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//                char *test;
//                lr_save_string("test123", "TestParam");        // save the string "test123" into parameter {TestParam}
//                test=y_get_parameter_ext("TestParam");
//                lr_message("Test: %s", test);
//                lr_eval_string_ext_free(test);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* y_get_parameter_ext(const char *source_param)
{
    char* buffer;
    unsigned long size;
    char* source = y_get_parameter_eval_string(source_param); // Puts the parameter name into parameter seperators { }.
    lr_eval_string_ext(source, strlen(source), &buffer, &size, 0, 0, -1); // Evaluates the result and copy the data into buffer.
    free(source);                                             // Free the intermediate parameter name.
    return buffer;
}

// --------------------------------------------------------------------------------------------------
// Copy a string into a malloc'd piece of memory using strdup(), and lr_abort() if the allocation fails.
//
// See strdup() c++ documentation for what strdup does. This is just a simple wrapper around it.
//
// @author Floris Kraak
char* y_strdup(char* source)
{
    char* result = strdup(source);
    if( result == NULL )
    {
        lr_error_message("Out of memory while calling strdup()");
        lr_abort();
    }
    return result;
}

// --------------------------------------------------------------------------------------------------
// Semi-efficiënt parameter copy using lr_eval_string_ext() with appropriate freeing of memory.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//                lr_save_string("text", "param_a");
//                y_copy_param("param_a", "param_b");   // Results in an exact copy of the content of param_a being stored in param_b.
//                lr_log_message(lr_eval_string("param_b: {param_b}")); // Prints "param_b: text".
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_copy_param(char* source_param, char* dest_param)
{
    char* buffer;
    unsigned long size;
    char* source = y_get_parameter_eval_string(source_param); // Puts the parameter name into parameter seperators { }.
    lr_eval_string_ext(source, strlen(source), &buffer, &size, 0, 0, -1); // Evaluates the result and copy the data into buffer.
    free(source);                              // Free the intermediate parameter name.
    lr_save_var(buffer, size, 0, dest_param);  // Save the result.
    lr_eval_string_ext_free(&buffer);          // Free the buffer.
}

// --------------------------------------------------------------------------------------------------
// In some cases we want to fetch the content of a parameter but the parameter contains embedded NULL
// characters which make further processing harder. This will fetch a parameter but "cleanse" it
// from such contamination, leaving the rest of the data unaltered.
// 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// example usage:
// 
// {
//     char buffer[11] = { '\0', 'b', '\0', 'r','o', '\0', 'k', 'e', 'n', '\0', '\0' };
//     char *tmp;
//     lr_save_var(buffer, 11, 0, "broken");
//     tmp = y_get_cleansed_parameter("broken", '!');
//     lr_log_message("Result: %s", tmp); // Prints "Result: !b!ro!ken!!".
// }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* y_get_cleansed_parameter(const char* param_name, char replacement)
{
   char* result;
   unsigned long result_size;
   size_t param_eval_size = strlen(param_name) +3; // parameter name + "{}" + '\0' (end of string)
   char* param_eval_string = y_mem_alloc(param_eval_size);
   //lr_log_message("y_cleanse_parameter(%s)", param_name );

   // Get the contents of the parameter using lr_eval_string_ext() - we can't use the
   // regular version if we expect to find \x00 in there.
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
      // Now replace NULL bytes (\x00) in the input with something else..
      for( result_strlen = strlen(result); result_strlen < result_size; result_strlen = strlen(result))
      {
         result[result_strlen] = replacement;
         //lr_log_message("Cleansing param %s, result now '%-*.*s' and contains %d bytes.", param_name, result_size, result_size, result, result_size);
      }
   }
   return result;
}
// --------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------
// Clean a parameter by replacing any embedded \x00 (null) characters with replacement_char.
// This would normally only happen if you have used to web_reg_save_param() and the result contains
// one or more null-character(s).
// Any such characters are replaced with replacement_char.
// Since this changes existing parameters be careful what types of parameters you use this on. 
// But when no null-character is found, the result is unaltered.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
// {
//    char buffer[11] = { '\0', 'b', '\0', 'r','o', '\0', 'k', 'e', 'n', '\0', '\0' };
//    lr_save_var(buffer, 11, 0, "broken");
//    y_cleanse_parameter_ext("broken", '!'); // will save "!b!ro!ken!!" into the "broken" parameter.
// }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

// --------------------------------------------------------------------------------------------------
// Clean a parameter by removing any embedded \x00 (null) characters from it.
// Any such characters are replaced with ' '. (space)
// This would only happen if you have used to web_reg_save_param() and the result contains a 
// null-character. 
// Since this changes existing parameters be careful what types of parameters you use this on.
// But when no null-character is found, the result is unaltered.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
// {
//    char buffer[1024] = { 'b', 'r','o', '\0', 'k', 'e', 'n', '\0'};
//    lr_save_var(buffer, 8, 0, "broken");
//    y_cleanse_parameter("broken"); // saves "bro ken " into the parameter called "broken"
// }
// lr_log_message("----");
// {
//    char buffer[1024] = { 'b', 'r','o', '\0', 'k', 'e', 'n', 'Z', 'z', 'Z'};
//    lr_save_var(buffer, 7, 0, "broken");
//    y_cleanse_parameter("broken"); // saves "bro ken" into the parameter called "broken"
// }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_cleanse_parameter(const char* param_name)
{
    y_cleanse_parameter_ext(param_name, ' ');
}


// --------------------------------------------------------------------------------------------------
// Convert the content of a parameter to UPPERCASE. Does not affect non-alphabetic characters.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//            lr_save_string("aBcDeFgHiJ &*45#$@#)!({}", "Test");
//            lr_message(lr_eval_string("Original: {Test}\n"));
//            y_uppercase_parameter("Test");
//            lr_message(lr_eval_string("Altered: {Test}\n"));
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_uppercase_parameter(const char* param_name)
{
    char *result = y_get_parameter_or_null(param_name);
    if(result == NULL)
    {
        lr_error_message("Nonexistant parameter %s passed to y_uppercase_parameter(): Aborting.", param_name);
        lr_abort();
    }
    strupr(result);
    lr_save_string(result, param_name);
}
// --------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------
// Search for a specific substring inside a parameter using left and right boundaries and save that 
// into a new parameter.
// 
// @author André Luyer + Marcel Jepma + Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//   {
//      char* str = "LorumIpsumLipsum";
//      lr_save_string(str, "param");
//      y_substr("param", "param", "Lorum", "Lipsum");
//      lr_log_message(lr_eval_string("{param}")); // Prints "Ipsum".
//   }
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_substr(const char *original_parameter, const char *result_parameter, const char *left, const char *right)
{
    char *p1;
    char *str = y_get_parameter_or_null(original_parameter);
    if( str == NULL )
    {
        lr_error_message("y_substr(): Error: Parameter %s does not exist!", original_parameter);
        lr_abort();
    }

    // zoek start
    if (left) {
        p1 = strstr(str, left);
        if (p1) str = p1 + strlen(left);
        // else start is positie 0...
    }

    // zoek eind
    if (right) {
        p1 = strstr(str, right);
        if (p1) {
            lr_param_sprintf(result_parameter, "%.*s", p1 - str, str); // of sprintf
            return;
        }
    }
    lr_save_string(str, result_parameter);
}


// --------------------------------------------------------------------------------------------------
// Split the string into 2 parts using the search string. Save the left part into the resultParameter.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//            lr_save_string("AstrixObelixIdefix", "Test");
//            lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}=AstrixObelixIdefix
//            y_left( "Test", "Obelix", "Test2" );
//            lr_message(lr_eval_string("New Param: {Test2}\n"));    // {Test2}=Astrix
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_left( const char *original_parameter, const char *search, const char *result_parameter )
{
    char *original = y_get_parameter_or_null(original_parameter);
    if( original == NULL )
    {
        lr_error_message("y_left(): Error: Parameter %s does not exist!", original_parameter);
        lr_abort();
    }
    else if( search == NULL || strlen(search) == 0 )
    {
        lr_save_string(original, result_parameter);
        lr_log_message("Warning: Empty search parameter passed to y_left()");
        return;
    }
    else
    {
        char *buffer;
        char *posPtr = (char *)strstr(original, search);
        int pos = (int)(posPtr - original);
        //lr_log_message("y_left: original=%s, search=%s, resultParam=%s", original, search, resultParam);

        if( posPtr == NULL )
        {
            lr_save_string(original, result_parameter);
            return;
        }
        //lr_log_message("pos = %d", pos);

        // Copy the original to a temporary buffer
        buffer = y_strdup(original);
        buffer[pos] = '\0'; // make the cut
        lr_save_string(buffer, result_parameter); // save the result
        free(buffer);
    }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Split the string into 2 parts, using the search string. Save the right part into the 
// resultParameter.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//            lr_save_string("AstrixObelixIdefix", "Test");
//            lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}=AstrixObelixIdefix
//            y_right( "Test", "Obelix", "Test4" );
//            lr_message(lr_eval_string("New Param: {Test4}\n"));    //    {Test4}=Idefix
//
//    note: previous name: head() and  y_head()
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_right( const char *original_parameter, const char *search, const char *result_parameter)
{
    char* original = y_get_parameter_or_null(original_parameter);
    if( original == NULL )
    {
        lr_error_message("y_right(): Error: Parameter %s does not exist!", original_parameter);
        lr_abort();
    }
    else if( search == NULL || strlen(search) == 0 )
    {
        lr_save_string(original, result_parameter);
        lr_log_message("Warning: Empty search parameter passed to y_right()");
        return;
    }
    else
    {
        char* posPtr = (char *)strstr(original, search);
        //int pos = (int)(posPtr - original);
        //lr_log_message("y_right: original=%s, search=%s, resultParam=%s", original, search, result_parameter);
    
        if( posPtr == NULL )
        {
            lr_save_string(original, result_parameter);
            return;
        }
        //lr_log_message("pos = %d", pos);
        posPtr = posPtr + strlen(search);
        lr_save_string(posPtr, result_parameter);
    }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Same as y_right(), but don't stop at the first match, rather use the last match instead.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//            lr_save_string("WackoYackoDotWarner", "Test");
//            lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}=WackoYackoDotWarner
//            y_last_right( "Test", "ot", "Test2" );
//            lr_message(lr_eval_string("New Param: {Test2}\n"));
//
//    note: previous name: tail_tail()
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_last_right( const char *original_parameter, const char *search, const char *result_parameter)
{
    char *result = y_get_parameter(original_parameter);
    if( result == NULL )
    {
        lr_error_message("y_last_right(): Error: Parameter %s does not exist!", original_parameter);
        lr_abort();
    }
    else if( search == NULL || strlen(search) == 0 )
    {
        lr_save_string(result, result_parameter);
        lr_log_message("Warning: Empty search parameter passed to y_last_right()");
        return;
    }
    else
    {
        char *posPtr;
        //lr_log_message("y_last_right: original=%s, search=%s, resultParam=%s", original, search, resultParameter);
        do 
        {
            posPtr = (char *)strstr(result, search);
            //pos = (int)(posPtr - result);
            //lr_log_message("pos = %d", pos);
    
            // not found, save what we have as the result.
            if( posPtr == NULL )
            {
                lr_save_string(result, result_parameter);
                return;
            }
            // found, update the result pointer and go find more..
            result = posPtr + strlen(search);
        } 
        while(1);
    }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// y_split a string in two based on a search parameter.
//
// Note: Unlike the others this one does not use parameter, but raw char pointers instead.
// This mostly to accomodate the primary user - y_array_split();
// Both output buffers need to be pre-allocated! 
//
// For the parameter version use y_split().
//
// @author Floris Kraak
//
void y_split_str( const char *original, const char *separator, char *left, char *right)
{
    char *buffer;
    char *posPtr = (char *)strstr(original, separator);
    int pos = (int)(posPtr - original);;

    //lr_log_message("y_split_str: original=%s, search=%s", original, search);

    if( posPtr == NULL )
    {
        // Copy the original to the left hand output buffer.
        strncpy(left, original, strlen(original)+1); // Let's not forget to copy the null byte, too.
        return;
    }
    //lr_log_message("pos = %d", pos);

    // Copy the left hand using pos bytes from the original
    strncpy(left, original, pos);
    left[pos] = '\0'; // make the cut by putting a null character at the end.

    // Copy the right hand side starting from the position just after the found string.
    {
        char *start = posPtr + strlen(separator);
        strncpy(right, start, strlen(start)+1); // Let's not forget to copy the null byte, too.
    }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// y_split a string in two based on a seperating string.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//            lr_save_string("WackoYackoDotWarner", "Test");
//            lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}    = WackoYackoDotWarner
//            y_split("Test", "Yacko", "Left", "Right");           // Use "Yacko" as the separator
//            lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}    = WackoYackoDotWarner
//            lr_message(lr_eval_string("Left    : {Left}\n"));    // {Left}    = Wacko
//            lr_message(lr_eval_string("Right   : {Right}\n"));   // {Right}   = DotWarner
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_split( const char *originalParameter, const char *separator, const char *leftParameter, const char *rightParameter)
{
    char *item = y_get_parameter(originalParameter);
    int len = strlen(item);

    if( len < strlen(separator) )
    {
        // Since the separator doesn't even fit in the item, searching is pointless.
        // Store the original in the left hand parameter, the right hand is empty.
        lr_save_string(originalParameter, leftParameter);
        lr_save_string("", rightParameter);
        return;

    }
    else
    {

        // Left hand side
        // If the separator isn't found the full original string gets stored here.
        // Don't forget the 0 byte at the end though..
        char *left = y_mem_alloc(len+1);

        // Right hand side
        // If the separator gets found in position 1 the remainder of the
        // original string gets stored here (and left gets a zero-length string).
        char *right = y_mem_alloc(len-strlen(separator)+1);

        // Start off with zero-length strings. We can safely assume
        // both variables contain garbage when freshly allocated.
        left[0] = '\0';
        right[0] = '\0';

        // This is where the magic happens.
        y_split_str(item, separator, left, right);

        // Store the results in parameters.
        lr_save_string(left, leftParameter);
        free(left);
        lr_save_string(right, rightParameter);
        free(right);
    }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Remove leading and trailing whitespace from a parameter. Does not support Unicode so use with care.
//    Whitespace can be: " "(=space)        "\r"(=carrige return)    "\n"(=line feed)    "\t"(=tab)
// @author: Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:
//            lr_save_string("  WackoYackoDot ", "Test");
//            lr_message(lr_eval_string("Original: >{Test}<\n"));    // {Test}= "  WackoYackoDot "
//            y_chop("Test");
//            lr_message(lr_eval_string("Original: >{Test}<\n"));    // {Test}= "WackoYackoDot"
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_chop( const char* parameter )
{
    char *result;
    int i = 0;
    char character;
    
    //lr_output_message( "y_chop(%s)", parameter);    
    result = y_get_parameter(parameter);

    // y_chop leading whitespace
    character = result[i];
    while( (character == ' ') || (character == '\r') || (character == '\n') || (character == '\t') )
    {
        character = result[++i];
    }
    result += i;
    
    //lr_output_message("result after removal of leading whitespace: %s", result);
    
    // y_chop trailing whitespace
    i = strlen(result)-1;    
    character = result[i];
    while( (i >= 0) &&
           (  (character == ' ' ) ||
              (character == '\r') ||
              (character == '\n') ||
              (character == '\t') ) )
    {
        character = result[--i];
    }
    result[i+1] = '\0';
    
    lr_save_string(result, parameter);
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Search and replace.
// Find 'search' in 'parameter' and replace it with 'replace'.
// Replaces the originally passed-in parameter with the new one.
//
// Note: This one has a built-in search/y_replace limit when
// search > y_replace. It won't do it more than 'limit' times.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:     
//             lr_save_string("test123", "par1");
//          y_replace("par1", "1", "ing1");        // {par1} now has the value testing123
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_replace( const char *parameter, const char *search, const char *replace)
{
   int slen, rlen, plen;      // lengte van search, replace, en basis string
   int i = 0;                 // aantal replacements
   int limit = 1000;          // replacement limiet als replace > search

   char *c;                   // punt waar change moet beginnen
   char *cend;                // einde van de change = c+slen
   char *last;                // de \0 slotbyte van input string
   char *buffer;              // buffer voor bewerkingen
   char *string;              // originele string waar we in zoeken

   if ((search == NULL)     || (strlen(search) <1)) 
       return;
   if (!search || !replace)      return;   // ongeldige search of_replace
   if (!strcmp(search, replace)) return;   // search == replace: geen wijziging

   slen = strlen (search);          // de lengte van search
   rlen = strlen (replace);         // de lengte van replace

   string = y_get_parameter(parameter); // <-- memory allocated by loadrunner, too small if replace > search
   plen = strlen(string);

   //lr_log_message("y_replace(%s, %s, %s) - slen %d, rlen %d, plen %d", parameter, search, replace, slen, rlen, plen);
    
   if ( rlen > slen)
   {
      // Reserve memory for -limit- replacements.
      size_t size = plen + ((rlen-slen) * limit);
      buffer = y_mem_alloc( size );
      snprintf( buffer, size, "%s", string );
   }
   else
   {
      // No need to reserve memory
      buffer = string;
   }

   last = buffer + strlen(buffer) +1;      // het einde van de string, de null byte
   c = buffer;                             // initialiseer c - buffer moet intact blijven

   while (c = (char*) strstr(c, search))   // doorzoek de search string vanaf waar je gebleven bent
   {
      //lr_output_message("c:%s", c);
 
      i++;
      if (slen != rlen)                    // zijn search en replace van verschillende lengte?
      {
         if( i >= limit )
         {
            lr_log_message("Unable to handle more than %d search-and-replaces, apologies for the inconvenience.", limit);
            break;
         }
         cend = c+slen;                        // cend is de plaats van waaraf de string moet opschuiven
         //lr_output_message("memmove(dest=%d, src=%d, size=%d)", (long)(c+rlen), (long)(cend), (long)(last-cend) );
         memmove (c+rlen , cend, last-cend);   // verplaats de rest van de string - als de geheugenberekening niet klopt gaat dit stuk!
         //lr_output_message("memmove completed, result = %s", c);
         last = last - slen + rlen;            // en bereken het nieuwe eindpunt
      }
      memmove(c, replace, rlen);  // execute the replacement
      c += rlen;                  // start the next search at the point where the replacement ended
   }
   lr_save_string(buffer, parameter);

   if( rlen > slen )
   {
      free(buffer);
   }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Lightweight alternative to the y_replace() function.
// Remove all occorrances of 'removeMe' in the parameter named 'paramName'
// Stores the result in the original parameter.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        example usage:     
//          lr_save_string("test123", "par1");
//          y_remove_string_from_parameter("par1", "1");   // {par1} now has the value test23
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_remove_string_from_parameter(const char* paramName, const char* removeMe)
{
   char *parameter;
   char *removePtr;
   int remlen;
 
   //lr_log_message("y_remove_string_from_parameter( remove:%s, parameter:%s )", removeMe, paramName);

   if(!removeMe || !*removeMe)
      return;

   // fetch the contents of the parameter to change
   parameter = y_get_parameter(paramName);
   // removePtr is used to track our progress through this string
   removePtr = parameter;
   remlen = strlen(removeMe);

   // while we find occurrances of the string we're looking for
   while ( removePtr = (char *)strstr( removePtr, removeMe ) )
   {
      // copy the characters between the end of the data we wish to remove and end-of-string 
      // to the place where we found our offending content.
      char* origin = removePtr + remlen;
      strncpy(removePtr, origin, (strlen(origin)+1));
   }

   // store it in the original parameter
   lr_save_string( parameter, paramName );
}
// --------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------
#endif // _STRING_C

