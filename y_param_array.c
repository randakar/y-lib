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
/*! 
\file y_param_array.c
\brief This file contains loadrunner parameter array helper functions.

This file provides functions that make using and manipulating loadrunner parameter arrays easier.
A parameter array is a set of loadrunner parameters that is usually created by web_reg_save_param() with the "Ord=All" argument.
These parameters all use the same name, postfixed with a number to indicate the index into the array.

\note Some historical functions now have loadrunner equivalents, starting with LoadRunner version 9. 
Those functions have been replaced with macros that will call the correct modern equivalent instead. 
If Y_COMPAT_LR_8 is enabled the old behaviour is restored.
If you still use loadrunner versions earlier than 9 you may need to add the line below to your script:
\#define Y_COMPAT_LR_8

\sa lr_paramarr_random(), lr_paramarr_idx()
*/
#ifndef _Y_PARAM_ARRAY_C_
//! \cond include_protection
#define _Y_PARAM_ARRAY_C_
//! \endcond

#include "vugen.h"
#include "y_string.c"
#include "y_loadrunner_utils.c"

int _y_random_array_index = 0;


/*! \def y_array_count
\brief Determine the number of elements in the target parameter array.
\param [in] param_array_name The name of the parameter array.
\return The number of elements in the array.

\note Superseded by the LR 9 function lr_paramarr_len(). 
If Y_COMPAT_LR_8 is not defined this function is replaced with a simple macro calling the new loadrunner equivalent.

\b Example:
\code
{
    web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
	web_url("URL=www.google.nl", LAST);
    lr_log_message("RESULT: %d", y_array_count("TAG")); // Logs the text "RESULT: " followed by how many hyperlinks are found on the www.google.nl index page.
}
\endcode
\see lr_paramarr_len()
\author Floris Kraak
*/
#ifdef Y_COMPAT_LR_8
int y_array_count( const char *param_array_name )
{
    int result;
    size_t size = strlen(pArrayName) +9; // 9 = strlen("{}_count") +1 -- the +1 is '\0'
    char *tmp = y_mem_alloc(size);

    snprintf(tmp , size, "{%s_count}" , param_array_name );
    result = atoi(lr_eval_string(tmp));
    free(tmp);
    return result;
}
#else
#define y_array_count( param_array_name ) lr_paramarr_len(param_array_name) 
#endif // Y_COMPAT_LR_8


/*! \def y_array_get
\brief Fetch the content of a specific element from a parameter list based on index.
\param [in] pArray The name of the parameter array.
\param [in] pIndex The index of the chosen element.
\return A char* pointer pointing to the content of the chosen element, if it exist. Will call lr_abort() if it doesn't.

\note Superseded by the LR 9 function lr_paramarr_idx(). 
If Y_COMPAT_LR_8 is not defined this function is replaced with a simple macro calling the new loadrunner equivalent.

\note If the data inside the parameter contains embedded null (\\x00) characters you may have an issue processing the return value. 
\see y_array_get_no_zeroes()

\b Example:
\code
{
    web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
    web_url("URL=www.google.nl", LAST);
    lr_log_message("Fourth tag on www.google.nl: %s", y_array_get("TAG", 4));
}
\endcode
\see lr_paramarr_idx()
\author Floris Kraak
*/
#ifdef Y_COMPAT_LR_8
char *y_array_get( const char *pArray, const int pIndex )
{
    int size = y_array_count( pArray );
    char *tmp;
    char *result;

    //lr_log_message("y_array_get(%s,%d)", pArray, pIndex ); 
    if ( (pIndex > size) || (pIndex < 1) )
    {
        lr_error_message("Index out of bounds");
        lr_abort();
		return NULL;
    }

    // Calculate space requirements
    {
        size_t bufsize = strlen(pArray)+y_int_strlen(pIndex)+4; // strlen() + size of index + {}_\0
        tmp = y_mem_alloc(bufsize); 
        snprintf( tmp , bufsize, "{%s_%d}" , pArray , pIndex );
    }

    result = lr_eval_string(tmp);
    free (tmp);
    return result;
}
#else
#define y_array_get( pArray, pIndex ) lr_paramarr_idx(pArray, pIndex)
#endif // Y_COMPAT_LR_8


/*! \brief Get the content the nth member of target parameter array, but without embedded zeroes.

As y_array_get(), but it filters embedded zeroes from the input, replacing them with a single space: ' '.
It's not ideal, but better than having your script break on this particular type of broken web page.

\warning the return value of this function needs to be freed using lr_eval_string_ext_free().

\param [in] pArray The name of the parameter array to get an element from.
\param [in] pIndex The index number of the element to fetch.
\returns The parameter value at index pIndex in the target parameter array.

\warning the return value of this function needs to be freed using lr_eval_string_ext_free().

\b Example:
\code
{
   char *result;
   lr_save_string("bar", "foo_1");
   lr_save_string("baz", "foo_2");
   lr_save_int(2, "foo_count");

   result = y_array_get_no_zeroes("foo", 1);
   lr_log_message("Result: %s", result); // Prints "Result: bar" 
   free(result);
   result = y_array_get_no_zeroes("foo", 2);
   lr_log_message("Result: %s", result); // Prints "Result: baz"
   free(result);
}
{
   char buffer[11] = { '\0', 'b', '\0', 'r','o', '\0', 'k', 'e', 'n', '\0', '\0' };
   char *tmp;
   lr_save_var(buffer, 11, 0, "broken_1");
   y_array_save_count(1, "broken");
   tmp = y_array_get_no_zeroes("broken", 1);
   lr_log_message("Result: '%s'", tmp); // prints "Result: ' b ro ken  '"
   free(tmp);
}
\endcode

\see y_array_get(), lr_paramarr_idx(), y_int_strlen(), lr_eval_string_ext()
\author Floris Kraak
*/
char* y_array_get_no_zeroes( const char *pArray, const int pIndex )
{
    if ( (pIndex > y_array_count(pArray)) || (pIndex < 1) )
    {
        lr_error_message("Parameter array %s does not exist or index %d out of bounds.", pArray, pIndex);
        lr_abort();
        return NULL;
    }
    else
    {
        // Calculate space requirements
        size_t bufsize = strlen(pArray) + y_int_strlen(pIndex) +2; // strlen() + _\0
        char* tmp = y_mem_alloc(bufsize);
        snprintf(tmp, bufsize, "%s_%d", pArray, pIndex );
        return y_get_cleansed_parameter(tmp, ' '); // <-- Might want to make that configurable..
    }
}

/*! \brief Save a string value into an array at a specified position

This does not update the size of the array.

\param [in] value The value to store
\param [in] pArray The name of the array to store the value in
\param [in] pIndex The index of the element to be updated.

\b Example:
\code
web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
web_url("URL=www.google.nl", LAST);
y_array_save("newvalue", "TAG", 2);                    // save the sting "newvalue" into {TAG_2}
lr_log_message("Value: %s", y_array_get("TAG", 2));    // print the value of {TAG_2}. (will be "newvalue" in this example)
\endcode

\see y_array_get(), y_array_count()
\author Floris Kraak
*/
void y_array_save(const char* value, const char* pArray, const int pIndex)
{
    if(pArray == NULL)
    {
        // In some cases calling this without a parameter name is entirely valid.
        // eg. y_split() on an array where you only need one half of the results.
        return;
    }
    else
    {
        int len = strlen(pArray) + y_int_strlen(pIndex) +2; // +2 = _\0
        char *result = y_mem_alloc(len);
        snprintf(result, len, "%s_%d", pArray, pIndex);
        lr_save_string(value, result);
        free(result);
    }
}

/*! Change the size of the target parameter array.

Updates the _count field of the chosen parameter array, changing the reported size of the array.
\note This does not verify if the array in question actually contains that number of elements.

\param [in] count The new size of the array.
\param [in] pArray The target array to resize.

\see y_array_save()
\author Floris Kraak
*/
void y_array_save_count(const int count, const char *pArray)
{
    if( pArray == NULL )
    {
        // In some cases calling this without a parameter name is entirely valid.
        // eg. y_split() on an array where you only need one half of the results.
        return;
    }
    else 
    {
        int len = strlen(pArray) +7; // 7 = strlen("_count") +1, where +1 would be the '\0' byte at the end.
        char* result = y_mem_alloc(len);

        snprintf(result, len, "%s_count", pArray);
        lr_save_int(count, result);
        free(result);
    }
}

/*! Add a new element to the end of the target parameter array.

\note This will call y_array_save_count() each time it is called. For bulk inserts this should not be used - the performance will suck.

\param [in] pArray The target array to resize. If this array does not exist, a new one will be created.
\param [in] value the value to add.

\b Example:
\code
web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
web_url("URL=www.google.nl", LAST);
y_array_add("TAG", "newValue");        // the added value (=last one) in {TAG} is now "newValue".
\endcode

\see y_array_save()
\author Floris Kraak
*/
void y_array_add( const char* pArray, const char* value )
{
    int size = y_array_count(pArray) +1;
    y_array_save(value, pArray, size);
    y_array_save_count(size, pArray);
}

/*! \brief Concatenate two arrays together and save the result into a third array.

\b Example:
\code 
web_reg_save_param("TAG1", "LB=<a h", "RB=>", "ORD=ALL", LAST);  
web_reg_save_param("TAG2", "LB=<A id", "RB=>", "ORD=ALL", LAST);
web_url("URL=www.google.nl", LAST);
                                         // Loadrunner saves parameters:
                                         // TAG1 - "TAG1_1" to "TAG1_12", "TAG1_count" = 12
                                         // TAG2 - "TAG2_1" to "TAG2_14", "TAG2_count" = 14
y_array_concat("TAG1", "TAG2", "TAG");   // saves "TAG_1" to "TAG_26", "TAG_count" = 26
\endcode

\see y_array_save()
\author Floris Kraak
*/
void y_array_concat(const char *pArrayFirst, const char *pArraySecond, const char *resultArray)
{
    int size_first = y_array_count(pArrayFirst);
    int size_second = y_array_count(pArraySecond);
    int size_total = size_first + size_second;
    int i = 1;
    int j = 1;

    //lr_log_message("y_array_concat(%s, %s, %s)", pArrayFirst, pArraySecond, resultArray);
    //lr_log_message("size_total = %d, i = %d", size_total, i);

    for(i=1; i <= size_total; i++)
    {
        char *value;
        //lr_log_message("Iteration %i, j=%d", i, j);

        if( i <= size_first )
        {
            value = y_array_get_no_zeroes(pArrayFirst,i);
        }
        else
        {
            value = y_array_get_no_zeroes(pArraySecond,j);
            j++;
        }

        y_array_save(value, resultArray, i);
        lr_eval_string_ext_free(&value);
    }
    y_array_save_count(size_total, resultArray);
}


/*! \brief Get a random element from a parameter list

Fetch the contents of a random element in a parameter array.
As lr_paramarr_random(), but stores the rolled index number internally.

\param [in] pArray The name of the parameter array to fetch a random value from.
\return The value of the randomly chosen parameter.

\deprecated This doesn't really do anything that lr_paramarr_random() doesn't already do.

\sa lr_paramarr_random(), y_rand()
\author Floris Kraak
*/
char *y_array_get_random( const char *pArray )
{
    int count = y_array_count( pArray );
    if( count < 1 )
    {
        lr_log_message("No elements found in parameter array!");
        return NULL;
    }

    _y_random_array_index= (y_rand() % count) +1;
    return y_array_get(pArray, _y_random_array_index);
}


/*! \brief Get a random element from a parameter list without embedded zeroes.

Fetch the contents of a random element in a parameter array, filtering out any embedded zeroes.
Stores the rolled index number internally.

\warning the return value of this function needs to be freed using lr_eval_string_ext_free().

\param [in] pArray The name of the parameter array to fetch a random value from.
\return The value of the randomly chosen parameter, minus any embedded \\x00 characters.

\sa lr_paramarr_random(), y_array_get_random(), y_rand()
\author Floris Kraak
*/
char *y_array_get_random_no_zeroes( const char *pArray )
{
    int count = y_array_count( pArray );
    if( count < 1 )
    {
        lr_log_message("No elements found in parameter array!");
        return NULL;
    }

    _y_random_array_index = (y_rand() % count) +1;
    return y_array_get_no_zeroes(pArray, _y_random_array_index);
}


/*! \brief Choose an element at random from a saved parameter list and store it in a parameter with the same name.

Fetch the contents of a random element in a parameter array, filtering out any embedded zeroes, and saves it as a parameter with the same name.

\param [in] pArray The name of the parameter array to fetch a random value from.
\return The index number of the randomly chosen parameter.

\sa lr_paramarr_random(), y_array_get_random_no_zeroes(), y_rand()
\author Floris Kraak
*/
int y_array_pick_random( const char *pArray )
{
    if(y_array_count(pArray))
    {
        char *result = y_array_get_random_no_zeroes(pArray);
        lr_save_string(result, pArray);
        lr_eval_string_ext_free(&result);
        return _y_random_array_index;
    }
    else
    {
        lr_save_string("", pArray);
        //lr_output_message("y_array_pick_random(): Unknown parameter list");
        return 0;
    }
}

/*! \brief Dump the contents of a list of saved parameters to standard output (the run log)

\param [in] pArrayName The name of the array to log.

\b Example:
\code
web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
web_url("URL=www.google.nl", LAST);
y_array_dump("TAG"); // Prints the content of the parameter array named "TAG" to the output log.
\endcode
\author Raymond de Jongh
*/
void y_array_dump( const char *pArrayName )
{
    int i;
    int count;

    for ( i=1 ; i <= y_array_count(pArrayName); i++ )
    {
        char *msg = y_array_get_no_zeroes(pArrayName, i);
        lr_output_message("{%s_%d} = %s" , pArrayName, i, msg);
        lr_eval_string_ext_free(&msg);
    }
}


/*! \brief Create a parameter list from a single parameter value.

As if using web_reg_save_param() with "Ord=All" as an option, but for already saved parameters instead of web requests.
This is especially useful when there is a need to save lists of parameters from sections of the application output instead of the entire thing.

For example, HTML dropdown boxes look like this:
\code
<select>
  <option value="volvo">Volvo</option>
  <option value="saab">Saab</option>
  <option value="mercedes">Mercedes</option>
  <option value="audi">Audi</option>
</select>
\endcode

These cannot be correlated using just `<option>` and `</option>` as the left and right boundaries if there is more than one dropdown box on a page.
Using this function you can save the HTML for the entire dropdown box in a parameter instead, then have the options saved as a parameter list afterwards.

\warning Using this for large lists can be really slow. This is running lr_save_string() in a loop, and lr_save_string() will slow down if you use it a few thousand times.

\todo Write a wrapper around this - let's call it 'y_dropdown()' specifically for HTML dropdown boxes, as that is the most common case.

\param [in] sourceParam The name of a single parameter containing a list of elements. See dropdown example, above.
\param [in] LB The left boundary of the values you wish to save into a list. Example: `<option>`
\param [in] RB The right boundary of the values you wish to save into a list. Example: `</option>`
\param [in] destArrayParam The name of the parameter array to save the values into.

\b Example:

\code
lr_save_string("<option value=\"water\"><option value=\"fire\"><option value=\"burn\">", "SOURCE");
y_array_save_param_list("SOURCE", "value=\"", "\">", "VALUES");
y_array_dump("VALUES");    // {VALUES_1} contains "water" (no quotes)    {VALUES_2} contains "fire" (no quotes)    etc...
\endcode

\author Floris Kraak
*/
void y_array_save_param_list(const char *sourceParam, const char *LB, const char *RB, const char *destArrayParam)
{
    int i = 0;
    char *source = y_get_parameter(sourceParam);
    int buflen = strlen(source)+1;
    char *buffer = y_mem_alloc(buflen);
    char *next = buffer;
    memcpy(buffer, source, buflen);

    while( next = (char *)strstr(next, LB) )
    {
        char* end = strstr(next, RB);
        if(!end) 
            break;
        buffer[end - buffer] = '\0';

        i++;
        y_array_save(next+strlen(LB), destArrayParam, i);
        next = end + strlen(RB);
    }
    free(buffer);
    y_array_save_count(i, destArrayParam);
}

/*! \brief Search a parameter array for a specific text and build a new array containing only parameters containing that text.

Let's just call it 'grep'. :)

\param [in] pArrayName The name of the array to be searched.
\param [in] search The string to search for.
\param [in] resultArrayName The name of the array to hold the resulting values. Can be the same as the original parameter array.

\b Example:
\code
lr_save_string("<apple><balloon><crayon><drum>", "SOURCE");
y_array_save_param_list("SOURCE", "<", ">", "VALUES");
y_array_grep("VALUES", "r", "VALUES2");   // get all elements containing "r" (crayon and drum)
y_array_dump("VALUES2");
\endcode

\see y_array_filter(), y_array_concat(), y_array_pick_random()
\author Floris Kraak
*/
void y_array_grep( const char *pArrayName, const char *search, const char *resultArrayName)
{
    int i, j = 1;
    char *item;
    int size = y_array_count(pArrayName);

    for( i=1; i <= size; i++)
    {
        item = y_array_get_no_zeroes(pArrayName, i);
        if( strstr(item, search) )
        {
            y_array_save(item, resultArrayName, j++);
        }
        lr_eval_string_ext_free(&item);
    }
    y_array_save_count(j-1, resultArrayName);
}

/*! \brief Search a parameter array for a specific string and and build a new result array containing only parameters NOT containing the string.

As y_array_grep(), but reversed.

\param [in] pArrayName The name of the array to be searched.
\param [in] search The string to search for.
\param [in] resultArrayName The name of the array to hold the resulting values. Can be the same as the original parameter array.

\b Example:
\code
lr_save_string("<apple><balloon><crayon><drum>", "SOURCE");
y_array_save_param_list("SOURCE", "<", ">", "VALUES");
y_array_filter("VALUES", "r", "VALUES2");   // get all elements NOT containing "r" (apple, balloon)
y_array_dump("VALUES2");
\endcode

\see y_array_grep(), y_array_concat(), y_array_pick_random()
\author Floris Kraak
*/
void y_array_filter( const char *pArrayName, const char *search, const char *resultArrayName)
{
    int i, j = 1;
    char *item;
    int size = y_array_count(pArrayName);

    for( i=1; i <= size; i++)
    {
        item = y_array_get_no_zeroes(pArrayName, i); // Some pages contain a null byte - \x00 in the input. Ugh.
        if( strstr(item, search) == NULL )
        {
            y_array_save(item, resultArrayName, j++);
        }
        lr_eval_string_ext_free(&item);
    }
    y_array_save_count(j-1, resultArrayName);
}


/*! Merge two parameter arrays into a single array. 

\warning The source parameter arrays have to be of the same length.

The resulting list contains each item from the left array appended to the item with the same index in the right array, with an optional glue separator in the middle for convenient re-splitting later.

This thing is mostly created to facilitate situations where you have two lists of parameters that contain data that is related to each other.
Example: Hyperlinks consist of a hyperlink and a title, and each of these is captured seperately. 
Since the entries in these lists are closely correlated picking one involves correlating entries in the hyperlink list with the entries in the title list.

With this function you can just glue the two lists together based on their index and continue processing from there.

\param [in] pArrayNameLeft The parameter array to use for the lefthand side of the concatenations.
\param [in] pArrayNameRight The parameter array to use for the righthand side of the concatenations.
\param [in] separator A fixed string to be used as a seperator between the two values.
\param [in] resultArray The name of the array to hold the resulting values. Can be the same as either the left or the righthand parameter array.


\b Example:
\code
lr_save_string("<apple><balloon><crayon><drum>", "THING");
lr_save_string("<fruit><toy><art><music>", "CAT");
y_array_save_param_list("THING", "<", ">", "THING2");    //    {THING2} contains "balloon" (no quotes)
y_array_save_param_list("CAT", "<", ">", "CAT2");        //    {CAT2} contains "toy"
y_array_merge("THING2", "CAT2", "=>", "RESULT");         //    {RESULT_2} now contains balloon=>toy
y_array_dump("RESULT");
\endcode

\see y_array_split(), y_array_concat(), y_array_pick_random()
\author Floris Kraak
*/
int y_array_merge(const char *pArrayNameLeft, const char *pArrayNameRight, const char *separator, const char *resultArray)
{
    int i = 1;
    char *param;
    int length = y_array_count(pArrayNameLeft);
    int seperator_size = strlen(separator);

    if( length != y_array_count(pArrayNameRight) )
    {
        // If the sizes aren't the same there's a good chance numbers won't line up on both sides.
        // We definitely don't want to end up with records merged that don't actually correspond to each other!
        lr_error_message("Unable to merge arrays %s and %s - sizes unequal!", pArrayNameLeft, pArrayNameRight);
        lr_abort();
        return 0;
    }

    for( i=1; i <= length; i++)
    {
        char *left = y_array_get_no_zeroes(pArrayNameLeft, i);
        char *right = y_array_get_no_zeroes(pArrayNameRight, i);
        size_t size = strlen(left)+seperator_size+strlen(right)+1;
        char *result = y_mem_alloc(size);

        snprintf(result, size, "%s%s%s", left, separator, right);
        lr_eval_string_ext_free(&left);
        lr_eval_string_ext_free(&right);
        y_array_save(result, resultArray, i);
        free(result);
    }
    y_array_save_count(i-1, resultArray);
    return 1;
}




/*! Split an input array vertically into two new arrays, based on a search parameter.

This is the reverse of y_array_merge(). It will examine each parameter in turn and save each value into two separate parameter lists.

\note Using the same array for the left and right hand side result arrays will result in one side overwriting the values from the other side.

\param [in] pInputArray The name of the array holding the values to be split.
\param [in] separator A fixed string to be used as a seperator between the two values.
\param [in] pArrayNameLeft The parameter array to use for the lefthand side of the concatenations. Can be the same as the input array.
\param [in] pArrayNameRight The parameter array to use for the righthand side of the concatenations. Can also be the same as the input array.

\see y_array_merge(), y_array_concat(), y_array_pick_random(), y_split_str()
\author Floris Kraak
*/
void y_array_split(const char *pInputArray, const char *separator, const char *pArrayNameLeft, const char *pArrayNameRight)
{
    int i = 1;
    int size = y_array_count(pInputArray);

    //lr_log_message("y_array_split(%s, %s, %s, %s)", pInputArray, separator,pArrayNameLeft, pArrayNameRight);

    for( i=1; i <= size; i++)
    {
        char *item = y_array_get_no_zeroes(pInputArray, i);
        int len = strlen(item);
        char *left = y_mem_alloc(len);
        char *right = y_mem_alloc(len);

        left[0] = '\0';
        right[0] = '\0';

        y_split_str(item, separator, left, right);
        lr_eval_string_ext_free(&item);

        y_array_save(left, pArrayNameLeft, i);
        free(left);
        y_array_save(right, pArrayNameRight, i);
        free(right);
    }

    y_array_save_count(i-1,pArrayNameLeft);
    y_array_save_count(i-1,pArrayNameRight);
}


/*! Shuffle a parameter array and store the result in a new array of parameters.

\warning Unlike most of the parameter array functions, the source parameter array CANNOT be the same as the result parameter array.
\note y_array_pick_random() is usually better and much faster than using this function. Only use it if you cannot allow the code to repeatedly pick the same element.

\param [in] source_param_array_name The name of the array holding the values to be shuffled.
\param [in] dest_param_array_name The parameter array holding the shuffled result. This array must NOT have the same name as the original array.

\b Example:
\code
web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
web_url("URL=www.google.nl", LAST);
y_array_shuffle("TAG", "SHUFFLE_TAG");
y_array_dump("SHUFFLE_TAG");
//  Now, suppose {TAG_1}="cow", {TAG_2}="chicken", {TAG_3}="boneless", {TAG_4}="redguy".
//  Then this could be the result: 
//         {SHUFFLE_TAG_1} = "chicken", {SHUFFLE_TAG_2}="redguy", {SHUFFLE_TAG_3} = "cow", {SHUFFLE_TAG_4}="boneless".
\endcode

\see y_array_pick_random()
\author Raymond de Jongh, Floris Kraak
*/
void y_array_shuffle(char *source_param_array_name, char *dest_param_array_name)
{
    int *shuffle;
    int source_length, i;

    if (strcmp(source_param_array_name, dest_param_array_name) == 0)
    {
        lr_error_message("Source and Destination parameter name can not be equal!");
        lr_abort();
        return;
    }

    source_length = y_array_count(source_param_array_name);
    if(source_length < 1)
    {
        lr_error_message("Cannot shuffle empty parameter arrays!");
        lr_abort();
        return;
    }
    else if(source_length == 1)
    {
        lr_log_message("Warning: Cannot shuffle a list with just 1 entry.");
        y_array_save( y_array_get(source_param_array_name, 1), dest_param_array_name, 1);
        y_array_save_count(1, dest_param_array_name);
        return;
    }

    // Now the cases where we can actually shuffle something: 
    shuffle=(int *)y_array_alloc(source_length+1, sizeof(int)); // Allocate room for an array of ints.
    for (i=1; i<=source_length; i++) // Fill it with the numbers 1 .. source length, denoting indexes into the source array, unshuffled.
    {
        //lr_message("i: %d", i);    
        shuffle[i]=i;
    }

	// For each item in that list of ints, have it swap places with a randomly chosen item from that same list. (Switching with yourself is legal.)
    for(i=1; i<=source_length; i++)
    {
        int temp, r;
        r=(y_rand() % (source_length))+1;
        lr_log_message("shuffle r %d into i %d", r, i);
        lr_log_message("swapping %d with %d", shuffle[i], shuffle[r]);
        temp = shuffle[i];
        shuffle[i] = shuffle[r];
        shuffle[r] = temp;
    }

	// Now that we have a list of shuffled numbers, use those numbers to store the elements in the source parameter array into the corresponding slots of the destination.
    for(i=1; i<=source_length; i++)
    {
        y_array_save(
           y_array_get(source_param_array_name, shuffle[i]),
           dest_param_array_name, i);
    }
    y_array_save_count(--i, dest_param_array_name); // We can probably just use the source_length here instead.
    free (shuffle);
}

#endif // _Y_PARAM_ARRAY_C_
