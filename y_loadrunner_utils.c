/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2014 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

/*! 
\file y_loadrunner_utils.c
\brief Collection of miscellaneous support functions.

This file contains loadrunner specific helper functions.
If we don't have somewhere else to put some piece of code, this is where it will probably end up.
*/
#ifndef _LOADRUNNER_UTILS_C
//! \cond include_protection
#define _LOADRUNNER_UTILS_C
//! \endcond

#include "vugen.h"
#include "y_core.c"

//! \cond function_removal
#define _vUserID 0_vUserID_no_longer_exists_please_use_y_virtual_user_id_or_function_y_is_vugen_run
#define _vUserGroup 0_vUserGroup_no_longer_exists_please_use_y_virtual_user_group
//! \endcond function_removal


/*! \brief Create a hash of string input. 

A fairly simple hash that can be used to compare long input texts with each other.
This uses the sdbm algorithm, described at http://www.cse.yorku.ca/~oz/hash.html.

\param [in] str The string to create a hash from.
\sa http://www.cse.yorku.ca/~oz/hash.html 
\return hash (unsigned long)
\author Floris Kraak
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


/*! \brief Generate random number and test if it lies between 2 given boundaries.

This will generate a random number between (and including) 0 and rand_max, and tell us if that number lies between the lower and upper bounds or not. 
Boundaries are included!

This is useful for pathing decisions: Say that at point P in a script a choice has to be made
between continuing with actions A, B, and C. The decision is made based on a percentage:
A = 10% chance, B = 50% chance, C = 40% chance. This function was written to support the code
that would make this decision.

\note y_flow_list.c provides a better and more intuitive alternative for this one.
For that reason, we are likely to remove it in the near future.

@deprecated
@param[in] lowerbound Minumum value
@param[in] upperbound Maximum value
@param[in] rand_max Upper boundary of the random number
\author Floris Kraak
\return 0: no, random number lies outside the boundaries\n
        1: yes, random number lies inside the boundaries\n
       <0: input made no sense.
\code
y_rand_in_sliding_window(1, 10, 20); // Returns 1 if the random number rolled is 4, and 0 if the random number was 11.
\endcode
\sa y_rand(), y_flow_list.c
*/
int y_rand_in_sliding_window(int lowerbound, int upperbound, int rand_max)
{
    int roll;

    if( (0>lowerbound) || (lowerbound>upperbound) || (upperbound > rand_max) || (rand_max <= 0))
    {
        lr_error_message("y_rand_in_sliding_window called with nonsensical arguments: ( 0 <= %d < %d <= %d ) == FALSE",
                        lowerbound, upperbound, rand_max);
        return -1;
    }

    roll = y_rand_between(0, rand_max);
    if( (roll >= lowerbound) && (roll <= upperbound) )
    {
        return 1;
    }

    return 0;
}


// --------------------------------------------------------------------------------------------------


//! Create a random number between two boundaries. (the boundaries are included!)
/*! When the lower boundary equals the upper boundary, y_rand_between() simply returns the lower boundary.
@param[in] lowerbound Lower boundary of the generated number
@param[in] upperbound Upper boundary of the generated number
\return random number

\b Example:
\code
int random;        
random = y_rand_between(0, 10);        // generate a random number between 0 and 10 (including 0 and 10!)
\endcode
\sa y_rand()
\author Floris Kraak
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


/*! \brief Fetch attribute from vUser's command line and store it in a parameter.
This will fetch an attribute from the vUser's command line (as set in the scenario or in runtime settings as additional attributes) and stores it in the given parameter.

\param[in] attrib Attribute name.
\param[out] param LR parameter name in which the argument value is stored. 
\return 1 if the attribute exists, -1 if it doesn't, in which case no value is stored in the parameter.

\b Example:
\code
y_save_attribute_to_parameter("server", "nice_server");
web_add_auto_filter("Action=Include", "HostSuffix={nice_server}", LAST );
\endcode
\sa y_save_attribute()
\author Floris Kraak
*/
int y_save_attribute_to_parameter( char* attrib, char* param )
{
   char *tmp = lr_get_attrib_string(attrib);
   if( tmp  == NULL )
      return -1;
   else
   {
      lr_save_string(tmp, param);
      return 1;
   }
}

// --------------------------------------------------------------------------------------------------


//! 
/*! \brief Fetch attribute from vUser's command lin and store it in a parameter of the same name.
This will fetch an attribute from the vUser's command line (as set in the scenario or in runtime settings (addition attributes)) and stores it in a parameter of the same name.
This function is a shortcut to y_save_attribute_to_parameter()

\param[in] param argument name of both the attribute and the parameter the value should be stored in.
\return 1 if the attribute exists, -1 if it doesn't, in which case no value is stored in the parameter.

\b Example:
\code
y_save_attribute("server");
web_add_auto_filter("Action=Include", "HostSuffix={server}", LAST );
\endcode
\sa y_save_attribute_to_parameter()
\author Floris Kraak
*/
int y_save_attribute( char* param )
{
   return y_save_attribute_to_parameter( param, param );
}


// --------------------------------------------------------------------------------------------------


//! Process the result code of lr_rendezvous() call to log human readable errors.
/*!
When calling lr_rendezvous() the result code indicates the end state of the rendezvous.
This function will translate the codes into human readable errors which are then logged as normal.

@param[in] result code from lr_rendezvous()
\return None
\code
{
    int result = lr_rendezvous("03_Portefeuille");

    y_start_transaction("Rendezvous_me");
    // Perform the transaction in question.
    y_end_transaction("", LR_AUTO);

    y_log_rendezvous_result(result);
}
\endcode
\author Floris Kraak
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
does things in a random order and you want to know (in the end) which order it actually used.
You can, of course, write this to a (log)file.
Don't forget to use y_breadcrumb_reset() to clear the parameter at the start of the script.
(else you end up with a very long breadcrumb (breadstick?).)

@param[in] breadcrumb
\return LR parameter {breadcrumb}
\code
y_breadcrumb_reset();    // clean the breadcrumb-variable. (previous data in {breadcrumb} is deleted.
y_breadcrumb("start");
// .... some code....
y_breadcrumb("processing data")
//... some code ....
y_breadcrumb("finished")
The result is that {breadcrumb} contains "start;processing data;finished"   
\endcode
\sa y_breadcrumb_reset()
\author Raymond de Jongh
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


/*!
\def y_breadcrumb_reset()
\brief Resets the breadcrumb 
Use this macro to start a new breadcrumb or to reset an existing one.
\author Raymond de Jongh
\sa y_breadcrumb()
*/
#define y_breadcrumb_reset() lr_save_string("", "breadcrumb")


// --------------------------------------------------------------------------------------------------


//! Write a string to a file.
/*! Write a string to a file. Creates the file if it doesn't exist. Appends to an existing file.

@param[in] filename Name of the file in which the content is saved.
@param[in] content String which is saved into the file
\return 0: everthing went fine\n
<0: failed
\code
int result;
result=y_write_to_file("c:\\temp.txt", "This is a test");
if (result != 0)
{   // o dear, something went wrong!
}
\endcode
\author Raymond de Jongh
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

\deprecated
\note This doesn't add much. People should be able to call lr_save_datetime() themselves, right?
Putting it on the deprecation list for that reason.
\code
y_datetime();
lr_message("Current date/time: %s", lr_eval_string("{DATE_TIME_STRING}"));
\endcode
\sa y_get_current_time()
\author Raymond de Jongh
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

\b Example:
\code
// Reserve a meeting room in 'reservationOffset' days.
int daysOffSet = y_workdays_from_today( atoi(lr_eval_string("{reservationOffset}")) );
lr_save_datetime("%d-%m-%Y", DATE_NOW + (daysOffSet*ONE_DAY), "ReservationDate");

lr_vuser_status_message(lr_eval_string("Running with offset {reservationOffset} at day offset %d"), daysOffSet);
\endcode
\author Floris Kraak
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

/*!
\brief Load the kernel32.dll file if not already loaded.
Internal function that we need so so that we can use kernel32.dll to query windows for disk space usage.

\author Floris Kraak
\sa y_disk_space_guard(), y_disk_space_usage_guard()
*/
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

/*! \brief Get the free diskspace percentage on the target folder on the load generator.

In order to prevent loadrunner logs from filling up disks on the generator beyond capacity we need to know how much free space the log file system has.
This function provides that number, relative to the total disk space size.

\param [in] folder_name The name of the folder to report on.
\return The percentage of free space, as a double.

\note This is not capable of handling disk sizes > 16 TB.
The only way to get bigger sizes reported involves 64-bit integer variables in a 32-bit compiler that has no support for it at all.
Fixing that is a definite TODO item, but it won't happen today, and may not happen until we get a 64-bit vugen ..
\sa y_get_free_disk_space_in_mebibytes()
\author Floris Kraak
*/
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

/*! \brief get the amount of free disk space in the target folder in mebibytes (SI unit)

In order to prevent loadrunner logs from filling up disks on the generator beyond capacity we need to know how much free space the log file system has.
This function provides that number, in absolute terms.

\param [in] folder_name The name of the folder to report on.
\return The amount of mebibytes free, as a double.

\note This is not capable of handling disk sizes > 16 TB.
The only way to get bigger sizes reported involves 64-bit integer variables in a 32-bit compiler that has no support for it at all.
Fixing that is a definite TODO item, but it won't happen today, and may not happen until we get a 64-bit vugen ..
\sa y_get_free_disk_space_percentage()
\author Floris Kraak
*/
double y_get_free_disk_space_in_mebibytes(char* folder_name)
{
    size_t SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
    double free_mebibytes;


    y_load_kernel_dll(); // This will abort the script if it fails to load.
    GetDiskFreeSpaceA(folder_name, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

    lr_log_message("GetDiskFreeSpaceA reports: SectorsPerCluster: %.lu, BytesPerSector: %.lu, NumberOfFreeClusters: %.lu, TotalNumberOfClusters: %.lu", 
                                               SectorsPerCluster,       BytesPerSector,       NumberOfFreeClusters,       TotalNumberOfClusters);

    free_mebibytes = (double)NumberOfFreeClusters / 1048576. * SectorsPerCluster * BytesPerSector; // Beware for overflows when changing this.
    lr_log_message("Free disk space for folder %s : %f MebiBytes)", folder_name, free_mebibytes);
    return free_mebibytes;
}

/*! \brief Read the contents of a file into a single parameter.
\param [in] filename The name of the file to read (relative to script root, or full path)
\param [in] param The name of the parameter to store the file contents in.
\return The number of bytes read from the file, or -1 if the file could not be read (but in that case it will also call lr_abort().)

\note The size of the file is stored in a parameter named "y_size_{param}", where {param} is the name of the parameter you passed in.

This can be used in various situations; For example, situations where you have a template file that you wish to use for various requests.
For instance, when all communications are based on a simple XML submit with slightly different (parameterised) contents.

\note If the file cannot be opened this will call lr_abort().

\b Example:
\code
vuser_init()
{
    // File contains: <xml><customer_element>{customer_id}</customer_element></xml>
    y_read_file_into_parameter("template.xml", "template_content");
}

projectname_some_request_transaction()
{
   // double evaluation evaluates the customer id, giving you the request body.
   lr_save_string(lr_eval_string(lr_eval_string("{template_content}")), "request_body");

   y_start_transaction("some_request");
   web_custom_request("some_request", 
		"URL=http://{Host}/some/request/v1/", 
		"Method=POST", "Resource=0", 
		"Body={request_body}",
		"EncType=text/xml; charset=utf-8",
		LAST);   
   y_end_transaction("", LR_AUTO);
}
\endcode
\author Floris Kraak
*/
int y_read_file_into_parameter(char* filename, char* param)
{
    long pos;
    char *bytes;
    long f = fopen(filename, "rb");

    if( f == NULL )
    {
        lr_error_message("Unable to open file %s", filename);
        lr_abort();
        return -1;
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
    return pos;
}

/*! \brief Create a user data point for a parameter.
\param [in] param The name of a parameter containing an integer or floating point number.

The name of the parameter will be the name of the datapoint that was created.

\b Example
\code
lr_save_int(1, "one");
y_user_data_point("one"); // Creates a datapoint named "one", containing a value of 1.
\endcode
\sa lr_user_data_point()
\author Floris Kraak
*/
void y_user_data_point(char* param)
{
    lr_user_data_point(param, atof(y_get_parameter(param)) );
}

/*! \brief Get the current time in seconds since 1970, as a double.
\returns The current time in seconds since 1970-01-01 00:00
\author Floris Kraak
*/
double y_get_current_time()
{
    struct _timeb timebuffer;
    ftime(&timebuffer);
    return timebuffer.time + (timebuffer.millitm / 1000);
}


/*! \brief Delay until a certain time.
\returns the amount of time waited, in seconds.

\author Floris Kraak
*/ 
double y_delay_until(double timestamp)
{
    double current_time = y_get_current_time();

    if( current_time < timestamp )
    {
        double result = timestamp - current_time;
        lr_force_think_time(result);
        return result;
    }
    else
        return 0;
}

/*! \def y_delay_once
\brief Delay for delay_in_seconds seconds, once.
This macro will put the code to sleep for the specified amount of seconds, but only the first time the current position in the code is reached.
If you use this in more than one place each of the calls will sleep exactly once.

The particular usecase this was made for is a case where users need to register themselves prior to the actual test run.
We want to seperate this registration period from the actual rampup, so when each user finishes registering we want a delay. 
But, we only want it once, and we will only know that we're done registering when we hit the regular load path for the first time. 

Hence: delay_once
 
This is a macro because a function call would make it impossible to use this more than once in a script - it would execute the delay the first time this function is called,
independent of where it was called *from*. Which would be rather counterintuitive.
\author Floris Kraak
*/
#define y_delay_once( delay_in_seconds )       \
{                                              \
    static int delay_done = 0;                 \
                                               \
    if( !delay_done )                          \
    {                                          \
        delay_done = 1;                        \
        lr_force_think_time(delay_in_seconds); \
    }                                          \
}


/*! \brief Ramp up the load by using varying amounts of think time instead of virtual users.

This can be used for simulating situations with limited amounts of connections on the client side. 
In such a case we cannot use regular vuser based rampups, so instead we have to gradually lower the thinktime to get a similar effect.
This implements a "think time" function that will use responsetime calculations and a TPS target to simulate such a ramp up.
 
Base formula:
    TT = threads/TPS(target) - response time

This will be increasing TPS linearly until rampup_period has passed, unless either think time reaches zero or TPS_max is reached.
Note that "response time" is actually "time passed since the previous call to this function". This includes wasted time.

For best effect, call this once in vuser_init and once for each transaction.
The recommended practice is to overload lr_think_time() with your own version that calls this (or y_think_time_for_rampup()) every time, like so:

\note This will create a user datapoint called y_thinktime that can be used for monitoring the calculations during the test.

\b Example:
\code
// Call this instead of lr_think_time(), before each transaction.
void my_think_time()
{ 
    y_think_time_for_rampup(1800, 10); // delays for a variable amount of time depending on target load.
}
\endcode

\warning The actual load achieved is highly dependant on what pacing settings, thinktimes, and calls are used. 
This assumes you only have one transaction after each call to this function, and that each call uses the same parameters.

\note The first 3% of the rampup period uses think times calculated as if 3% of the rampup has passed. This is prevents extremely long think times from occurring.

\param [in] rampup_period How long the rampup should take, in seconds. Default 1800 seconds.
\param [in] TPS_initial Start rampup with this many transactions / sec for this transaction, across all virtual users. Default 0,1 TPS.
\param [in] TPS_max End rampup with this many transactions / sec in total per virtual user. Default 10. This cannot go higher than 1/average response time - when think time reaches zero.
\param [in] virtual_users How many virtual users the script is using. If you use "1", you can just use TPS / virtual user for the initial target TPS. Default 1.

\warning Do not set TPS_initial lower than 0,1 or the calculated think times may get really large.

\author Floris Kraak
*/
double y_think_time_for_rampup_ext(const int rampup_period, double TPS_initial, double TPS_max, const int virtual_users)
{
    static double test_start_time = 0;               // Test starttime in seconds since 1 jan 1970.
    static double previous_time = 0;                 // Timestamp of the last call to this, after think time.
    double time_passed;                              // Elapsed time since test start, in seconds.
    double response_time;                            // Elapsed time since previous call.
    double current_time = y_get_current_time();      // Current time, in seconds since 1 jan 1970.

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
        double TT, TPS_target;
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

        lr_user_data_point("y_thinktime", TT);
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

/*! \brief Ramp up the load by using varying amounts of think time instead of virtual users.
For simulating situations with limited amounts of connections on the client side.

As y_think_time_for_rampup_ext(), assuming 1 virtual user and an initial target load of 0.1 TPS (the defaults.
\sa y_think_time_for_rampup_ext()

\param [in] rampup_period How long the rampup should take, in seconds. Default 1800 seconds.
\param [in] TPS_max End rampup with this many transactions / sec in total per virtual user. Default 10. This cannot go higher than 1/average response time - when think time reaches zero.
\author Floris Kraak
*/
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
