/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2013 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
 * Copyright (C) 2009 Raymond de Jongh <ferretproof@gmail.com> | <rdjongh@ymor.nl>
 * Copyright (C) 2013 André Luyer
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

#include "vugen.h"
#include "y_string.c"

// Reserved space to hold lr_whoami() output.
int y_virtual_user_id = 0;                         // virtual user id
char* y_virtual_user_group = NULL;                 // virtual user group
int y_scid;                                        // pointer to scenario or session step identifier. See "lr_whoami()";

// Complain loudly with a compiler error if people still use the old variants of the above.
#define _vUserID 0_vUserID_no_longer_exists_please_use_y_virtual_user_id_or_function_y_is_vugen_run
#define _vUserGroup 0_vUserGroup_no_longer_exists_please_use_y_virtual_user_group


// Loadrunner does not give you full C headers, so the 'RAND_MAX' #define from <stdlib.h>
// is missing. We define it here mostly for documentation, as we do not have access
// to the header files themselves and therefore cannot change this. 
// #define RAND_MAX 32767
//
// With some slight changes to y_rand() this constant can be increased by quite a bit ..
// 
#define RAND_MAX 1073741823

//
// This file contains loadrunner specific helper funtions.
//
// See also: string.c, logging.c, transaction.c, profile.c - all of which
// contain helper functions (grown out of this file) which cover a specific topic.
//


// --------------------------------------------------------------------------------------------------
//// Random number generator control ////



//!   y_setup()
/*!   Used by y_rand(), and (possibly) others.
Runs lr_whoami and sets vUserId and vUserGroup as global(!) variables.
\return void
\author Floris Kraak
\warning uses 2 global variables: y_virtual_user_id and y_virtual_user_group. Use with care!
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


int y_is_vugen_run()
{
    y_setup();
    return (y_virtual_user_id == -1);
}


// --------------------------------------------------------------------------------------------------


//!   Generate a random (integer) number between 0 and RAND_MAX (2147483648)
/*!   Seeds the random number generator only the first time this function is called.
\return random number (integer)
\author Floris Kraak
\start_example
int random_number;
random_number=y_rand();
\end_example
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
       //lr_log_message("y_rand: 30 random bits = %d, RAND_MAX = %d", result, RAND_MAX);

       // Doing a third call to rand() just to get 1 bit of entropy isn't really efficiënt ..
       //result = (result << 1) | (rand() & 0x0000000000000001); // add another bit and we're done.
       lr_log_message("y_rand: final random roll = %x, RAND_MAX = %d", result, RAND_MAX);

       return result;
   }
}

// --------------------------------------------------------------------------------------------------


//! Create a hash of string input. 
/*! See: http://www.cse.yorku.ca/~oz/hash.html 
\return hash (unsigned long)
\author Floris Kraak
\start_example
\end_example
*/
static unsigned long y_hash_sdbm(char* str)
{
    unsigned long hash = 0;
    int c;
    while (c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}

// --------------------------------------------------------------------------------------------------


//! Create a unique parameter that is more predictable in length than the LR counterpart: Exactly 22 characters.
/*!
@param[in] The name of a parameter to store the resulting string in.
\return void
\author Floris Kraak & André Luyer
\start_example
y_param_unique("test");
\end_example
*/
void y_param_unique(char *param)
{
/*
    struct _timeb ms;
    static unsigned short i = 0;
    static unsigned long vuser_group_hash = 0;

    if(!vuser_group_hash) 
    { 
        y_setup();
        vuser_group_hash = y_hash_sdbm(y_virtual_user_group);
    }
    ftime(&ms);

    lr_log_message("y_param_unique(%s) in group %s (hash: %x) for user %d (%x) at time %d.%03d (%x.%x) with iterator %d (%x)", 
        param, y_virtual_user_group, vuser_group_hash, y_virtual_user_id, y_virtual_user_id, ms.time, ms.millitm, ms.time, ms.millitm, i, i);

    // Exactly 24 characters. No more, no less. Close enough for our purposes, I reckon. */
    //lr_param_sprintf(param, "%08x%04x%08x%03x%01x", vuser_group_hash /*& 0xFFFFFFFF */, y_virtual_user_id & 0xFFFF, ms.time /*& 0xFFFFFFFF */, ms.millitm, i++ & 0xF);


//     char buf[23];                   // UUID's are always 22 characters, plus null byte.
//     lr_generate_uuid_on_buf(buf);
//     lr_save_var(buf, 22, 0, param); // save & trim off ==

    char buf[100]; // should be only 22 characters, but apparently not always ..
    memset(buf, '\0', sizeof(buf) * sizeof(char));
    lr_generate_uuid_on_buf(buf);
    lr_save_var(buf, strlen(buf), 0, param); // save & trim off ==
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
\sa y_random_string_buffer_core
*/
y_random_string_buffer(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 3, 8, 
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
}


// --------------------------------------------------------------------------------------------------


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
y_random_number_buffer(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 0, 0, "0123456789");
}


// --------------------------------------------------------------------------------------------------


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
y_random_string_buffer_curses(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 0, 0, "!@#$%^&*()");
}


// --------------------------------------------------------------------------------------------------


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
y_random_string_buffer_hex(const char *parameter, int minimumLength, int maximumLength)
{
   y_random_string_buffer_core(parameter, minimumLength, maximumLength, 0, 0, "0123456789ABCDEF");
}

// --------------------------------------------------------------------------------------------------


//! Check whether or not a random number lies between 2 given boundaries
/*!
Generate a random number between (and including) 0 and randMax, and tell us if that number lies 
between the lower and upper bounds or not. (attention: boundaries are included!)

This is useful for pathing decisions: Say that at point P in a script a choice has to be made
between continuing with actions A, B, and C. The decision is made based on a percentage:
A = 10% chance, B = 50% chance, C = 40% chance. This function was written to support the code
that would make this decision.
@param[in] lowerbound Minumum value
@param[in] upperbound Maximum value
@param[in] randMax Upper boundary of the random number
\author Floris Kraak
\return 0: no, random number lies outside the boundaries\n
      1: yes, random number lies inside the boundaries\n
      <0: input made no sense.
\start_example
y_rand_in_sliding_window(1, 10, 20); // Returns 1 if the random number rolled is 4, and 0 if the random number was 11.
\end_example
*/
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


//! Fetch attribute from vUser's command line.
/*!
This will fetch an attribute from the vUser's command line (as set in the scenario or in runtime settings (addition attributes))
and stores it in a parameter of the same name.
This function is a short cut of y_save_attribute_to_parameter()

@param[in] param Argument Name of the attribute.
\return A LR parameter with the same name as the Argument Name.
\author Floris Kraak
\start_example
y_save_attribute("server");
web_add_auto_filter("Action=Include", "HostSuffix={server}", LAST );
\end_example
\sa y_save_attribute_to_parameter()
*/
y_save_attribute( char* param )
{
   y_save_attribute_to_parameter( param, param );
}


// --------------------------------------------------------------------------------------------------


//! Fetch attribute from vUser's command line.
/*!
This will fetch an attribute from the vUser's command line (as set in the scenario or in runtime settings (addition attributes))
and stores it in a parameter of the same name.

@param[in] attrib Argument Name of the attribute.
@param[out] param LR-parameter name in which the Argument Value is stored.
\return A LR parameter with the same name as the Argument Name.
\author Floris Kraak
\start_example
y_save_attribute_to_parameter("server", "nice_server");
web_add_auto_filter("Action=Include", "HostSuffix={nice_server}", LAST );
\end_example
\sa y_save_attribute()
*/
y_save_attribute_to_parameter( char* attrib, char* param )
{
   char *tmp;

   if( (tmp = lr_get_attrib_string(attrib)) != NULL )
   {
      lr_save_string(tmp, param);
   }
}


// --------------------------------------------------------------------------------------------------


//! Process the result code of lr_rendezvous() call to log human readable errors.
/*!
When calling lr_rendezvous() the result code indicates the end state of the rendezvous.
This function will translate the codes into human readable errors which are then logged as normal.

@param[in] result code from lr_rendezvous()
\return None
\author Floris Kraak
\start_example
{
    int result = lr_rendezvous("03_Portefeuille");

    y_start_transaction("Rendezvous_me");
    // Perform the transaction in question.
    y_end_transaction("", LR_AUTO);

    y_log_rendezvous_result(result);
}
\end_example
\sa y_log_rendezvous_result()
*/
void y_log_rendezvous_result(int result)
{
    char *message;
    switch( result ) {
        case LR_REND_ALL_ARRIVED:
            message = "LR_REND_ALL_ARRIVED - Vuser was released after all the designated Vusers arrived.";
            break;
        case LR_REND_TIMEOUT:
            message = "LR_REND_TIMEOUT - Vuser was released after the timeout value was reached.";
            break;
        case LR_REND_DISABLED:
            message = "LR_REND_DISABLED - The rendezvous was disabled from the Controller.";
            break;
        case LR_REND_NOT_FOUND:
            message = "LR_REND_NOT_FOUND - The rendezvous was not found.";
            break;
        case LR_REND_VUSER_NOT_MEMBER:
            message = "LR_REND_VUSER_NOT_MEMBER - Vuser was not defined in the rendezvous.";
            break;
        case LR_REND_VUSER_DISABLED:
            message = "LR_REND_VUSER_DISABLED - Vuser was disabled for the rendezvous.";
            break;
        case LR_REND_BY_USER:
            message = "LR_REND_BY_USER - The rendezvous was released by the user.";
            break;
        default:
            message = "Unknown rendezvous result code.";
    }

    //lr_vuser_status_message("Rendezvous returned: %s", message);
    lr_log_message("Rendezvous returned: %s", message);
}


// --------------------------------------------------------------------------------------------------


//! Keep track of the steps in the script
/*!
Adds (another) string (read: step) to the LR-parameter {breadcrumb}
Use this to keep track of the steps taken by the script. Very useful if you have a script which
does things in a random order and you want to know (in the end) which order it used.
You can, ofcourse, write this to a (log)file.
Don't forget to use y_breadcrumb_reset() to clear the parameter at the start of the script.
(else you end up with a very long breadcrumb (breadstick?).)

@param[in] breadcrumb
\return LR parameter {breadcrumb}
\author Raymond de Jongh
\start_example
y_breadcrumb_reset();    // clean the breadcrumb-variable. (previous data in {breadcrumb} is deleted.
y_breadcrumb("start");
// .... some code....
y_breadcrumb("processing data")
//... some code ....
y_breadcrumb("finished")
The result is that {breadcrumb} contains "start;processing data;finished"   
\end_example
\sa y_breadcrumb_reset()
*/
void y_breadcrumb(char *breadcrumb)
{
    lr_message("---------------------------------------------------------------------------------");

    if( y_is_empty_parameter("breadcrumb") || ((strlen(breadcrumb) == 0)) )
    {
        lr_save_string(breadcrumb, "breadcrumb");
    }
    else
    {
        lr_param_sprintf("breadcrumb", lr_eval_string("{breadcrumb};%s"), breadcrumb);
    }
}

// --------------------------------------------------------------------------------------------------


//! Resets the breadcrumb 
/*! Use this function to start a new breadcrumb or to reset an existing one.
\author Raymond de Jongh
\sa y_breadcrumb()
*/
/*void y_breadcrumb_reset()
{
    lr_save_string("", "breadcrumb");
}*/
#define y_breadcrumb_reset() lr_save_string("", "breadcrumb")


// --------------------------------------------------------------------------------------------------


//! Write a string to a file.
/*! Write a string to a file. Creates the file if it doesn't exist. Appends to an existing file.

@param[in] filename Name of the file in which the content is saved.
@param[in] content String which is saved into the file
\return 0: everthing went fine\n
<0: failed
\author Raymond de Jongh
\start_example
int result;
result=y_write_to_file("c:\\temp.txt", "This is a test");
if (result != 0)
{   // o dear, something went wrong!
}
\end_example
\sa y_breadcrumb_reset()
*/
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


//! Saves the current date/time into a LR-parameter
/*!
Stores the current date/time into LR-parameter {DATE_TIME_STRING} in this format:\n
YYYYMMDD,HHMMSS (yes, separated by a comma.)

\return current date/time into LR-parameter {DATE_TIME_STRING}.
\author Raymond de Jongh
\start_example
y_datetime();
lr_message("Current date/time: %s", lr_eval_string("{DATE_TIME_STRING}"));
\end_example
*/
void y_datetime()
{
    lr_save_datetime("%Y%m%d,%H%M%S", DATE_NOW, "DATE_TIME_STRING");
}


// --------------------------------------------------------------------------------------------------


//! Calculate the difference in days between today and X workdays into the future.
/*!
Calculate the difference in days between today and a date X workdays into the future.

\return How many days into the future X workdays will be.
\author Floris Kraak
\start_example
// Reserve a meeting room in 'reservationOffset' days.
int daysOffSet = y_workdays_from_today( atoi(lr_eval_string("{reservationOffset}")) );
lr_save_datetime("%d-%m-%Y", DATE_NOW + (daysOffSet*ONE_DAY), "ReservationDate");

lr_vuser_status_message(
lr_eval_string("Running with offset {reservationOffset} at day offset %d"),
daysOffSet);
\end_example
*/
int y_workdays_from_today(int workdays)
{
    int weekday, weeksOffset, weekstart;
    int i = 0;
    int result = workdays;
    //int debugOffset = 0;

    // debugging loop
    //for(debugOffset = 0; debugOffset < 13; debugOffset++) {
    // result = debugOffset;

    //lr_log_message("--- result start %d ---", result);

    // Determine what day of the week today falls into.
    lr_save_datetime("%w", DATE_NOW, "weekdayToday");
    weekstart = atoi(lr_eval_string("{weekdayToday}"));
    //lr_log_message("--- weekstart = %d ---", weekstart);

    weeksOffset = result / 5;
    //lr_log_message("weeksOffset %d", weeksOffset);
    result += (2 * weeksOffset);
    //lr_log_message("Adding extra weeks weekends adds up to %d", result);

    // Determine what day of the week our target day falls into.
    lr_save_datetime("%w", DATE_NOW + result*(ONE_DAY), "weekdayFuture");
    weekday = atoi(lr_eval_string("{weekdayFuture}"));
    //lr_log_message("--- weekday = %d ---", weekday);

    // Look at each day between the day of the week that our count started on,
    // and the day of our target date. Shift the target date backwards if we find
    // a weekend.
    i = weekstart;
    do {
        // Weekend rollover
        if( i > 6) {
            i = 0;
        }
        // Weekend day found
        if( i == 0 || i == 6 ) {
            //lr_log_message("i = %d, adding 1 day", i);
            result++;
        }
        i++;
    } while( i != (weekday+1) );

    // Add another day if our target day falls on a saturday.
    // The search loop accounted for the saturday itself, but saturdays tend to be
    // followed by sundays ..
    if ( weekday == 6 ) {
        //lr_log_message("weekday = %d, adding 1 day", weekday);
        result++;
    }

    //lr_log_message( "############ Final day offset = %d ############", result);
    
    //} // end debugging loop
    return result;
}

// Load the kernel32.dll file if not already loaded, so that we can use it to query windows for disk space usage.
int y_load_kernel_dll()
{
    static int kernel_dll_loaded = 0;
    int load_dll_result;

    if( kernel_dll_loaded > 0)
    {
        // Nothing to do.
        return;
    }

    // Try to load the dll.
    if((load_dll_result = lr_load_dll("kernel32.dll")) != 0 )
    {
        lr_log_error("Unable to load kernel32.dll. Error number %d. Unable to report disk space usage.", load_dll_result);
        lr_abort();
    }
    else
    {
        kernel_dll_loaded = 1;
    }
    return load_dll_result;
}

//
// In order to prevent our logs from filling up disks beyond capacity we need to know how much free space the log file system has.
// 
// Note: This is not capable of handling disk sizes > 16 TB.
// The only way to get bigger sizes reported involves 64-bit integer variables in a 32-bit compiler that has no support for it at all.
// Fixing that is a definite TODO item, but it won't happen today, and may not happen until we get a 64-bit vugen ..
// 
double y_get_free_disk_space_percentage(char* folder_name)
{
    size_t SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
    double free_space_percentage;

    y_load_kernel_dll(); // This will abort the script if it fails to load.
    GetDiskFreeSpaceA(folder_name, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

    lr_log_message("GetDiskFreeSpaceA reports: SectorsPerCluster: %.lu, BytesPerSector: %.lu, NumberOfFreeClusters: %.lu, TotalNumberOfClusters: %.lu", 
                                               SectorsPerCluster,       BytesPerSector,       NumberOfFreeClusters,       TotalNumberOfClusters);

    free_space_percentage = 100. * NumberOfFreeClusters / TotalNumberOfClusters;
    lr_log_message("Free disk space percentage for folder %s: %.2lf%%", folder_name, free_space_percentage);

    return free_space_percentage;
}


//
// In order to prevent our logs from filling up disks beyond capacity we need to know how much free space the log file system has.
// 
// Note: This is not capable of handling disk sizes > 16 TB.
// The only way to get bigger sizes reported involves 64-bit integer variables in a 32-bit compiler that has no support for it at all.
// Fixing that is a definite TODO item, but it won't happen today, and may not happen until we get a 64-bit vugen ..
// 
double y_get_free_disk_space_in_mebibytes(char* folder_name)
{
    size_t SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
    double free_mebibytes;


    y_load_kernel_dll(); // This will abort the script if it fails to load.
    GetDiskFreeSpaceA(folder_name, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

    lr_log_message("GetDiskFreeSpaceA reports: SectorsPerCluster: %.lu, BytesPerSector: %.lu, NumberOfFreeClusters: %.lu, TotalNumberOfClusters: %.lu", 
                                               SectorsPerCluster,       BytesPerSector,       NumberOfFreeClusters,       TotalNumberOfClusters);

    free_mebibytes = (double)NumberOfFreeClusters / 1048576. * SectorsPerCluster * BytesPerSector; // Pas op voor overflows bij aanpassingen.
    lr_log_message("Free disk space for folder %s : %f MebiBytes)", folder_name, free_mebibytes);
    return free_mebibytes;
}


y_read_file_into_parameter(char* filename, char* param)
{
    long pos;
    char *bytes;
    long f = fopen(filename, "rb");

    if( f == NULL )
    {
        lr_error_message("Unable to open file %s", filename);
        lr_abort();
        return;
    }

    fseek(f, 0, SEEK_END);
    pos = ftell(f);
    fseek(f, 0, SEEK_SET);

    bytes = y_mem_alloc(pos);
    fread(bytes, pos, 1, f);
    fclose(f);

    // hexdump(bytes); // do some stuff with it

    // Save the size for later use
    lr_param_sprintf("y_byte_size_param_name", "y_size_%s", param);
    lr_save_int(pos, lr_eval_string("{y_byte_size_param_name}"));

    lr_save_var(bytes, pos, 0, param);
    free(bytes); // free allocated memory
}

y_user_data_point(char* param)
{
    lr_user_data_point(param, atof(y_get_parameter(param)) );
}


// For simulating situations with limited amounts of connections on the client side. 
//
// In such a case we cannot use regular vuser based rampups, so instead we have to gradually lower the thinktime to get a similar effect.
// This implements a "think time" function that will use response time calculations and a TPS target to get simulate such a ramp up.
// 
// Base formula:
// 
//     TT = threads/TPS(target) - response time, 
// 
// while increasing TPS linearly until rampup_period has passed, until either think time reaches zero or TPS_max is reached.
// Note that "response time" is actually "time passed since the previous call to this function". This includes wasted time, for example.
// 
// For best effect, call this once in vuser_init and once for each transaction, preferably by overloading lr_think_time()
// with your own version that calls this (or the simplified wrapper, below) every time, like so:
// 
// void my_little_think_time()
// { 
//     y_think_time_for_rampup(1800, 10);
// }
//
// 
// Parameters ( = recommended default ):
//   const int rampup_period = 1800;        -  How long the rampup should take, in seconds.
//   double TPS_initial = 0.1;              -  Start rampup with this many transactions / sec for this transaction, across all virtual users. 
//                                             Do not set this too low or thinktime values may get really large.
//   double TPS_max = 10;                   -  End rampup with this many transactions / sec in total, with 'virtual_users' users. This cannot be higher than 1/average response time.
//                                             Note that the actual TPS achieved is highly dependant on what mix of thinktimes and transactions are used.
//                                             Notably, this assumes you only have one transaction after each call to this function, and that each call uses the same parameters.
//   const int virtual_users = 1;           -  How many virtual users the script is using. If you use "1", you can just use TPS / virtual user for the initial target TPS.
//
double y_think_time_for_rampup_ext(const int rampup_period, double TPS_initial, double TPS_max, const int virtual_users)
{
    static double test_start_time = 0;             // Test starttime in seconds since 1 jan 1970.
    static double previous_time = 0;               // Timestamp of the last call to this, after think time.
    struct _timeb ts;                              // timeb buffer to hold the current time.
    double current_time;                           // Current time, in seconds since 1 jan 1970.
    double time_passed;                            // Elapsed time since test start, in seconds.
    double response_time;                          // Elapsed time since previous call.

    ftime(&ts);
    current_time = ts.time + (ts.millitm / 1000);

    // Initialisation.
    // On the first call we store the current time as the test start time and the end time of the previous call.
    if( test_start_time < 1 )
    {
        test_start_time = current_time;
        previous_time = current_time;
    }


    // Calculate how much time has passed since test start and the previous call.
    time_passed = current_time - test_start_time;
    response_time = current_time - previous_time;

    // Debugging//
    lr_log_message("TT calculation: starttime %f, current time %f, previous time %f, virtual_users %d, rampup_period %d",
                                 test_start_time, current_time,    previous_time,    virtual_users,    rampup_period);


    {
        double TT;
        double TPS_target;
        double factor = time_passed / rampup_period; // multiplication factor for the target load, based on rampup.

        if( factor > 1 ) // after rampup load should be stable.
            factor = 1;
        else if( factor < 0.03 )  // To prevent extreme thinktimes the first 3% of the rampup thinktime is calculated as if 3% of the time has passed.
            factor = 0.03;

        // double TPS_max = virtual_users * (1/response_time); // this is the absolute theoretical maximum, which we aren't using right now.
        TPS_target = ((TPS_max - TPS_initial) * factor) + TPS_initial; // Apply the ramp up to get the target TPS.
        TT = (virtual_users / TPS_target) - response_time;             // Apply the base formula to get the resulting think time

        // Debugging.
        lr_log_message("TT: %f, time_passed: %f, factor %f, response_time %f, TPS_init %f, TPS_max %f, TPS_target %f", 
                        TT,     time_passed,     factor,    response_time,    TPS_initial, TPS_max,    TPS_target);

        lr_user_data_point("ThinkTime", TT);
        if( TT > 0 )
            lr_think_time(TT);

        // Store the old time in our backup location.
        //previous_time = current_time; 
        // Do a new time measurement..
        //ftime(&ts);
        //current_time = ts.time + (ts.millitm / 1000);

        // We measure the response time by comparing previous_time to a new measurement later, but that measurement 
        // will have to be schewed a tad to account for rounding in the windows sleep() call used by lr_think_time().
        //{
            //double time_slept = current_time - previous_time;
            //double deviation = TT - time_slept;
            //previous_time = current_time + deviation;
            //lr_log_message("Slept %f sec, deviation %f, current_time %f with deviation added: %f", time_slept, deviation, current_time, previous_time);
        //}

        // All of the above can be rewritten to:
        previous_time = current_time + TT; // Note that the "current time" actually is the timestamp from *before* we slept.

        return TT;
    }
}


// For simulating situations with limited amounts of connections on the client side. 
// As y_think_time_for_rampup_ext(), assuming 1 virtual user and an initial target load of 0.1 TPS.
// 
// Parameters:
//   const int rampup_period = 1800;        // How long the rampup should take, in seconds.
//   double TPS_max = 10;                   // End rampup with this many transactions / sec in total, across all virtual users. This cannot be higher than 1/average response time
//
double y_think_time_for_rampup(const int rampup_period, double TPS_max)
{
    // These used to be parameters. I've kept them for documentation purposes, but I feel 
    // these defaults are sane enough to allow me to internalize them.
    const double TPS_initial = 0.1;                // Start rampup with this many transactions / sec in total, across all virtual users.
    const int virtual_users = 1;                   // How many virtual users the script is using. If you use "1", you can just use TPS / virtual user for the initial target TPS.

    return y_think_time_for_rampup_ext(rampup_period, TPS_initial, TPS_max, virtual_users);
}


// --------------------------------------------------------------------------------------------------
#endif // _LOADRUNNER_UTILS_C
