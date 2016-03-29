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

/*
 * Documentation generated from this source code can be found here: http://randakar.github.io/y-lib/
 * Main git repitory can be found at https://github.com/randakar/y-lib
 */

 
/*! 
\file y_string.c
\brief Y-lib string function library

Contains low level string and memory manipulation functions, insofar not provided by the C standard.
The philosophy of ylib is that the script engineer should not be required to worry about C-strings and C-like memory manipulation when parameters will suffice.
Most string manipulation functions in the y-lib library take loadrunner parameters as arguments and place their output in one or more of these parameters.
This usually makes it easy to correlate a value (capturing it in a parameter), process it, then pass it on to the next request (again as a parameter).
*/
#ifndef _Y_STRING_C_
//! \cond include_protection
#define _Y_STRING_C_
//! \endcond

#include "y_core.c"

//! \cond function_removal
/*!
\def y_get_int_from_char 
\brief Convert a *single* character 0-9 to an int

\b Example:
\code
int i = y_get_int_from_char('9');
lr_message("i = %d", i);  // result is "i = 9"
\endcode
\author Floris Kraak
\deprecated If you need this you lack basic C foundation skills.
*/
#define y_get_int_from_char(c) (isdigit(c) ? c - ‘0’: 0)

//! \cond function_removal
/*!
\brief Calculate how much space storing the decimal representation of a number into a string requires.
\param [in] number An integer number that needs to be stored in a string in decimal notation.
\returns The number of characters required.

\b Example:
\code
    int input = 12345;
    lr_log_message("Length of %d = %d", input, y_int_strlen(input)); // Prints "Length of 12345 = 5"
    input = -12345;
    lr_log_message("Length of %d = %d", input, y_int_strlen(input)); // Prints "Length of -12345 = 6"
    input = 0;
    lr_log_message("Length of %d = %d", input, y_int_strlen(input)); // Prints "Length of 0 = 1"
\endcode
\deprecated If you need this you lack basic C foundation skills.
*/
#define y_int_strlen(number) (number?(int)floor(log10(abs(number)))+(number<0?2:1):1)
//! \endcond

/*!
\brief Copy a parameter to a new name.
This is a semi-efficiënt parameter copy using lr_eval_string_ext(), with appropriate freeing of memory.
\param [in] source_param The parameter to copy.
\param [in] dest_param The name of the parameter to copy the first parameter to.

\b Example:
\code
lr_save_string("text", "param_a");
y_copy_param("param_a", "param_b");   // Results in an exact copy of the content of param_a being stored in param_b.
lr_log_message(lr_eval_string("param_b: {param_b}")); // Prints "param_b: text".
\endcode
\author Floris Kraak
*/
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

/*!
\brief Convert the content of a parameter to UPPERCASE. 

This will replace the content of the paramenter named in 'param_name' with the uppercased version.
Does not affect non-alphabetic characters.

\param [in] param_name The parameter to convert to uppercase.

\b Example:
\code
lr_save_string("aBcDeFgHiJ &*45#$@#)!({}", "Test");
lr_message(lr_eval_string("Original: {Test}\n"));
y_uppercase_parameter("Test");
lr_message(lr_eval_string("Altered: {Test}\n")); // prints "Altered: ABCDEFGHIJ &*45#$@#)!({}".
\endcode
\author Floris Kraak
*/
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

/*!
\brief Save a substring of a parameter into a new parameter.
Search for a specific substring inside a parameter using left and right boundaries and save that into a new parameter.

\param [in] original_parameter The parameter to search.
\param [in] result_parameter The name of the parameter to store the result in.
\param [in] left The left boundary - the text immediately preceding the substring in question.
\param [in] right The right boundary.

\b Example:
\code
char* str = "LorumIpsumLipsum";
lr_save_string(str, "param");
y_substr("param", "param", "Lorum", "Lipsum");
lr_log_message(lr_eval_string("{param}")); // Prints "Ipsum".
\endcode
\author André Luyer, Marcel Jepma & Floris Kraak
*/
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


/*!
\brief Split a string into 2 parts using the search string. Save the left part into the result parameter.
\param [in] original_parameter The parameter to search.
\param [in] search The text after the text we're looking for.
\param [in] result_parameter The name of the parameter to store the result in.

\b Example:
\code
lr_save_string("AstrixObelixIdefix", "Test");
lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}=AstrixObelixIdefix
y_left( "Test", "Obelix", "Test2" );
lr_message(lr_eval_string("New Param: {Test2}\n"));    // {Test2}=Astrix
\endcode
\author Floris Kraak
*/
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


/*!
\brief Split a string into 2 parts using the search string. Save the right part into the result parameter.
\param [in] original_parameter The parameter to search.
\param [in] search The text preceding the text we're looking for.
\param [in] result_parameter The name of the parameter to store the result in.

\b Example:
\code
lr_save_string("AstrixObelixIdefix", "Test");
lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}=AstrixObelixIdefix
y_right( "Test", "Obelix", "Test4" );
lr_message(lr_eval_string("New Param: {Test4}\n"));    //    {Test4}=Idefix
\endcode
\author Floris Kraak
*/
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


/*!
\brief Split a string into 2 parts using the search string. Save the rightmost part into the result parameter.
This is almost the same as y_right(), but doesn't stop at the first match - instead, it uses the *last* match.
It's pretty much the difference between 'greedy' and 'not greedy' in a regular expression..

\param [in] original_parameter The parameter to search.
\param [in] search The text preceding the text we're looking for.
\param [in] result_parameter The name of the parameter to store the result in.

\b Example:
\code
lr_save_string("AstrixObelixIdefix", "Test");
lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}=AstrixObelixIdefix
y_right( "Test", "Obelix", "Test4" );
lr_message(lr_eval_string("New Param: {Test4}\n"));    //    {Test4}=Idefix
\endcode
\author Floris Kraak
*/
void y_last_right( const char *original_parameter, const char *search, const char *result_parameter)
{
    char *result = y_get_parameter_or_null(original_parameter);
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


/*!
\brief Split a string into 2 parts based on a search string

\warning Unlike the others this one does not use parameter, but raw char pointers instead.
This mostly to accomodate y_array_split().
For the parameter version use y_split().

\param [in] original The string to search.
\param [in] separator The string to use as a seperation marker between the two parts.
\param [in] left A preallocated char* buffer to hold the left hand side of the result.
\param [in] right A preallocated char* buffer to hold the right hand side of the result.
\author Floris Kraak
*/
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


/*!
\brief Split a parameter in two based on a seperating string.
If the seperator is not found in the original parameter the original parameter will be stored in it's entirety in the left hand parameter.

\param [in] originalParameter The parameter to search.
\param [in] separator The string to use as a seperation marker between the two parts.
\param [in] leftParameter The parameter that will hold the left hand side of the split result.
\param [in] rightParameter The parameter that will hold the right hand side of the split result.

\b Example:
\code
lr_save_string("WackoYackoDotWarner", "Test");
lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}    = WackoYackoDotWarner
y_split("Test", "Yacko", "Left", "Right");           // Use "Yacko" as the separator
lr_message(lr_eval_string("Original: {Test}\n"));    // {Test}    = WackoYackoDotWarner
lr_message(lr_eval_string("Left    : {Left}\n"));    // {Left}    = Wacko
lr_message(lr_eval_string("Right   : {Right}\n"));   // {Right}   = DotWarner
\endcode
\author Floris Kraak
*/
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


/*!
\brief Remove leading and trailing whitespace from a parameter.

This does not support unicode, so it may not catch everything.
Supported whitespace is: 
  " "(=space)
  "\r"(=carrige return)
  "\n"(=line feed)
  "\t"(=tab)
The result is stored in the original parameter.

\param [in] parameter The parameter to chop.

\b Example:
\code
lr_save_string("  WackoYackoDot ", "Test");
lr_message(lr_eval_string("Original: >{Test}<\n"));    // {Test}= "  WackoYackoDot "
y_chop("Test");
lr_message(lr_eval_string("Original: >{Test}<\n"));    // {Test}= "WackoYackoDot"
\endcode
\author: Floris Kraak
*/
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


/*!
\brief Search and replace inside a parameter.
This replaces the content of the originally passed-in parameter with the new content when done.

\note This one has a built-in search/y_replace limit. After 1000 replacements it will stop.
If 1000 replacements in a single parameter does not suffice consider using other methods.

\param [in] parameter The parameter to search.
\param [in] search What to search for.
\param [in] replace What to replace it with.

\b Example:
\code
lr_save_string("test123", "par1");
y_replace("par1", "1", "ing1");        // {par1} now has the value testing123
\endcode
*/
void y_replace( const char *parameter, const char *search, const char *replace )
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


/*!
\brief Remove all occurrences of a specified text from a parameter.

This is a lighter weight alternative to the y_replace() function in cases where just want to remove text, rather than replace it with something else.
Stores the result in the original parameter.

\param [in] paramName The parameter to search.
\param [in] removeMe The text to remove.

\b Example:
\code
lr_save_string("test123", "par1");
y_remove_string_from_parameter("par1", "1");   // {par1} now has the value test23
\endcode
*/
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


/*!
\brief Create a unique parameter.
\param param The name of a parameter to store the resulting string in. Length is always 22 (base64) characters.
\return void
\author Floris Kraak & André Luyer

Example:
\code
y_param_unique("test");
\endcode
*/
void y_param_unique(char *param)
{
    char buf[25];                   // base64 of UUID's are always 24 characters, plus null byte.
    lr_generate_uuid_on_buf(buf);   // Probably a wrapper for http://msdn.microsoft.com/en-us/library/windows/desktop/aa379205(v=vs.85).aspx
    lr_save_var(buf, 22, 0, param); // save & trim off ==
}



//! Generates a random string with (pseudo) words created from a given string of characters
/*!
This function uses a given set of characters to create words, separated by spaces.
The words are minimal \e minWordLength characters long, and maximum \e minWordLength characters.
The total length of the line is minimal \e minimumLength and maimum \e maximumLength long.

\b Example:
\code
// Generates a string of minimal 3 and max 20 characters, 
// with words of minimal 1 and maximal 3 charactes.
// Chooses only characters a, c, d or d.
y_random_string_buffer_core("uitvoer", 3,20, 1, 3, "abcd");

// Generates some sort of mock morse-code of exactly 30 characters.
// with words of minimal 1 and maximal 3 charactes.
// Chooses only characters a, c, d or d.
y_random_string_buffer_core("uitvoer", 3,20, 1, 3, "abcd"); // could result in "ccc db dac c"
\endcode

@param[out] parameter Name of the LR-parameter in which the result is stored
@param[in] minimumLength Minumum length of the string
@param[in] maximumLength Maximum length of the string
@param[in] minWordLength Minimum length of the words within the string
@param[in] maxWordLength Minimum length of the words within the string
@param[in] characterSet The string is build from this string of characters
\return void
\author Floris Kraak / Raymond de Jongh

\sa y_random_number_buffer
\sa y_random_string_buffer_curses
\sa y_random_string_buffer
\sa y_random_string_buffer_hex
*/
void y_random_string_buffer_core(const char *parameter, int minimumLength, int maximumLength, 
                                 int minWordLength, int maxWordLength, char *characterSet)
{
   char *buffer;
   int charSetSize; // length of the characterSet
   int length = 0;
   int max = -1;

   char randomNumber;
   int lettersInWord;

   charSetSize=strlen(characterSet);

   //lr_message("minimumLength %d -- maximumLength %d -- minWordLength %d -- maxWordLength %d", 
   //      minimumLength, maximumLength, minWordLength, maxWordLength);

   // error checks - lots of code that saves us headaches later
   if( minimumLength < 0 ) {
      lr_error_message( "minimumLength less than 0 (%d)", minimumLength );
   }
   else if( maximumLength < 1 ) {
      lr_error_message( "maximumLength less than 1 (%d)", maximumLength );
   }
   else if( maximumLength > (1024 * 1024) ) {
      lr_error_message( "maximumLength too big (%d)", maximumLength );
   }
   else if( maximumLength < minimumLength ) {
      lr_error_message( "minimumLength (%d) bigger than maximumLength (%d)", minimumLength, maximumLength );
   }
   else if(maximumLength > minimumLength) {
      // Not an error
      max = y_rand_between(minimumLength, maximumLength);
      lr_log_message("Max: %d", max);
   }
   else if(maximumLength == minimumLength) {
      // Not an error either
      max = maximumLength;
   }
   else {
      lr_error_message("This can never happen. If we reach this point it's a bug.");
   }

   // if we got an error
   if( max < 0 )
   {
      lr_set_transaction_status(LR_FAIL);
      lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_FAIL);
   }

   // get memory for the buffer
   buffer = (char *)y_mem_alloc( max +1 );
   // note: if this fails y_mem_alloc() aborts the script, so no error handling needed.

   while( length < max )
   {
//      lr_message("Length: %d   max: %d", length, max);
//      lettersInWord = ((y_rand() % 8) + 2);
      if( maxWordLength == 0 )
      {
         lettersInWord = maximumLength;
      }
      else
      {
         lettersInWord = y_rand_between(minWordLength, maxWordLength);
         if( lettersInWord < 0 )
         {
            lr_error_message( "y_rand_between() returned an errorcode (%d)", lettersInWord );
            lr_set_transaction_status(LR_FAIL);
            lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_FAIL);
         }
      }

      while( lettersInWord-- && (length < (max)) )
      {
         randomNumber = (char) (y_rand() % charSetSize);
         buffer[length++] = characterSet[randomNumber];
      }

      if( maxWordLength != 0 )
      {
         if( length < max -1 )
         {
            buffer[length++] = ' ';
         }
      }
   }
   buffer[max] = '\0';

   lr_save_string(buffer, parameter);
   free(buffer);
}

//! Returns a random string with (pseudo) words created from a given string of characters
/*!
This function uses a given set of characters to create words, separated by spaces.
The words are minimal 3 characters long, and maximum 8 characters long.
Should you need other word lenghts, use y_random_number_buffer_core().
The total length of the line is minimal \e minimumLength and maximum \e maximumLength long.

@param[out] parameter Name of the LR-parameter in which the result is stored
@param[in] minimumLength Minumum length of the string
@param[in] maximumLength Maximum length of the string
\return void
\author Raymond de Jongh
\sa y_random_string_buffer_core
*/
void y_random_string_buffer(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 3, 8, 
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
}


//! Returns a random string of numbers with a given minimum and maximum length.
/*!
This function generates a string of numbers with a given minimum and maximum length.
@param[out] parameter Name of the LR-parameter in which the result is stored
@param[in] minimumLength Minumum length of the string
@param[in] maximumLength Maximum length of the string
\return void
\author Raymond de Jongh
\sa y_random_string_buffer_core
*/
void y_random_number_buffer(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 0, 0, "0123456789");
}


//! Returns a string containing only the "shift-1...shift 9 characters (on a US-keyboard).
/*!
This function generates a string of non-alfa-characters with a given minimum and maximum length.

@param[out] parameter Name of the LR-parameter in which the result is stored
@param[in] minimumLength Minumum length of the string
@param[in] maximumLength Maximum length of the string
\return void
\author Raymond de Jongh
\sa y_random_string_buffer_core
*/
void y_random_string_buffer_curses(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 0, 0, "!@#$%^&*()");
}


//! Generates a random string with a hexadecimal number, of a given minimum and maximum length
/*!
This function generates a string with a hexadecimal number.

Should you need other word lenghts, use y_random_number_buffer_core().
The total length of the line is minimal \e minimumLength and maimum \e maximumLength long.

@param[out] parameter Name of the LR-parameter in which the result is stored
@param[in] minimumLength Minumum length of the string
@param[in] maximumLength Maximum length of the string
\return void
\author Raymond de Jongh
\sa y_random_string_buffer_core
*/
void y_random_string_buffer_hex(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 0, 0, "0123456789ABCDEF");
}



/*!
\brief Get the content of a parameter without embedded null bytes (\0 characters) from the named parameter, if any.
In some cases we want to fetch the content of a parameter but the parameter contains embedded NULL characters which make further processing harder. 
This will fetch a parameter but "cleanse" it from such contamination, leaving the rest of the data unaltered before returning it.

\warning The return value of this function needs to be freed using lr_eval_string_ext_free().

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
   free(tmp);
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
   if (result_size) {
      char *ptr;
	  for(ptr = result; result_size--; ptr++)
	     if (*ptr == 0) *ptr = replacement;	  
	  
	  /*
      // Now replace NULL bytes (NULL) in the input with something else..
      for( result_strlen = strlen(result); result_strlen < result_size; result_strlen = strlen(result))
      {
         result[result_strlen] = replacement;
         //lr_log_message("Cleansing param %s, result now '%-*.*s' and contains %d bytes.", param_name, result_size, result_size, result, result_size);
      }*/
   }
   return result;
}

/*!
\brief Clean a parameter by replacing any embedded NULL (null) characters with a replacement character.

This would normally only happen if you have used to web_reg_save_param() and the result contains one or more null-character(s).
Any such characters are replaced with replacement_char and the result is stored in the original parameter.
When no null-character is found, the result is unaltered.

\warning Since the return value is allocated with malloc(), it will need to be freed using free() at some point.

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


#endif // _Y_STRING_C_

