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

/*
Todo: 
- split the extralogging bits from the setup stuff
- Seed the random number generator using _vuserId and _vUserGroup
- y_ify these variables (below)
*/

// Global variables
// FIXME: y_ ify these ..
int _vUserID;                             // virtual user id
int _extraLogging = 0;                    // client specific logging code on/off switch; 0 = off, 1 = on
char *_vUserGroup = NULL;                 // virtual user group
int _logLevel = LR_MSG_CLASS_DISABLE_LOG; // previous loglevel for use with log toggle functions.


// --------------------------------------------------------------------------------------------------
//// Time/date/stamp functions

typedef long time_t;

struct tm { 
    int tm_sec;   // seconds after the minute - [0,59] 
    int tm_min;   // minutes after the hour - [0,59] 
    int tm_hour;  // hours since midnight - [0,23] 
    int tm_mday;  // day of the month - [1,31] 
    int tm_mon;   // months since January - [0,11] 
    int tm_year;  // years since 1900 
    int tm_wday;  // days since Sunday - [0,6] 
    int tm_yday;  // days since January 1 - [0,365] 
    int tm_isdst; // daylight savings time flag 
#ifdef LINUX 
    int tm_gmtoff; 
    const char * tm_zone; 
#endif 
};

struct timeb {
    time_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
};


/*
 * getDateTimeStamp
 * Returns the current date and time represented as YYYY-MM-DD HH:MM:SS.mmm.
 */
char *y_get_datetimestamp()
{
	struct timeb timebuffer;
	struct tm *nu;
	static char YMDHMSm[24]; // moet static char zijn om te gebruiken als returnwaarde

	_tzset();
	ftime( &timebuffer );
	nu = (struct tm *)localtime( & (timebuffer.time) );
	
	sprintf(YMDHMSm, "%04u-%02u-%02u %02u:%02u:%02u.%03u", 
       nu->tm_year + 1900,
			nu->tm_mon + 1,
			nu->tm_mday,
			nu->tm_hour,
			nu->tm_min,
			nu->tm_sec,
			timebuffer.millitm);
	return YMDHMSm;
}

//
// Hmn, do we even still need this?
// 
time_t y_timestamp()
{
    return time(NULL);
}
// --------------------------------------------------------------------------------------------------


y_setup_logging()
{
    // Global variables, handle with care
    lr_whoami(&_vUserID, &_vUserGroup, NULL);

    // Make the extra logging facility available to the user.
    _extraLogging = 1;
}

y_log_to_report(char *message)
{
    char *logLine = "%s: VUserId: %d, Host: %s, %s";

    // Only add extra logging if it has been turned on.
    if( _extraLogging ) 
    {
        lr_log_message(logLine, y_get_datetimestamp(), _vUserID, lr_get_host_name(), lr_eval_string(message));
    }
}

y_log_error(char *message)
{
    char *msg = lr_eval_string(message);
    y_log_to_report(msg);
    //lr_error_message(msg); 
    lr_fail_trans_with_error(msg);
}

y_log_warning(char *message)
{
    char *msg;
    lr_save_string(lr_eval_string(message), "_log_msg");
    msg = lr_eval_string("Warning: {_log_msg}");

    y_log_to_report(msg);
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
