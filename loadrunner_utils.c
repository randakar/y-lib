/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2009 Floris Kraak
 * Copyright (C) 2009 Raymond de Jongh
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

#ifndef _LOADRUNNER_UTILS_C
#define _LOADRUNNER_UTILS_C

#include "string.c"


//
// This file contains loadrunner specific helper funtions.
//
// See also: string.c, logging.c, transaction.c, profile.c - all of which
// contain helper functions (grown out of this file) which cover a specific topic.
//



//////////////////////////// Param array functions. ////////////////////////////
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Retrieve the number of saved elements for the parameter defined in *pArrayName
// 
// Superseded by the LR 9 function lr_paramarr_len() but kept for compatibility reasons.
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
int y_array_count( const char *pArrayName )
{
    // -- Loadrunner 9 and upwards
    // return lr_paramarr_len(pArrayName);

    // -- Loadrunner 8 and below
    int result;
    char *tmp = y_mem_alloc( strlen(pArrayName) + strlen("_count") +4 );  // 4 characters added: { _ } \0
    
    sprintf(tmp , "{%s_count}" , pArrayName );
    result = atoi(lr_eval_string(tmp));
    free(tmp);
    return result;
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
// Get a specific element from a parameter list.
//
// Note: If the data inside the parameter contains embedded null (\x00) characters you may have an issue
// processing the return value. See also y_array_get_no_zeroes().
//
// Superseded by the LR 9 function lr_paramarr_idx() but kept for compatibility reasons.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//    example usage:     
//        web_reg_save_param("TAG", "LB=<a", "RB=>", "ORD=ALL", LAST);
//        web_url("www.google.nl", 
//         ...
//        lr_message("LR9x: 4e in de array: %s", lr_paramarr_idx("TAG", 4));    // LR9 variant
//        lr_message("LRxx: 4e in de array: %s", y_array_get("TAG", 4));        // y_array_get variant.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
y_array_save(const char* value, const char* pArray, const int pIndex)
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
y_array_save_count(const int count, const char *pArray)
{
    const char *ctpf = "_count";
    int len = strlen(pArray) + strlen(ctpf) + 1;
    char *result = y_mem_alloc(len);
    sprintf(result, "%s%s", pArray, ctpf);
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
y_array_add( const char* pArray, const char* value )
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
y_array_concat(const char *pArrayFirst, const char *pArraySecond, const char *resultArray)
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
    
    index = (rand() % count) +1;
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
    
    index = (rand() % count) +1;
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
y_array_dump( const char *pArrayName )
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//      lr_save_string("<option value=\"water\"><option value=\"fire\"><option value=\"burn\">", "SOURCE");
//      y_array_save_param_list("SOURCE", "value=\"", "\">", "VALUES");
//      y_array_dump("VALUES");    // {VALUES_1} contains "water" (no quotes)    {VALUES_2} contains "fire" (no quotes)    etc...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_array_save_param_list(const char *sourceParam, const char *LB, const char *RB, const char *destArrayParam)
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
y_array_grep( const char *pArrayName, const char *search, const char *resultArrayName)
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
y_array_filter( const char *pArrayName, const char *search, const char *resultArrayName)
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

    y_array_save_count(pArrayNameLeft, i-1);
    y_array_save_count(pArrayNameRight, i-1);
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
//        y_shuffle_parameter_array("TAG", "SHUFFLE_TAG");
//         
//    now, suppose {TAG_1}="cow", {TAG_2}="chicken", {TAG_3}="boneless", {TAG_4}="redguy"
//  then this could be the result: 
//         {SHUFFLE_TAG_1} = "chicken", {SHUFFLE_TAG_2}="redguy", {SHUFFLE_TAG_3} = "cow", {SHUFFLE_TAG_4}="boneless".
//     to do: remove Dunglish.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_shuffle_parameter_array(char *source_param_array_name, char *dest_param_array_name)
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
    }

    source_length=lr_paramarr_len(source_param_array_name);
    shuffle=(int *)calloc(source_length+1, sizeof(int));
    destination_length=(char *)calloc(strlen(dest_param_array_name)+9); 

    lr_message("source_length: %d", source_length);
    for (i=1; i<=source_length; i++)
    {
        lr_message("i: %d", i);    
        shuffle[i]=i;
    }

    for(i=1; i<=source_length; i++)
    {
        r=y_rand(1,source_length);
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

    sprintf(destination_length, "%s_count", dest_param_array_name);        // moet nog iets maken wat lijkt op "{DEST_count}"
    lr_save_int(source_length, destination_length);
    free (shuffle);
    free(destination_length);

}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
//// Time/date/stamp functions

typedef long time_t;

time_t y_timestamp()
{
    return time(NULL);
}
// --------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------
//// Random number generator control ////

//
// Roll a random number between (and including) 0 and randMax, and tell us if that number falls 
// between the lower and upper bounds or not. (attention: boundaries are included!)
//
// Zero = No.
// One = Yes.
// Negative return values are errors and generally mean that arguments make no sense
//
// Fixme: Think up a way to automatically initialize the random number generator instead
// of just relying on the user to put it in vuser_init().
// (Hint to the user: Please call srand() with something sufficiently random in vuser_init().)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//         y_rand_between(1, 10, 20);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int y_rand_between(int lowerbound, int upperbound, int randMax)
{
    int roll;

    if( (0>lowerbound) || (lowerbound>upperbound) || (upperbound > randMax) || (randMax <= 0))
    {
        lr_error_message("y_rand_between called with nonsensical arguments: ( 0 <= %d < %d <= %d ) == FALSE",
                        lowerbound, upperbound, randMax);
        return -1;
    }

    roll = y_rand(0, randMax);
    if( (roll >= lowerbound) && (roll <= upperbound) )
    {
        return 1;
    }

    return 0;
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// create a random number (integer), between two values, including the boundaries(!)
// So, y_rand(0,4) can result in these values: 0,1,2,3,4
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//         int random;        
//         random = y_rand(0, 10);        // generate a random number between 0 and 10.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int y_rand(int lowerbound, int upperbound)
{
    int roll;

    if ((lowerbound>upperbound) || ((upperbound - lowerbound) == 0))
    {
        lr_error_message("y_rand called with nonsensical arguments. (lowerbound should be less than upperbound)");
        return -1;    //    hmmm. is this correct?
    }
    roll = rand() % ((upperbound + 1 - lowerbound)) + lowerbound;
    return roll;
}




// --------------------------------------------------------------------------------------------------
////// Controller / Scenario related //////


// This will fetch attributes from the vUser's command line (as set in the scenario!)
// and store them in a parameter of the same name.
// See also y_save_attribute_to_parameter()
//
// Arguments:
//   - param: the name of the attribute to get AND
//            the name of the parameter to store the retrieved value in.
//
// @author: Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_save_attribute( char* param )
{
   y_save_attribute_to_parameter( param, param );
}


// This will fetch attributes from the vUser's command line (as set in the scenario!)
// and store them in a parameter.
// See also y_save_attribute()
//
// These attributes are defined in the details of the vUser group
// They take the form "-attributename value"
//
// Arguments:
//   - attrib: The name of the attribute to fetch
//   - param : The name of the parameter to store the attribute in.
//
// @author: Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_save_attribute_to_parameter( char* attrib, char* param )
{
   char *tmp;

   if( (tmp = lr_get_attrib_string(attrib)) != NULL )
   {
      lr_save_string(tmp, param);
   }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// y_breadcrumb();
//         Adds a string to the parameter {breadcrumb}.
//      Use this to keep track of the steps taken by the script. Very useful is you have a script which
//      does things in a random order and you want to know (in the end) which order it used.
//      You can, ofcourse, write this to a (log)file.
//      Don't forget to use y_breadcrumb_reset() to clear the parameter at the start of the script.
// @author: Raymond de Jongh
// Example:
//        y_breadcrumb_reset();    // clean the breadcrumb-variable. (previous data in {breadcrumb} is deleted.
//        y_breadcrumb("start");
//        .... some code....
//        y_breadcrumb("processing data")
//        ... some code ....
//        y_breadcrumb("finished")
//      The result is that {breadcrumb} contains "start;processing data;finished"
// --------------------------------------------------------------------------------------------------
void y_breadcrumb(char *breadcrumb)
{
    lr_message("---------------------------------------------------------------------------------");

    if ((strcmp(lr_eval_string("{breadcrumb}"), "{breadcrumb}") == 0) || ((strcmp(breadcrumb, "") == 0)))
    {
        lr_save_string("", "breadcrumb");
        lr_save_string(breadcrumb, "y_breadcrumb_temp");
        lr_save_string(lr_eval_string("{y_breadcrumb_temp}"), "breadcrumb");
    }
    else
    {
        lr_save_string(breadcrumb, "y_breadcrumb_temp");
        lr_save_string(lr_eval_string("{breadcrumb};{y_breadcrumb_temp}"), "breadcrumb");
    }
}



void y_breadcrumb_reset()
{
    lr_save_string("", "breadcrumb");

}




// --------------------------------------------------------------------------------------------------
// y_write_to_file()
//       writes content (a string) to a file.
// @author: Raymond de Jongh
// Example:
//      y_write_to_file("c:\\test.txt", "Write this to a file!");
// --------------------------------------------------------------------------------------------------
int y_write_to_file(char *filename, char *content)
{
   long file;
   int result;

   lr_message("LOGGING: %s", content);

   if ((file = fopen(filename, "at")) == NULL)
   { 
       lr_error_message ("Cannot write to file >>%s<<", filename); 
       return -1;             // fail to open file...
   } 
   if (result = fprintf(file, "%s\n", content) <0)
   {
       return result;        // fail to write to file...
   }

   if (result = fclose(file)!=0)
   {
       return result;        // fail to close file...
   }

   return 0;                // everything worked great!
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// y_write_to_log()
//     writes "content" (a string) to a (log)file.
//     The content will start with current date, time, Vuser-group, VuserId-number and Scenario-ID
//        separated by commas. This function relies on y_write_to_file();
// @author: Raymond de Jongh
// Example:
//     y_write_to_log("c:\\logfile.txt", "Everything went great");
//     This will result that the file (c:\logfile.txt) has this content:
//        20091126,215212,SomeGroup,3,4,Everything went great
//     Ofcourse, the ID's and groupname will be different as this is just an example.
// --------------------------------------------------------------------------------------------------
int y_write_to_log(char *filename, char *content)
{
    int id, scid;
    char *vuser_group;
    int string_length=0;
    char *log;
    int len_vuser_group;
    int len_scid;
    int result;

    lr_whoami(&id, &vuser_group, &scid);



    string_length = strlen(content);
    string_length +=strlen(vuser_group);
    string_length +=15;        // y_datetime() is altijd 15 chars lang.
    string_length +=6;        // 6 chars voor id (is dat genoeg?!?)
    string_length +=6;        // 6 chars voor scid (is dat genoeg?!?)

    log = y_mem_alloc(string_length);
    y_datetime();
    sprintf(log, "%s,%s,%6d,%6d,%s", lr_eval_string("{DATE_TIME_STRING}"), vuser_group, id, scid, content);

    result = y_write_to_file(filename, log);

    free(log);

    return result;

}
// --------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------
//    y_datetime()
//      Simply returns the current date-time as a string, in this format:
//        YYYYMMDD,HHMMSS (yesss, separated by a comma. That is most suitable for this moment.
// @author: Raymond de Jongh
// --------------------------------------------------------------------------------------------------
void y_datetime()
{
    char *tmp;
    typedef long time_t; 
    struct tm 
    { 
        int tm_sec;   // seconds after the minute - [0,59] 
        int tm_min;   // minutes after the hour - [0,59] 
        int tm_hour;  // hours since midnight - [0,23] 
        int tm_mday;  // day of the month - [1,31] 
        int tm_mon;   // months since January - [0,11] 
        int tm_year;  // years since 1900 
        int tm_wday;  // days since Sunday - [0,6] 
        int tm_yday;  // days since January 1 - [0,365] 
        int tm_isdst; // daylight savings time flag 
    }; 

    time_t t; 
    struct tm * now; 

    _tzset(); // Sets variables used by localtime 
    time(&t); 

    tmp = (char *)y_mem_alloc(16);

    // Convert to time structure
    now = (struct tm *)localtime(&t); 
    sprintf(tmp, "%04d%02d%02d,%02d%02d%02d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec); 
    tmp[15] = '\0';

    lr_save_string(tmp, "DATE_TIME_STRING");

    free(tmp);
}





// --------------------------------------------------------------------------------------------------
#endif // _LOADRUNNER_UTILS_C
