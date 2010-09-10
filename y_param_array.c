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
#ifndef _LOGGING_C
#define _LOGGING_C

#include "y_loadrunner_utils.c"
/*
Todo: 
- split the extralogging bits from the setup stuff
*/

// Global variables
int _y_extra_logging = 0;                    // client specific logging code on/off switch; 0 = off, 1 = on
int _y_log_level = LR_MSG_CLASS_DISABLE_LOG; // previous loglevel for use with log toggle functions.


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
	if( _vUserID == NULL ) 
	{
		y_setup();
	}

    // Make the extra logging facility available to the user.
    _y_extra_logging = 1;
}

y_log_to_report(char *message)
{
    char *logLine = "%s: VUserId: %d, Host: %s, %s";

    // Only add extra logging if it has been turned on.
    if( _y_extra_logging ) 
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
    //lr_log_message("Log level set to OFF.\n");

    // Save the previous loglevel.
    _y_log_level = lr_get_debug_message();

    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_ON);
}

y_log_set_brief()
{
    //lr_log_message("Log level set to BRIEF.\n");

    // Save the previous loglevel.
    _y_log_level = lr_get_debug_message();

    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_OFF);
    lr_set_debug_message(LR_MSG_CLASS_BRIEF_LOG, LR_SWITCH_ON);
}

y_log_set_extended()
{
    //lr_log_message("Log level set to EXTENDED.\n");

    // Save the previous loglevel.
    _y_log_level = lr_get_debug_message();

    lr_set_debug_message(LR_MSG_CLASS_DISABLE_LOG, LR_SWITCH_OFF);
    lr_set_debug_message(
        LR_MSG_CLASS_EXTENDED_LOG | LR_MSG_CLASS_RESULT_DATA | LR_MSG_CLASS_PARAMETERS | LR_MSG_CLASS_FULL_TRACE,
        LR_SWITCH_ON);
}

// Restore the log level to the old state.
y_log_restore()
{
    /*
    if(_y_log_level == LR_MSG_CLASS_DISABLE_LOG)
    {
        lr_log_message("Warning: Restoring the previous loglevel will turn logging OFF.\n");
        // If the current loglevel is off as well nobody can hear this scream ..
    }
    */
    lr_set_debug_message(_y_log_level, LR_SWITCH_ON);
    //lr_log_message("Log level restored to the previous state.\n");
    // Of course if the previous state was "OFF" the user will never see this either ;-)
}

y_log_turn_on()
{
    if(_y_log_level == LR_MSG_CLASS_DISABLE_LOG)
    {
        log_set_extended();
    }
    else
    {
        log_restore();
    }
}

// --------------------------------------------------------------------------------------------------
// y_write_to_log()
//     writes "content" (a string) to a (log)file.
//     The content will start with current date, time, Vuser-group, VuserId-number and Scenario-ID
//        separated by commas. This function relies on y_write_to_file();
//
// Todo: move this to logging.c
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
    int id, scid;
    char *vuser_group;
    int string_length=0;
    char *log;
    int len_vuser_group;
    int len_scid;
    int result;

    // todo: make this call y_setup().
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


#endif // _LOGGING_C
