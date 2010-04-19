/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2010 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

#ifndef _PARAM_ARRAY_C
#define _PARAM_ARRAY_C

#include "y_string.c"
#include "y_loadrunner_utils.c"

//
// This file contains loadrunner param array helper functions.
// "param arrays" are lists of parameters as saved by web_reg_save_param() with the
// "ord=all" argument. 
//

//
// Some historical functions now have loadrunner equivalents, starting with LoadRunner version 9. 
// Those functions have been replaced with macros that will call the correct LR 9 equivalent 
// instead. If Y_COMPAT_LR_8 is enabled the old behaviour is restored.
//
// If you still use loadrunner versions earlier than 9 or are using Performance Center (version?)
// you may need to uncomment the line below.
//
// #define Y_COMPAT_LR_8


//////////////////////////// Param array functions. ////////////////////////////
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Retrieve the number of saved elements for the parameter defined in *pArrayName
// 
// Superseded by the LR 9 function lr_paramarr_len(), if Y_COMPAT_LR_8 is turned off this function 
// is replaced with a simple macro calling the loadrunner equivalent.
// Note: Performance Center may need Y_COMPAT_LR_8
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//    example usage:     
//        int result;
//        web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", 
//        ...
//        LAST);
//        result = y_array_count("TAG");
//        lr_message("RESULT: %d", result);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef Y_COMPAT_LR_8
int y_array_count( const char *pArrayName )
{
    // -- Loadrunner 9 and upwards
    // return lr_paramarr_len(pArrayName);

    // -- Loadrunner 8 and below
    int result;
    char *tmp = y_mem_alloc( strlen(pArrayName) +9 );  // 9 = strlen("{}_count") +1 -- the +1 is '\0'.
    
    sprintf(tmp , "{%s_count}" , pArrayName );
    result = atoi(lr_eval_string(tmp));
    free(tmp);
    return result;
}
#else
#define y_array_count( pArrayName ) lr_paramarr_len(pArrayName) 
#endif // Y_COMPAT_LR_8


// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Get a specific element from a parameter list.
//
// Note: If the data inside the parameter contains embedded null (\x00) characters you may have an 
// issue processing the return value. See also y_array_get_no_zeroes().
//
// Superseded by the LR 9 function lr_paramarr_idx(), if Y_COMPAT_LR_8 is turned off this function
// is replaced with a simple macro calling the loadrunner equivalent.
// Note: Performance Center may need Y_COMPAT_LR_8
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//    example usage:     
//        web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", 
//         ...
//        lr_message("LR9x: 4e in de array: %s", lr_paramarr_idx("TAG", 4));    // LR9 variant
//        lr_message("LRxx: 4e in de array: %s", y_array_get("TAG", 4));        // y_array_get variant.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef Y_COMPAT_LR_8
char *y_array_get( const char *pArray, const int pIndex )
{
    //-- Loadrunner 9 and upwards
    // return lr_paramarr_idx(pArray, pIndex);

    //-- Loadrunner 8 and below, or when using performance center,
    //   or when you want some sane boundary checks:
    int size = y_array_count( pArray );
    char *tmp;
    char *result;
    
    //lr_log_message("y_array_get(%s,%d)", pArray, pIndex ); 
    if ( (pIndex > size) || (pIndex < 1) )
    {
        lr_error_message("Index out of bounds");
        lr_abort();
    }

    tmp = y_mem_alloc( strlen(pArray) +10 +4 ); // 10 characters for the index, 4 characters added: { _ } \0
    sprintf( tmp , "{%s_%d}" , pArray , pIndex );

    // This breaks if the index number is 10 billion or more  ;-)
    // I presume we have run out of memory by then ..
    result = lr_eval_string(tmp);
    free (tmp);    
    return result;
}
#else
#define y_array_get( pArray, pIndex ) lr_paramarr_idx(pArray, pIndex)
#endif // Y_COMPAT_LR_8

// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// As y_array_get(), but it filters embedded zeroes from the input, replacing them with 
// a simple space: ' '.
// It's not ideal, but better than having your script break on this type of idiocy.
//
//
// Note: The output of this needs to be freed using lr_eval_string_ext_free();
// See also the loadrunner documentation regarding lr_eval_string_ext();
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//    example usage:     
//         see y_array_get()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char *y_array_get_no_zeroes( const char *pArray, const int pIndex )
{
    int size = y_array_count( pArray );
    char *tmp;
    char *result;
    unsigned long resultLen;
    size_t resultStrlen;
    
    //lr_log_message("y_array_get_no_zeroes(%s,%d)", pArray, pIndex );
    
    if ( (pIndex > size) || (pIndex < 1) )
    {
        lr_error_message("Index out of bounds");
        lr_abort();
    }
    
    // This breaks if the index number is 10^12 or more  ;-)
    // I presume we have run out of memory by then ..
    tmp = y_mem_alloc( strlen(pArray) +12 +4 ); // 12 characters for the index with 4 characters added: { _ } \0
    sprintf( tmp , "{%s_%d}" , pArray , pIndex );
    lr_eval_string_ext(tmp, strlen(tmp), &result, &resultLen, 0, 0, -1);
    free (tmp);    


    // y_replace NULL bytes (\x00) in the input with something else..
    for( resultStrlen = strlen(result);
         resultStrlen < resultLen;
         resultStrlen = strlen(result))
    {
        result[resultStrlen] = ' ';
    }

    return result;
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Save a string value in array pArray at index pIndex.
// This does not update the count value (size) of the array.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//    example usage:     
//        web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", ............
//        y_array_save("newvalue", "TAG", 2);                // save the sting "newvalue" into {TAG_2}
//        lr_message("Value: %s", y_array_get("TAG", 2));    // print the value of {TAG_2}. (will be "newvalue" in this example)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_array_save(const char* value, const char* pArray, const int pIndex)
{
    int len = strlen(pArray) +3;
    char *result;
    int power = pIndex;

    while(power = (power / 10))
    {
        len++;
    }

    result = y_mem_alloc(len);
    sprintf(result, "%s_%d", pArray, pIndex);
    lr_save_string(value, result);
    free(result);
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Update the array count (size).
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//    example usage:   
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_array_save_count(const int count, const char *pArray)
{
    int len = strlen(pArray) +7; // 7 = strlen("_count") +1, where +1 would be the '\0' byte at the end.
    char *result = y_mem_alloc(len);
    sprintf(result, "%s_count", pArray);
    lr_save_int(count, result);
    free(result);
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Add an element to an array at the end of the list.
// Note: Do not use this in a loop as it will update the count every time it's called.
// For one-offs this is fine though.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//        web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", 
//         y_array_add("TAG", "newValue");        // the added value (=last one) in {TAG} is now "newValue".
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_array_add( const char* pArray, const char* value )
{
    int size = y_array_count(pArray);
    // hmm - should we check if the array does not exist?
    // Maybe not - there are cases where we care, and there are cases where we don't.
    size++;
    y_array_save(value, pArray, size);
    y_array_save_count(size, pArray);
}
// --------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------
// Concatenate two arrays together, saves the result into a third array.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//        web_reg_save_param("TAG1", "LB=<a h", "RB=>", "ORD=ALL", LAST);  
//        web_reg_save_param("TAG2", "LB=<A id", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", ... );
//        // Loadrunner saves parameters:
//        // TAG1 - "TAG1_1" to "TAG1_12", "TAG1_count" = 12
//        // TAG2 - "TAG2_1" to "TAG2_14", "TAG2_count" = 14
//
//        y_array_concat("TAG1", "TAG2", "TAG");   // saves "TAG_1" to "TAG_26", "TAG_count" = 26
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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



// --------------------------------------------------------------------------------------------------
// Get a random element from a parameter list
// Superseded by the LR 9 function lr_paramarr_random() but kept for compatibility reasons.
//
// Also deprecated. See y_array_get_random_no_zeroes() as for why.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//         see Help of lr_paramarr_random()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char *y_array_get_random( const char *pArray )
{
    //return lr_paramarr_random(pArray);

    // -- Loadrunner 8 and below
    int index;
    int count = y_array_count( pArray );
    
    //lr_log_message("y_array_get_random(%s)", pArray);
    
    if( count < 1 )
    {
        lr_log_message("No elements found in parameter array!");
        return NULL;
    }
    
    index = (y_rand() % count) +1;
    return y_array_get(pArray, index);
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char *y_array_get_random_no_zeroes( const char *pArray )
{
    //return lr_paramarr_random(pArray); 
    // I don't think the LR function is actually an good substitute in this case..

    // -- Loadrunner 8 and below
    int index;
    int count = y_array_count( pArray );
    
    //lr_log_message("y_array_get_random(%s)", pArray);
    
    if( count < 1 )
    {
        lr_log_message("No elements found in parameter array!");
        return NULL;
    }
    
    index = (y_rand() % count) +1;
    return y_array_get_no_zeroes(pArray, index);
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Choose an element at random from a saved parameter list and store it in
// a parameter with the same name.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int y_array_pick_random( const char *pArray )
{
    if(y_array_count(pArray))
    {
        char *result = y_array_get_random_no_zeroes(pArray);
        lr_save_string(result, pArray );
        lr_eval_string_ext_free(&result);
        return 1;
    }
    else
    {
        lr_save_string("", pArray);
        //lr_output_message("y_array_pick_random(): Unknown parameter list");
        return 0;
    }
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Dump the contents of a list of saved parameters to std out (the run log)
// Please note that the logging can get kinda messy when you also have extended debug (substitute parameter) on.
// Consider switching the logging temporary off. 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//        web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", 
//         y_array_dump("TAG");
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Save the contents of a const char * variable to a parameter array.
// As if using web_reg_save_param() with "Ord=All" as an option, but for strings
// This is especially useful when there is a need to save lists of parameters from sections of the 
// application output (rather than the entire thing).    Think: Drop-down boxes.
// 
// Arguments: 
//  1.search: The string to be searched from.  2.pArrayName: Name of resulting parameter array.
//  3.LB    : Left Boundary (match)            4.RB:         Right Boundary (match)
//          
// Note     : This does not understand the entire web_reg_save_param() syntax,
//            notably left/right boundaries are simple strings rather than strings-with-text-flags.
// Note2    : Existing values in the destination parameter (pArrayname) will be destroyed.
// Todo     : Find out how to make this fast enough to deal with extremely large amounts of 'hits'.
// Todo2    : Write a wrapper around this - let's call it 'y_dropdown()' specifically for HTML
//            dropdown boxes.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//      lr_save_string("<option value=\"water\"><option value=\"fire\"><option value=\"burn\">", "SOURCE");
//      y_array_save_param_list("SOURCE", "value=\"", "\">", "VALUES");
//      y_array_dump("VALUES");    // {VALUES_1} contains "water" (no quotes)    {VALUES_2} contains "fire" (no quotes)    etc...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
        int end = strstr(next, RB);
        if(!end) 
            break;
        buffer[end - (int)buffer] = '\0';

        i++;
        y_array_save(next+strlen(LB), destArrayParam, i);
        next = (char *)(end + strlen(RB));
    }
    free(buffer);
    y_array_save_count(i, destArrayParam);
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Search array 'pArrayName' for string 'search' and build a new result array
// containing only parameters containing the string.
//
// Just call it 'grep' :P
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//        lr_save_string("<apple><baloon><crayon><drum>", "SOURCE");
//        y_array_save_param_list("SOURCE", "<", ">", "VALUES");
//        y_array_grep("VALUES", "r", "VALUES2");   // get all elements containing "r" (crayon and drum)
//        y_array_dump("VALUES2");
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_array_grep( const char *pArrayName, const char *search, const char *resultArrayName)
{
    int i, j = 1;
    char *item;
    int size = y_array_count(pArrayName);

    //lr_log_message("y_array_grep(%s, %s, %s)", pArrayName, search, resultArrayName);
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
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Search array 'pArrayName' for string 'search' and build a new result array
// containing only parameters that do NOT contain the string.
//
// Reverse grep, in other words.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//        lr_save_string("<apple><baloon><crayon><drum>", "SOURCE");
//        y_array_save_param_list("SOURCE", "<", ">", "VALUES");
//        y_array_filter("VALUES", "r", "VALUES2");   // get all elements NOT containing "r" (apple, baloon)
//        y_array_dump("VALUES2");
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_array_filter( const char *pArrayName, const char *search, const char *resultArrayName)
{
    int i, j = 1;
    char *item;
    int size = y_array_count(pArrayName);

    //lr_log_message("y_array_filter(%s, %s, %s)", pArrayName, search, resultArrayName);
    for( i=1; i <= size; i++)
    {
        item = y_array_get_no_zeroes(pArrayName, i); // Some pages contain \x00 in the input. Ugh.
        if( strstr(item, search) == NULL )
        {
            y_array_save(item, resultArrayName, j++);
        }
        lr_eval_string_ext_free(&item);
    }

    y_array_save_count(j-1, resultArrayName);
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Merge two arrays into a single array. They have to be of the same length.
//
// The resulting items are each item from the left array appended to the item with
// the same index in the right array, with an optional glue separator in the middle for
// convenient re-splitting later.
//
// This thing is mostly created to facilitate situations where you have a list of links
// but the titles of those links are really hard to capture in the same parameter. With this
// function you can just look for them separately, merge the results, then pick one link based on
// the title or something and then y_split() the resulting link back for further processing ..
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//        lr_save_string("<apple><baloon><crayon><drum>", "THING");
//        lr_save_string("<fruit><toy><art><music>", "CAT");
//        y_array_save_param_list("THING", "<", ">", "THING2");    //    {THING2} contains "baloon" (no quotes)
//        y_array_save_param_list("CAT", "<", ">", "CAT2");        //    {CAT2} contains "toy"
//        y_array_merge("THING2", "CAT2", "=>", "RESULT");         //    {RESULT_2} now contains baloon=>toy
//        y_array_dump("RESULT");
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int y_array_merge( const char *pArrayNameLeft, const char *pArrayNameRight, const char *separator, const char *resultArray)
{
    int i = 1;
    char *param;
    int length = y_array_count(pArrayNameLeft);

    if( length != y_array_count(pArrayNameRight) )
    {
        // If the sizes aren't the same there's a good chance numbers won't line up on both sides.
        // We definitely don't want to end up with records merged that don't actually correspond to each other!
        lr_error_message("Unable to merge arrays %s and %s - sizes unequal!", pArrayNameLeft, pArrayNameRight);
        lr_abort();
        return 0;
    }

    //lr_save_string(separator, "_sep");

    for( i=1; i <= length; i++)
    {
        char *left = y_array_get_no_zeroes(pArrayNameLeft, i);
        char *right = y_array_get_no_zeroes(pArrayNameRight, i);
        char *result = y_mem_alloc(strlen(left)+strlen(separator)+strlen(right)+1);

        sprintf(result, "%s%s%s", left, separator, right);
        lr_eval_string_ext_free(&left);
        lr_eval_string_ext_free(&right);
        y_array_save(result, resultArray, i);
        free(result);

        //lr_save_string( left, "_left");
        //lr_save_string( right, "_right");
        //y_array_save( resultArray, i, lr_eval_string("{_left}{_sep}{_right}"));
    }

    //lr_save_int(i-1, lr_eval_string("{_resultArray}_count") );
    y_array_save_count(i-1, resultArray);
    return 1;
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Split an input array vertically into two new arrays, based on a search parameter.
// This is the reverse of y_array_merge(). It will examine each parameter in turn and save
// each value into two separate parameter lists.
//
// See also y_split(), as that is the single parameter version of this.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

        // This is where the magic happens - see string.h
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
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Shuffle the array of parameters and stores the result in a new array of parameters.
// The original array of parameters is untouched.
// 
// * Please note: the source parameter must not be the same as the destination parameter. * 
// 
// Arguments:
//         1. source parameter array        2. destination parameter array
// @author: Raymond de Jongh
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//    example usage:     
//        web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", 
//         ...
//        y_array_shuffle("TAG", "SHUFFLE_TAG");
//         
//    now, suppose {TAG_1}="cow", {TAG_2}="chicken", {TAG_3}="boneless", {TAG_4}="redguy"
//  then this could be the result: 
//         {SHUFFLE_TAG_1} = "chicken", {SHUFFLE_TAG_2}="redguy", {SHUFFLE_TAG_3} = "cow", {SHUFFLE_TAG_4}="boneless".
//     to do: remove Dunglish.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_array_shuffle(char *source_param_array_name, char *dest_param_array_name)
{
    int source_length;
    int dest_length;
    int i;
    int r;
    int *shuffle;
    int temp;
    char *destination_length;

    if (strcmp(source_param_array_name, dest_param_array_name) == 0)
    {
        lr_error_message("Source and Destination parameter name can not be equal!");
        lr_abort();
    }

    source_length=lr_paramarr_len(source_param_array_name);


    shuffle=(int *)y_array_alloc(source_length+1, sizeof(int));
    destination_length=(char *)y_array_alloc(strlen(dest_param_array_name)+9, sizeof(char)); 

    lr_message("source_length: %d", source_length);
    for (i=1; i<=source_length; i++)
    {
        lr_message("i: %d", i);    
        shuffle[i]=i;
    }

    for(i=1; i<=source_length; i++)
    {
        r=y_rand_between(1,source_length);
        temp = shuffle[i];
        shuffle[i] = shuffle[r];
        shuffle[r] = temp;
    }

//    random_array_start_at_1(array, source_length);

    for(i=1; i<=source_length; i++)
    {
        // something like: 'dest_param_array'_name_i = 'source_param_array_name'_array[i];
        //y_array_save( const char* pArray, const int pIndex, const char* value )
        // char *y_array_get( const char *pArray, const int pIndex )
        y_array_save(
           y_array_get(source_param_array_name, shuffle[i]),
           dest_param_array_name, i);
    }

/*    
    sprintf(destination_length, "%s_count", dest_param_array_name);        // moet nog iets maken wat lijkt op "{DEST_count}"
    lr_message("dest_param_array_name: %s", dest_param_array_name);
    lr_message("destination_length: %s", 	destination_length);
    lr_message("source_length: %i", source_length);
    lr_save_int(source_length, destination_length);
*/
    y_array_save_count(--i, dest_param_array_name);

    free (shuffle);
    free(destination_length);
}

// --------------------------------------------------------------------------------------------------
#endif // _PARAM_ARRAY_C
