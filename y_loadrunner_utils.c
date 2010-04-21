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

#ifndef _LOADRUNNER_UTILS_C
#define _LOADRUNNER_UTILS_C

#include "y_string.c"

int _vUserID = 0;                         // virtual user id
char *_vUserGroup = NULL;                 // virtual user group
int _y_random_seed_initialized = 0;

//
// This file contains loadrunner specific helper funtions.
//
// See also: string.c, logging.c, transaction.c, profile.c - all of which
// contain helper functions (grown out of this file) which cover a specific topic.
//


// --------------------------------------------------------------------------------------------------
//// Random number generator control ////


void y_setup()
{
   // Global variables, handle with care
   lr_whoami(&_vUserID, &_vUserGroup, NULL);
}

int y_rand()
{
   if(!_y_random_seed_initialized)
   {
      if( _vUserID == NULL )
      {
         y_setup();
      }
      // Seed the random number generator for later use.
      // To make it random enough for our purposes mix in the vuser id and the adress of the vuser group name.
      // In case the script itself already initialized the random number generator, use a random number from 
      // there as well.
      srand( time() + _vUserID + ((int)(_vUserGroup)) + rand() );
      _y_random_seed_initialized = 1;
   }
   return rand();
}

// --------------------------------------------------------------------------------------------------


//! Generates a random string with (pseudo) words created from a given string of characters
/*!
This function uses a given set of characters to create words, separated by spaces.
The words are minimal \e minWordLength characters long, and maximum \e minWordLength characters.
The total length of the line is minimal \e minimumLength and maimum \e maximumLength long.

@param[out] parameter Name of the LR-parameter in which the result is stored
@param[in] minimumLength Minumum length of the string
@param[in] maximumLength Maximum length of the string
@param[in] minWordLength Minimum length of the words within the string
@param[in] maxWordLength Minimum length of the words within the string
@param[in] characterSet The string is build from this string of characters
\return void
\author Floris Kraak / Raymond de Jongh
\start_example
// Generates a string of minimal 3 and max 20 characters, 
// with words of minimal 1 and maximal 3 charactes.
// Chooses only characters a, c, d or d.
y_random_string_buffer_core("uitvoer", 3,20, 1, 3, "abcd");

// Generates some sort of mock morse-code of exactly 30 characters.
// with words of minimal 1 and maximal 3 charactes.
// Chooses only characters a, c, d or d.
y_random_string_buffer_core("uitvoer", 3,20, 1, 3, "abcd"); // could result in "ccc db dac c"
\end_example

\sa y_random_number_buffer
\sa y_random_string_buffer_curses
\sa y_random_string_buffer
\sa y_random_string_buffer_hex
*/
y_random_string_buffer_core(const char *parameter, int minimumLength, int maximumLength, 
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
      lr_message("Max: %d", max);
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
   return 0;
}


// --------------------------------------------------------------------------------------------------


//! Returns a random string with (pseudo) words created from a given string of characters
/*!
This function uses a given set of characters to create words, separated by spaces.
The words are minimal 3 characters long, and maximum 8 characters long.
Should you need other word lenghts, use y_random_number_buffer_core().
The total length of the line is minimal \e minimumLength and maimum \e maximumLength long.

@param[out] parameter Name of the LR-parameter in which the result is stored
@param[in] minimumLength Minumum length of the string
@param[in] maximumLength Maximum length of the string
\return void
\author Raymond de Jongh
\sa y_random_number_buffer_core
*/
y_random_string_buffer(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 3, 8, 
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
}


// --------------------------------------------------------------------------------------------------




//
// Roll a random number between (and including) 0 and randMax, and tell us if that number falls 
// between the lower and upper bounds or not. (attention: boundaries are included!)
//
// Zero = No.
// One = Yes.
// Negative return values are errors and generally mean that arguments make no sense
//
// This is useful for pathing decisions: Say that at point P in a script a choice has to be made
// between continuing with actions A, B, and C. The decision is made based on a percentage:
// A = 10% chance, B = 50% chance, C = 40% chance. This function was written to support the code
// that would make this decision.
//
// Note: Mathematically speaking this approach has flaws.  A better method would roll the number
// just once, and then apply the boundary constraints to it repeatedly to make the decision.
// This needs further work (tm).
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//     example usage:
//         y_rand_in_sliding_window(1, 10, 20);
//         // Returns 1 if the random number rolled is 4, and 0 if the random number was 11.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int y_rand_in_sliding_window(int lowerbound, int upperbound, int randMax)
{
    int roll;

    if( (0>lowerbound) || (lowerbound>upperbound) || (upperbound > randMax) || (randMax <= 0))
    {
        lr_error_message("y_rand_in_sliding_window called with nonsensical arguments: ( 0 <= %d < %d <= %d ) == FALSE",
                        lowerbound, upperbound, randMax);
        return -1;
    }

    roll = y_rand_between(0, randMax);
    if( (roll >= lowerbound) && (roll <= upperbound) )
    {
        return 1;
    }

    return 0;
}
// --------------------------------------------------------------------------------------------------



//! Create a random number (integer), between two boundaries. (the boundaries are included!)
/*! When the lower boundary equals the upper boundary, y_rand_between() simply returns the lower boundary.
@param[in] lowerbound Lower boundary of the generated number
@param[in] upperbound Upper boundary of the generated number
\return random number
\author Floris Kraak
\start_example
int random;        
random = y_rand_between(0, 10);        // generate a random number between 0 and 10 (including 0 and 10!)
\end_example
*/
int y_rand_between(int lowerbound, int upperbound)
{
    int roll;

    if( (lowerbound < 0) || (lowerbound > upperbound) || ((upperbound - lowerbound) == 0) )
    {
        lr_error_message("y_rand() called with negative or nonsensical arguments. Lowerbound should be less than upperbound!");
        return -1;    // Note to self: This is a classic case for standard error codes.
    }
    roll = y_rand() % ((upperbound + 1 - lowerbound)) + lowerbound;
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
       return -1;             // failed to open file...
   } 
   if (result = fprintf(file, "%s\n", content) <0)
   {
       return result;        // failed to write to file...
   }

   if (result = fclose(file)!=0)
   {
       return result;        // failed to close file...
   }

   return 0;                // everything worked great!
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
//    y_datetime()
//      Simply returns the current date-time as a string, in this format:
//        YYYYMMDD,HHMMSS (yesss, separated by a comma. That is most suitable for this moment.
// @author: Raymond de Jongh
// @author: Floris Kraak
//
// This mostly exists to facilitate y_write_to_log(), in logging.c 
//
// Comment: Ray, please have a look at lr_save_datetime() for me will you? Thanks ;-)
//           -- Floris
// --------------------------------------------------------------------------------------------------
void y_datetime()
{
    lr_save_datetime("%Y%m%d,%H%M%S", DATE_NOW, "DATE_TIME_STRING");
}


// --------------------------------------------------------------------------------------------------
#endif // _LOADRUNNER_UTILS_C
