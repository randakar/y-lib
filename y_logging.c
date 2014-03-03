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
#ifndef _LOGGING_C
#define _LOGGING_C

#include "vugen.h"
#include "y_loadrunner_utils.c"

// Constants
// const unsigned int Y_ALL_LOG_FLAGS = LR_MSG_CLASS_BRIEF_LOG | LR_MSG_CLASS_EXTENDED_LOG | LR_MSG_CLASS_RESULT_DATA | LR_MSG_CLASS_PARAMETERS | LR_MSG_CLASS_FULL_TRACE | LR_MSG_CLASS_JIT_LOG_ON_ERROR | LR_MSG_CLASS_AUTO_LOG;


// Global variables
int _y_extra_logging = 0;                             // client specific logging code on/off switch; 0 = off, 1 = on
unsigned int _y_log_level = LR_MSG_CLASS_DISABLE_LOG; // previous loglevel for use with log toggle functions.


// --------------------------------------------------------------------------------------------------
//// Time/date/stamp functions


/*
 * Convert a unixtime style timestamp to a date and time represented as YYYY-MM-DD HH:MM:SS.mmm.
 * @param time - the unix time stamp
 * @param millitm - the milliseconds belonging to the time stamp
 */
char* y_make_datetimestamp(time_t time, unsigned short millitm)
{
    struct tm *resulttime;
    static char YMDHMSm[24]; // moet static zijn om te gebruiken als returnwaarde

    // _tzset();  // The tzset function initializes the tzname variable from the value of the TZ environment variable. It is not usually necessary for your program to call this function, because it is called automatically when you use the other time conversion functions that depend on the time zone. 
    resulttime = (struct tm *)localtime(&time);

    snprintf(YMDHMSm, sizeof YMDHMSm, "%04u-%02u-%02u %02u:%02u:%02u.%03u", 
        resulttime->tm_year + 1900,
        resulttime->tm_mon + 1,
        resulttime->tm_mday,
        resulttime->tm_hour,
        resulttime->tm_min,
        resulttime->tm_sec,
        millitm);
    return YMDHMSm;
}

/*
 * Returns the current date and time represented as YYYY-MM-DD HH:MM:SS.mmm.
 */
char* y_get_datetimestamp()
{
    struct _timeb timebuffer;
    // _tzset();  // The tzset function initializes the tzname variable from the value of the TZ environment variable. It is not usually necessary for your program to call this function, because it is called automatically when you use the other time conversion functions that depend on the time zone. 
    ftime( &timebuffer );
    return y_make_datetimestamp( timebuffer.time, timebuffer.millitm);
}

//
// Hmn, do we even still need this?
// 
// -- removed. See y_get_current_time() in y_loadrunner_utils.c
// 
// time_t y_timestamp()
// {
//     return time(NULL);
// }
#define y_timestamp 0_y_timestamp_no_longer_exists_please_use_y_get_current_time



// --------------------------------------------------------------------------------------------------


void y_setup_logging()
{
    y_setup();

    // Make the extra logging facility available to the user.
    _y_extra_logging = 1;
}

// Force a line to be logged to the logfile even if logging is off.
// Only done if extra logging was enabled through a call to y_setup_logging();
// 
void y_log_to_report(char *message)
{
    char *logLine = "%s: VUserId: %d, Host: %s, %s";

    // Only add extra logging if it has been turned on.
    if( _y_extra_logging ) 
    {
        int log_level = lr_get_debug_message();

        lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG | LR_MSG_CLASS_AUTO_LOG, LR_SWITCH_OFF);
        lr_set_debug_message(
            LR_MSG_CLASS_EXTENDED_LOG | LR_MSG_CLASS_RESULT_DATA | LR_MSG_CLASS_PARAMETERS | LR_MSG_CLASS_FULL_TRACE,
            LR_SWITCH_ON);


        lr_log_message(logLine, y_get_datetimestamp(), y_virtual_user_id, lr_get_host_name(), lr_eval_string(message));

        lr_set_debug_message(log_level, LR_SWITCH_ON);
        //lr_set_debug_message((log_level ^ -1), LR_SWITCH_OFF);
    }
}

void y_log_error(char *message)
{
    char *msg = lr_eval_string(message);
    y_log_to_report(msg);
    //lr_error_message(msg); 
    lr_fail_trans_with_error(msg);
}

void y_log_warning(char *message)
{
    char *msg;
    lr_save_string(lr_eval_string(message), "_log_msg");
    msg = lr_eval_string("Warning: {_log_msg}");

    y_log_to_report(msg);
}

void y_log_save()
{
    // Save the previous loglevel.
    _y_log_level = lr_get_debug_message();
    //lr_error_message("Saved loglevel %d", _y_log_level);
}

void y_log_turn_off_without_saving()
{
    // Lots of debug code here. Good grief, this debug message interface is terrible.

    //unsigned int a, b, c, d, e;

    //lr_log_message("Auto log has value %d", LR_MSG_CLASS_AUTO_LOG);

    // First disable the 'extended' log types.
    //lr_set_debug_message(LR_MSG_CLASS_RESULT_DATA | LR_MSG_CLASS_PARAMETERS | LR_MSG_CLASS_FULL_TRACE, LR_SWITCH_OFF);
    //a = lr_get_debug_message();

    // Set it to 'brief' so we cannot be in extended logging anymore.
    //lr_set_debug_message(LR_MSG_CLASS_BRIEF_LOG, LR_SWITCH_ON);
    //b = lr_get_debug_message();

    // Disable the auto logging
    //lr_set_debug_message(LR_MSG_CLASS_JIT_LOG_ON_ERROR, LR_SWITCH_OFF);
    //a = lr_get_debug_message();

    // None of the above seems necessary, after extensive testing in LR 11.
    // No guarantees for earlier loadrunner versions though ..

    // Turn all logging off. Which has the really weird side effect of turning auto logging on again.
    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_ON);
    //a = lr_get_debug_message();

    // Disable the auto logging again, but using a different bit flag.
    lr_set_debug_message(LR_MSG_CLASS_AUTO_LOG, LR_SWITCH_OFF);
    //b = lr_get_debug_message();

    //lr_error_message("Log stages: a=%d, b=%d", a, b);

    //lr_error_message("All logging turned off, log level now %d", lr_get_debug_message() );
}


// Save the current loglevel and turn off logging.
void y_log_turn_off()
{
    y_log_save();
    y_log_turn_off_without_saving();
}

void y_log_turn_off_permanently()
{
    y_log_turn_off_without_saving();
    y_log_save(); // make sure it is never accidentally enabled again through y_log_restore()
}

void y_log_set_brief()
{
    //lr_log_message("Log level set to BRIEF.\n");

    y_log_turn_off();
    lr_set_debug_message(LR_MSG_CLASS_BRIEF_LOG, LR_SWITCH_ON);

    //lr_log_message("Log level set to brief, from %d to %d", _y_log_level, lr_get_debug_message() );
}

void y_log_set_extended()
{
    //lr_log_message("Log level set to EXTENDED.\n");

    y_log_turn_off();
    lr_set_debug_message(
        LR_MSG_CLASS_EXTENDED_LOG | LR_MSG_CLASS_RESULT_DATA | LR_MSG_CLASS_PARAMETERS | LR_MSG_CLASS_FULL_TRACE,
        LR_SWITCH_ON);

    //lr_log_message("Log level extended from %d to %d", _y_log_level, lr_get_debug_message() );
}

// Restore the log level to the old state.
void y_log_restore()
{
    //lr_log_message("Restoring log level to %d, current level %d.", _y_log_level, lr_get_debug_message());

    y_log_turn_off_without_saving();

    lr_set_debug_message(_y_log_level, LR_SWITCH_ON);

    // None of this debug code does what we want. Only y_log_turn_off_without_saving() (above) seems to work ..
    //lr_set_debug_message(~Y_ALL_LOG_FLAGS, LR_SWITCH_OFF);
    //lr_set_debug_message(~_y_log_level, LR_SWITCH_OFF); 
    //lr_set_debug_message(~_y_log_level & Y_ALL_LOG_FLAGS, LR_SWITCH_OFF);

    //lr_error_message("Attempted to restore loglevel to %d, now it is %d", _y_log_level, lr_get_debug_message());
    // Of course if the previous state was "OFF" the user will never see this either ;-)
}

void y_log_turn_on()
{
    if(_y_log_level == LR_MSG_CLASS_DISABLE_LOG)
    {
        y_log_set_extended();
    }
    else
    {
        y_log_restore();
    }
}

// Log a message forcefully, bypassing all log settings.
// Typically used for generating datafiles within scripts with the loglevel set to OFF.
// 
// Testcase:
//  lr_log_message("Before");
//  y_log_force_message("Forced message");
//  lr_log_message("After");
// 
void y_log_force_message(char *message)
{
    y_log_set_extended();
    lr_log_message( message );
    y_log_restore();

    //lr_error_message("Forced message done");
}

// --------------------------------------------------------------------------------------------------
// y_write_to_log()
//     writes "content" (a string) to a (log)file.
//     The content will start with current date, time, Vuser-group, VuserId-number and Scenario-ID
//        separated by commas. This function relies on y_write_to_file();
//
// @author: Raymond de Jongh
// Example:
//     y_write_to_log("c:\\logfile.txt", "Everything went great");
//     This will result that the file (c:\logfile.txt) has this content:
//        20091126,215212,SomeGroup,3,4,Everything went great
//     Ofcourse, the ID's and groupname will be different as this is just an example.
// --------------------------------------------------------------------------------------------------
int y_write_to_log(char *filename, char *content)
{
    size_t string_length;
    char* log;
    int result;

    y_setup();

    string_length = strlen(content)
      + 15  // y_datetime() is altijd 15 chars lang.
      + strlen(y_virtual_user_group) 
      + 9   // y_virtual_user_id
      + 6   // scid
      + 1;  // null byte (end of line)

    log = y_mem_alloc(string_length);
    y_datetime();
    snprintf(log, string_length, "%.15s,%s,%d,%d,%s", 
       lr_eval_string("{DATE_TIME_STRING}"), 
       y_virtual_user_group, 
       abs(y_virtual_user_id) % 1000000000, 
       abs(y_scid) % 1000000, 
       content);

    result = y_write_to_file(filename, log);
    free(log);

    return result;
}

//
// Detects low disk space situations and turns all logging off if not enough space is left.
//
void y_disk_space_guard(double max_free_percentage)
{
    char* hostname;
    static int disk_space_warning_given = 0;
    double free_space_percentage;
    char* log_folder = lr_get_attrib_string("out");

    // check already fired once before.
    if( disk_space_warning_given )
    {
        // guard against accidental re-enablement of the logs by turning them off explicitly.
        y_log_turn_off_permanently();
        return;
    }

    free_space_percentage = y_get_free_disk_space_percentage(log_folder);

    // data point
    hostname = lr_get_host_name();
    lr_save_string(hostname, "y_hostname_generator");
    lr_user_data_point( lr_eval_string("disk_space_{y_hostname_generator}_free_percentage"), free_space_percentage);

    if( free_space_percentage < max_free_percentage )
    {
        y_setup();
        lr_set_transaction(lr_eval_string("---DISK SPACE LOW IN LOG FOLDER FOR {y_hostname_generator}---"), 0, LR_FAIL);
        lr_error_message("Diskspace low on %s in folder %s. %.2lf%% remaining, exceeding the limit of %.21f%% Logging turned off for user id %d for the remainder of the test!", 
                         hostname, log_folder, free_space_percentage, max_free_percentage, y_virtual_user_id);
        disk_space_warning_given = 1; // turn off further warnings.
        y_log_turn_off_permanently();
    }
}

//
// Detects excessive disk usage on the output disk by the test
// by storing the maximum detected free space and comparing that to the current
// once max disk usage is exceeded all logging is turned off and an error reported.
// 
void y_disk_space_usage_guard(double limit_mebibytes_used)
{
    char* hostname;
    char* log_folder = lr_get_attrib_string("out");
    double free_mebibytes;
    static double max_free_mebibytes = -1;
    double mebibytes_used;
    static int disk_space_warning_given = 0;

    // check already fired once before.
    if( disk_space_warning_given )
    {
        // guard against accidental re-enablement of the logs by turning them off explicitly.
        y_log_turn_off_permanently();
        return;
    }


    free_mebibytes = y_get_free_disk_space_in_mebibytes(log_folder);
    lr_log_message("y_disk_space_usage_guard: current free: %f MB, max free: %f MB, limit: %f MB used in folder: %s",
                   free_mebibytes, max_free_mebibytes, limit_mebibytes_used, log_folder);

    if(max_free_mebibytes < 0)
    {
        lr_log_message("Storing free space as detected maximum");
        max_free_mebibytes = free_mebibytes;
        return;
    }
    else if(max_free_mebibytes < free_mebibytes)
    {
        lr_output_message("Warning: Free disk space increased from %f to %f, test disk space usage measurements may have become unreliable.", max_free_mebibytes, free_mebibytes);
        max_free_mebibytes = free_mebibytes;
        return;
    }

    // Ok, so we used *something*. Now let's see if it exceeds our limit.
    mebibytes_used = max_free_mebibytes - free_mebibytes;

    // data points
    hostname = lr_get_host_name();
    lr_save_string(hostname, "y_hostname_generator");
    lr_user_data_point( lr_eval_string("disk_space_{y_hostname_generator}_free_mebibytes"), free_mebibytes);
    lr_user_data_point( lr_eval_string("disk_space_{y_hostname_generator}_used_mebibytes"), mebibytes_used);

    if( mebibytes_used >= limit_mebibytes_used ) 
    {
        y_setup();
        lr_set_transaction(lr_eval_string("---DISKSPACE USAGE TOO HIGH IN LOG FOLDER FOR {y_hostname_generator}---"), 0, LR_FAIL);
        lr_output_message("Disk space used on host %s in folder %s was %f mebibytes, reaching the limit of %f. Logging turned off for user id %d for the remainder of the test!",
                           hostname, log_folder, mebibytes_used, limit_mebibytes_used, y_virtual_user_id);
        disk_space_warning_given = 1; // turn off further warnings.
        y_log_turn_off_permanently();
    }
}


// --------------------------------------------------------------------------------------------------

#endif // _LOGGING_C
