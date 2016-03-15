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

/*! \file y_logging.c
\brief Logging-related y-lib functions

Note that some of these are a bit marginal - the disk space guards for instance are logging related in the sense that they will turn all logging off when too much disk space has been used,
but they aren't exactly log manipulation functions in the low-level sense.
*/
#ifndef _Y_LOGGING_C_
//! \cond include_protection
#define _Y_LOGGING_C_
//! \endcond

//! \cond function_removal
#define y_timestamp 0_y_timestamp_no_longer_exists_please_use_y_get_current_time
#define y_log_error 0_y_log_error_no_longer_exists_please_use_lr_error_message
#define y_log_warning 0_y_log_warning_no_longer_exists_please_use_lr_error_message
#define y_get_datetimestamp 0_y_get_datetimestamp_no_longer_exists_please_use_lr_save_datetime
#define y_setup_logging 0_y_setup_logging_no_longer_exists_please_use_transaction_triggers
#define y_log_to_report 0_y_log_to_report_no_longer_exists_please_use_transaction_triggers
#define y_write_to_log 0_y_write_to_log_no_longer_exists_sorry
//! \endcond

#include "y_core.c"
#include "y_loadrunner_utils.c"

// Previous loglevel, saved with and restored by log toggle functions.
// Not documented because people really should not be using this directly.
//
//! \cond internal_global
unsigned int _y_log_level = LR_MSG_CLASS_DISABLE_LOG; 
//! \endcond

/*! \brief Convert a unixtime style timestamp to a date and time represented as YYYY-MM-DD HH:MM:SS.mmm.
 * 
 * \param [in] time - the unix time stamp, as reported by ftime()
 * \param millitm - the milliseconds belonging to the time stamp
 *
 * \return The string represation of the timestamp, formatted as YYYY-MM-DD HH:MM:SS.mmm.
 *
 * \see y_get_datetimestamp(), y_get_current_time()
 *
 * \deprecated lr_save_datetime() really should be used for this kind of thing.
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

/*! \brief Save the loglevel for later restoration through y_log_restore().
 *
 * This is called by the various y_log_* functions to make sure the loglevel can be put back the way it was.
 *
 *\author Floris Kraak
 */
void y_log_save()
{
    // Save the previous loglevel.
    _y_log_level = lr_get_debug_message();
    //lr_error_message("Saved loglevel %d", _y_log_level);
}

/*! \brief Force all logging off unconditionally, without saving the old state.
 *
 * This is used by various y_log_* functions to get the loglevel to a known state.
 * 
 * \author Floris Kraak
 */
void y_log_turn_off_without_saving()
{
    // Good grief, this debug message interface is terrible.

    // Turn all logging off. Which has the really weird side effect of turning auto logging on again.
    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_ON);

    // Disable the auto logging again, but using a different bit flag.
    lr_set_debug_message(LR_MSG_CLASS_AUTO_LOG, LR_SWITCH_OFF);
    //lr_error_message("All logging turned off, log level now %d", lr_get_debug_message() );
}


/*! \brief Save the current loglevel and turn off logging.
 * \note Calling y_log_restore() will restore the loglevel to the state it was in before this call. 
 *
 *  \author Floris Kraak
 */
void y_log_turn_off()
{
    y_log_save();
    y_log_turn_off_without_saving();
}

/*! \brief Turn all logging off and make sure it stays that way.
 *
 * This will overwrite the state saved by y_log_save() with "No logging at all, please.", so that y_log_restore() can't turn things back on.
 *
 * \note Direct calls to lr_set_debug_message() will still be able to turn things back on. 
 * \warning Mixing calls to y_log_* functions with calls to lr_debug_message() is not wise and may yield unpredictable results.
 *
 * \author Floris Kraak
 */
void y_log_turn_off_permanently()
{
    y_log_turn_off_without_saving();
    y_log_save(); // make sure it is never accidentally enabled again through y_log_restore()
}

/*! \brief Set the log level to "brief" messages only.
 * 
 * \note Calling y_log_restore() will restore the loglevel to the state it was in before this call. 
 *
 * Useful when you want to, say, not see that big PDF you are downloading somewhere in your logs but *do* want to see the rest of the logging.
 * \author Floris Kraak
 */
void y_log_set_brief()
{
    //lr_log_message("Log level set to BRIEF.\n");
    y_log_turn_off();
    lr_set_debug_message(LR_MSG_CLASS_BRIEF_LOG, LR_SWITCH_ON);
    //lr_log_message("Log level set to brief, from %d to %d", _y_log_level, lr_get_debug_message() );
}

/*! \brief Set the log level to show all messages, including extended result data, parameters, and tracing information.
 * \note Calling y_log_restore() will restore the loglevel to the state it was in before this call. 
 *
 * Useful when you want to debug a particular step in the script.
 *
 * \author Floris Kraak
 */
void y_log_set_extended()
{
    //lr_log_message("Log level set to EXTENDED.\n");
    y_log_turn_off();
    lr_set_debug_message(
        LR_MSG_CLASS_EXTENDED_LOG | LR_MSG_CLASS_RESULT_DATA | LR_MSG_CLASS_PARAMETERS | LR_MSG_CLASS_FULL_TRACE,
        LR_SWITCH_ON);
    //lr_log_message("Log level extended from %d to %d", _y_log_level, lr_get_debug_message() );
}


/*! \brief Restore the loglevel to previous state.
 * To be exact, it will restore it to the state it was in before the last call to y_log_set_{off|on|brief|extended}() or y_log_save().
 *
 * \note Multiple calls to y_log_set_* functions in a row will overwrite the previous saved state, making the first state potentially unrecoverable.
 *
 * \author Floris Kraak
 */
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

/*! \brief Turn logging on.
 *
 * What this function does depends on the saved loglevel.
 * - If the saved loglevel was "no logging", y_log_set_extended() is called, turning all logging on.
 * - Otherwise, y_log_restore() is called to undo the previous loglevel change.
 *
 * \note If the previous loglevel was "brief" while the current level is "extended", this may result in *less* logging.
 * \todo Either fix that, or remove this function.
 *
 * \author Floris Kraak
 */
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

/*! \brief Log a message forcefully, bypassing the current log settings.

Typically used for generating datafiles within scripts with the loglevel set to OFF, or things of that sort.

\b Example:
\code
   lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_ON);
   lr_set_debug_message(LR_MSG_CLASS_AUTO_LOG, LR_SWITCH_OFF);
   lr_log_message("Before");              // Nothing gets logged here.
   y_log_force_message("Forced message"); // "Forced message" appears in the logs.
   lr_log_message("After");               // No output again.
\endcode
\author Floris Kraak
*/ 
void y_log_force_message(char *message)
{
    y_log_set_extended();
    lr_log_message( message );
    y_log_restore();
}


/*! \brief Detect low disk space situations on the generator and turn all logging off if not enough space is left.

When called, this function will check the amount of space left on the generator's "output" device / directory. 
If the percentage of free space is lower than the treshold percentage, it will generate an error transaction "---DISK SPACE LOW IN LOG FOLDER FOR {y_hostname_generator}---" and turn all further logging off until the end of the test, using y_log_turn_off_permanently().

\note Scripts that call lr_debug_message() or the various y_lib() toggle functions to turn the loglevel back up may ignore this restriction.

Loadrunner does not protect the generators' disks from overflowing, even if the test is writing out a lot of information to the logs really quickly.
If a disk fills up on the generator or the disk the controller uses for results collation overflows there is no graceful recovery. Collation will fail and test results may be hard or impossible to recover.
This kind of situation can occur when temporary service disruptions happen (triggering a flood of 'log on error' messages), but also if runtime settings are incorrect and the test was configured to log *everything*.

In order to prevent this from happening scripts should regularly call to both y_disk_space_guard() and y_disk_space_usage_guard() with some reasonable limits.

\param [in] max_free_percentage The amount of free diskspace as a percentage of total space that will cause the logging to turn off.

\author Floris Kraak
*/
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


/*! \brief Detect excessive disk usage by the test and turn all further logging off if more than a specific limit number of mebibytes has been used.

When called, this function will check the amount of space used on the generator's "output" device / directory since the first call to this function.
If the amount of used space exceeds the treshold it will generate an error transaction "---DISK SPACE LOW IN LOG FOLDER FOR {y_hostname_generator}---" and turn all further logging off until the end of the test, using y_log_turn_off_permanently().

\note Scripts that call lr_debug_message() or the various y_lib() toggle functions to turn the loglevel back up may ignore this restriction.

Loadrunner does not protect the generators' disks from overflowing, even if the test is writing out a lot of information to the logs really quickly.
If a disk fills up on the generator or the disk the controller uses for results collation overflows there is no graceful recovery. Collation will fail and test results may be hard or impossible to recover.
This kind of situation can occur when temporary service disruptions happen (triggering a flood of 'log on error' messages), but also if runtime settings are incorrect and the test was configured to log *everything*.

\note Because of the way this measurement is done, it may also trigger if some other process on the same machine starts writing large amounts of data to the same device.

In order to prevent this from happening scripts should regularly call to both y_disk_space_guard() and y_disk_space_usage_guard() with some reasonable limits.

\note The contents of the generator's output folder will be transferred to the controller during collation. If the sum of the output directories for the generators exceed the size of that filesystem collation will fail, even if there is enough space on the generators. Therefore, set this low enough to make sure that if all generators output folders get filled to the limit the controller will still have enough space for it's collation process.

\param [in] limit_mebibytes_used The amount of mebibytes the test is allowed to used on the generator's target directory. 

\author Floris Kraak
*/
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

#endif // _Y_LOGGING_C_
