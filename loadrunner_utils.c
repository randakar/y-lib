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
//    y_datetime()
//      Simply returns the current date-time as a string, in this format:
//        YYYYMMDD,HHMMSS (yesss, separated by a comma. That is most suitable for this moment.
// @author: Raymond de Jongh
// 
// Comment: Ray, please have a look at lr_save_datetime() for me will you? Thanks ;-)
//           -- Floris
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
					  // 
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
#endif // _LOADRUNNER_UTILS_C
