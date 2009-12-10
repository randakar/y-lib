/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2009 Floris Kraak
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

// FIXME: Make this particular dll unneccesary by reimplementing getDateTimeStamp() in C.
#ifndef VUSERFUNC
    #define VUSERFUNC    "vuserfunctions.dll"    // dynamic link library Vuser functions
#endif

// Global variables
char* _injectorHost;         // injector name
int _vUserID;                // virtual user id
int _extraLogging = 0; // Client specific logging code on/off switch; 0 = off, 1 = on
int _logLevel = LR_MSG_CLASS_DISABLE_LOG; // previous loglevel for use with log toggle functions.


y_setup_logging()
{
    /*
    // Only add extra logging if it has been turned on.
    if( !_extraLogging )
    {
        return;
    }
    */
    // Turn extra logging on if setup_logging() gets called.
    _extraLogging = 1;

    // Global variables, handle with care
    lr_whoami(&_vUserID, NULL, NULL);
    _injectorHost = lr_get_host_name(); 

    // Load general vuser functions
    // These 'general vuser functions' are only used for logging, though ;-)
    // (not to mention specific to one client!)
    // FIXME: Make this particular dll unneccesary by reimplementing getDateTimeStamp() in C.
    {
        int result;
        if( (result = lr_load_dll(VUSERFUNC)) != 0)
        {
            char *logLine = "%s: VUserId: %d, host: %s, rc: %d, %s";
            lr_log_message(logLine, "N/A", _vUserID, _injectorHost, result, "could not load vuser functions");
            lr_exit(LR_EXIT_VUSER, LR_FAIL);
        }
    }
}


y_log_to_report(char *message)
{
    char *logLine = "%s: VUserId: %d, Host: %s, %s";

    // Only add extra logging if it has been turned on.
    if( _extraLogging ) 
    {
        lr_log_message(logLine, getDateTimeStamp(), _vUserID, _injectorHost, lr_eval_string(message));
    }
}

y_log_error(char *message)
{
    char *msg = lr_eval_string(message);
    log_to_report(msg);
    lr_error_message(msg);
    lr_fail_trans_with_error(msg);
}

y_log_warning(char *message)
{
    char *msg;
    lr_save_string(lr_eval_string(message), "_log_msg");
    msg = lr_eval_string("Warning: {_log_msg}");

    log_to_report(msg);
}

// Save the current loglevel and turn off logging.
y_log_turn_off()
{
    lr_log_message("Log level set to OFF.\n");

    // Save the previous loglevel.
    _logLevel = lr_get_debug_message();

    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_ON);
}

y_log_set_brief()
{
    lr_log_message("Log level set to BRIEF.\n");

    // Save the previous loglevel.
    _logLevel = lr_get_debug_message();

    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_OFF);
    lr_set_debug_message(LR_MSG_CLASS_BRIEF_LOG, LR_SWITCH_ON);
}

y_log_set_extended()
{
    lr_log_message("Log level set to EXTENDED.\n");

    // Save the previous loglevel.
    _logLevel = lr_get_debug_message();

    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_OFF);
    lr_set_debug_message(
        LR_MSG_CLASS_EXTENDED_LOG | LR_MSG_CLASS_RESULT_DATA | LR_MSG_CLASS_PARAMETERS | LR_MSG_CLASS_FULL_TRACE,
        LR_SWITCH_ON);
}

// Restore the log level to the old state.
y_log_restore()
{
    /*
    if(_logLevel == LR_MSG_CLASS_DISABLE_LOG)
    {
        lr_log_message("Warning: Restoring the previous loglevel will turn logging OFF.\n");
        // If the current loglevel is off as well nobody can hear this scream ..
    }
    */
    lr_set_debug_message(_logLevel, LR_SWITCH_ON);
    lr_log_message("Log level restored to the previous state.\n");
    // Of course if the previous state was "OFF" the user will never see this either ;-)

}

y_log_turn_on()
{
    if(_logLevel == LR_MSG_CLASS_DISABLE_LOG)
    {
        log_set_extended();
    }
    else
    {
        log_restore();
    }

}


#endif // _LOGGING_C
