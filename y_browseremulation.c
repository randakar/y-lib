/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2012-2014 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

/*! \file y_browseremulation.c
\brief Code for supporting (more) realistic browser emulation.

Basic concept: The tester defines a list of browsers that contains:
1. browser name
2. browser chance - A weight determining the odds that said browser will be chosen. Can be a percentage, or a count from a production access log.
3. max connections - How many connections said browser uses (See: www.browserscope.org for what this value should be for your browser)
4. max connections per host - Same as above, but this setting limits the number of connections to a single hostname.
5. User agent string - A user agent string used by said browser.

This list is read in during vuser_init() to construct a list of browsers, which can then be used during the loadtest to dynamically choose a browser from the list.

\b Usage:
1. Set up the appropriate parameters
2. Add this file to your script, plus any supporting ylib files.
3. Add an include statement: '\#include "y_browseremulation.c' to the top of vuser_init()
4. Call y_setup_browser_emulation() (or one of the alternative variants) in vuser_init()

    \b Example: 
    \code
    vuser_init()
    {
        y_log_turn_off(); // Suppress the logging - the setup code can create a fair amount of (harmless) log messages.
        y_setup_browser_emulation();
        y_log_restore();  // Resume logging.
    }
    \endcode


5. Call y_choose_browser() and y_emulate_browser() at the start of each iteration.

    \b Example:
    \code
    Action()
    {
        // This will set up some connection options for emulating different browser versions in a realistic fashion
        {
            // Note: This will blow up if the setup code hasn't been called beforehand.
            y_browser* browser = y_choose_browser();
            y_emulate_browser(browser);
        }

        // .. more code ..
    }
    \endcode

\author Floris Kraak
*/
#ifndef _Y_BROWSER_EMULATION_C_
//! \cond include_protection
#define _Y_BROWSER_EMULATION_C_
//! \endcond

#include "y_core.c"
#include "y_string.c"


/*! \brief Internal browser structure, describing a browser as an entity.
\see y_browseremulation.c
*/
struct y_struct_browser
{
    //! \brief The name of the browser, as you wish to refer to it.
    char* name;
    //! \brief The odds that said browser will be chosen. This is actually a weight, meaning you could use anything you want, including percentages.
    int chance; 
    //! \brief Pointer to the next element in the single-linked list of browsers
    void* next; 

    //! \brief The maximum number of connections per host to emulate for this browser. \see web_set_sockets_option()
    int max_connections_per_host;
    //! \brief The maximum number of connections to emulate for this browser. \see web_set_sockets_option()
    int max_connections;
    //! \brief The browser's user agent string.
    char* user_agent_string;
};

/*! \brief Internal browser structure, describing a browser as an entity.
\see y_browseremulation.c
*/
typedef struct y_struct_browser y_browser;


//! \brief Limit the browser list to 1000 browsers, as a failsafe in case of wrong parameter settings (which may cause endless loops otherwise)
#define MAX_BROWSER_LIST_LENGTH 1000

//! \brief The first element in the browser list.
y_browser* y_browser_list_head = NULL;

//! \brief The total of all the browser weights added together.
int y_browser_list_chance_total = 0; 

/*! \brief Log the content of a browser object.

Given a pointer to a browser struct, log what said browser struct contains.
This is useful for debugging.

\note Passing in NULL will result in a call to lr_abort().

\param [in] browser A pointer to the browser struct to log.
\author Floris Kraak
*/
void y_log_browser(const y_browser* browser)
{
    if(browser == NULL)
    {
        lr_error_message("y_browser_emulation.c: Attempt to log content of NULL browser. Ignoring.");
        return;
    }

    lr_log_message("y_browseremulation.c: browser: %s, chance %d, max_conn_per_host %d, max_conn %d, user agent string: %s",
                   browser->name,
                   browser->chance,
                   browser->max_connections_per_host,
                   browser->max_connections,
                   browser->user_agent_string);
}

/*! \brief Save the contents of a browser struct as a series of "browser_*" loadrunner parameters.

This will create the following set of parameters from the browser object:
- browser_name
- browser_chance
- browser_max_connections_per_host
- browser_max_connections
- browser_user_agent_string

\param [in] browser A pointer to the browser to create parameters for.

\note y_emulate_browser will use this, so these parameters will contain details about what browser the script is currently emulating after that call.

\note Passing in NULL will result in a call to lr_abort().
\author Floris Kraak
*/
void y_save_browser_to_parameters(const y_browser* browser)
{
    if(browser == NULL)
    {
        lr_error_message("y_browser_emulation.c: Attempt to store the content of NULL browser into parameters. Aborting.");
        lr_abort();
        return;
    }

    lr_save_string(browser->name, "browser_name");
    lr_save_int(browser->chance, "browser_chance");
    lr_save_int(browser->max_connections_per_host, "browser_max_connections_per_host");
    lr_save_int(browser->max_connections, "browser_max_connections");
    lr_save_string(browser->user_agent_string, "browser_user_agent_string");
}


/*! \brief Initialize the browser list based on a set of prepared parameters.

This will initialize the single-linked list of browsers, using the given set of parameters.
The 'browser_name' parameter named here is expected to be set to "Select Next Row: Sequential", and "Update value on: Each iteration".
The -last- browser name in the list has to contain the value "END" to mark the end of the list.
All other parameters should be set to "Select Next Row: Same line as {browser_name_param}".

Call this during vuser_init().

\param [in] browser_name_param Browser name. Set this parameter to "Select Next Row: Sequential", and "Update value on: Each iteration". The last browser name should be "END".

\param [in] browser_chance_param Browser weight. Can be any number as long as the total of chances does not exceed Y_RAND_MAX. Set this parameter to "Select Next Row: Same line as {browser_name_param}".

\param [in] browser_max_connections_per_host_param Maximum number of connections this browser allows per host. See www.browserscope.org for possible values. Set this parameter to "Select Next Row: Same line as {browser_name_param}".

\param [in] browser_max_connections_param Maximum number of connections this browser allows. See www.browserscope.org for possible values. Set this parameter to "Select Next Row: Same line as {browser_name_param}".

\param [in] browser_user_agent_string_param The user agent string this browser reports to the server. See your production HTTP access logs for examples. Set this parameter to "Select Next Row: Same line as {browser_name_param}".

\returns 0 if successful, -1 if errors occurred during setup.

\b Example: 
\code
vuser_init()
{
    y_log_turn_off(); // Suppress the logging - the setup code can create a fair amount of (harmless) log messages.
    y_setup_browser_emulation_from_parameters("browser_name", "browser_chance", "browser_max_connections_per_host", "browser_max_connections", "browser_user_agent_string");
    y_log_restore();  // Resume logging.
}
\endcode

\see y_setup_browser_emulation_from_file()
\see y_browser_list_head
\see y_browser_list_chance_total
\see y_browseremulation.c

\note You may wish to temporarily disable logging here using y_log_turn_off() and y_log_restore().
\author Floris Kraak
*/
int y_setup_browser_emulation_from_parameters(const char* browser_name_param,
                                              const char* browser_chance_param,
                                              const char* browser_max_connections_per_host_param,
                                              const char* browser_max_connections_param,
                                              const char* browser_user_agent_string_param)
{
    int i;
    y_browser* previous_browser = NULL;

    for(i=0; i < MAX_BROWSER_LIST_LENGTH; i++ )
    {
        y_browser* browser;
        char* browser_name = y_get_parameter_with_malloc_or_null(browser_name_param);

        if( browser_name == NULL )
        {
            lr_error_message("Browser name parameter %s does not exist. Aborting browser emulation setup.", browser_name_param);
            return -1;
        }
        if(strcmp(browser_name, "END") == 0)
        {
            free(browser_name);
            lr_log_message("y_browseremulation.c: End of browser list initialisation");
            return 0;
        }

        // Allocate a browser object
        browser = (y_browser*) y_mem_alloc( sizeof browser[0] );
        browser->name = browser_name;
        browser->next = NULL;

        // The next bit is a bit verbose due to all of the checks on missing parameters,
        // but really we're just filling out the remaining fields of the browser struct.
        {
            int error = 0;
            char* browser_chance                   = y_get_parameter_or_null(browser_chance_param);
            char* browser_max_connections_per_host = y_get_parameter_or_null(browser_max_connections_per_host_param);
            char* browser_max_connections          = y_get_parameter_or_null(browser_max_connections_param);

            if( browser_chance == NULL || browser_max_connections_per_host == NULL || browser_max_connections == NULL )
            {
                lr_error_message("Browser parameter missing. Aborting browser emulation setup. chance: %s:%s, max_connections_per_host %s:%s, max_connections %s:%s", 
                                 browser_chance_param, browser_chance,
                                 browser_max_connections_per_host_param, browser_max_connections_per_host,
                                 browser_max_connections_param, browser_max_connections);
                error = 1;
            }
            else if( (browser->user_agent_string = y_get_parameter_with_malloc_or_null(browser_user_agent_string_param)) == NULL )
            {
                lr_error_message("Browser user agent parameter %s does not exist. Aborting browser emulation setup.", browser_user_agent_string_param);
                error = 1;
            }

            if( error )
            {
                free(browser_name);
                free(browser);
                return -1;
            }
            browser->chance                   = atoi(browser_chance);
            browser->max_connections_per_host = atoi(browser_max_connections_per_host);
            browser->max_connections          = atoi(browser_max_connections);
        }

        //lr_log_message("y_browseremulation.c: Adding browser");
        //y_log_browser(browser);

        // Increment the global count of weights.
        y_browser_list_chance_total += browser->chance;
        //lr_log_message("y_browseremulation.c: Adding weight: %d", y_browser_list_chance_total);

        // Add it to the list.
        if( y_browser_list_head == NULL )
        {
            y_browser_list_head = browser;
        }
        else
        {
            previous_browser->next = browser;
        }
        previous_browser = browser; // Replaces the previous tail element with the new one.

        // Get the next value for the new iteration.
        // This parameter should be set to "update each iteration", or this code will play havoc with it ..
        lr_advance_param((char* )browser_name_param);
    }

    if( i == (MAX_BROWSER_LIST_LENGTH) )
    {
        lr_log_message("Too many browsers to fit in browser list struct, max list size = %d", MAX_BROWSER_LIST_LENGTH);
        lr_abort();
        return -1;
    }
    return 0; // No error.
}

/*! \brief Initialize the browser list, using the default values for the parameter names:
- browser_name_param = browser_name
- browser_chance_param = browser_chance
- browser_max_connections_per_host_param = browser_max_connections_per_host
- browser_max_connections_param = browser_max_connections
- browser_user_agent_string_param = browser_user_agent_string

\b Example: 
\code
vuser_init()
{
    y_log_turn_off(); // Suppress the logging - the setup code can create a fair amount of (harmless) log messages.
    y_setup_browser_emulation_from_parameters("browser_name", "browser_chance", "browser_max_connections_per_host", "browser_max_connections", "browser_user_agent_string");
    y_log_restore();  // Resume logging.
}
\endcode

\note We may remove this in favor of a call to y_setup_browser_emulation_from_file() at some point.

\see y_setup_browser_emulation_from_parameters(), y_setup_browser_emulation_from_file(), y_browseremulation.c
\author Floris Kraak
*/
int y_setup_browser_emulation()
{
    return y_setup_browser_emulation_from_parameters("browser_name", "browser_chance", "browser_max_connections_per_host", "browser_max_connections", "browser_user_agent_string");
}

/*! \brief Initialize the browser list based on a tab-seperated CSV file.

This will initialize the single-linked list of browsers, using a text file containing on each line one browser with the following fields, seperated by tabs:
1. browser name
2. chance (weight)
3. max_connections
4. max_connections_per_host
5. user_agent

The order of the fields is important; They should be listed on each line in the above order.

\note The file should not contain headers of any kind, nor the string "END" as y_setup_browser_emulation_from_parameters() requires.

\b Example: 
\code
vuser_init()
{
    y_log_turn_off(); // Suppress the logging - the setup code can create a fair amount of (harmless) log messages.
    y_setup_browser_emulation_from_file("browser.dat");
    y_log_restore();  // Resume logging.
}
\endcode

\note Empty lines and comments (text starting with "#") will be ignored.

Call this during vuser_init().

\param [in] filename The name of the file to read.
\returns 0 if setup was successful, -1 in case of an error.

\see y_setup_browser_emulation_from_parameters()
\see y_browser_list_head
\see y_browser_list_chance_total
\see y_browseremulation.c

\note You may wish to temporarily disable logging for this call using y_log_turn_off() and y_log_restore().
\author Floris Kraak
*/
int y_setup_browser_emulation_from_file(char* filename)
{
    y_browser* previous_browser = NULL;
    long fp = fopen(filename, "r");
    char line[4096];

    if (fp == NULL)
    {
        lr_error_message("Unable to open file %s", filename);
        lr_abort();
        return -1;
    }
    lr_log_message("Opened file %s", filename);

    // Overflow protection
    line[0] = '\0';
    line[4095] = '\0';

    // Now read the file line by line.
    while(fgets(line, sizeof line, fp))
    {
        char* remove;
        lr_log_message("Read line: %s", line);

        // Remove comments and trailing newlines.
        while( ((remove = strchr(line, '#'))  != NULL) ||
               //((remove = strstr(line, "//")) != NULL) ||
               ((remove = strchr(line, '\r')) != NULL) || 
               ((remove = strchr(line, '\n')) != NULL) )
        {
            remove[0] = '\0';
            //lr_log_message("Stripped line: %s", line);
        }


        { // start namespace for various temporary stack buffers

            char name[4096];
            size_t user_agent_offset;

            y_browser* browser = (y_browser*) y_mem_alloc( sizeof browser[0] );
            int scanresult = sscanf(line, "%4095s	%*f%%	%d	%d	%d	%n",
                name, &browser->chance, &browser->max_connections, &browser->max_connections_per_host, &user_agent_offset);

            // Debug code
//             lr_log_message("Found %d matches", scanresult);
//             switch(scanresult)
//             {
//                 case 5:
//                     lr_log_message("Found at least one too many ..");
//                 case 4:
//                     lr_log_message("user_agent_offset: %d", user_agent_offset); // %n telt niet mee  blijkbaar.
//                     lr_log_message("browser->max_connections_per_host: %d", browser->max_connections_per_host);
//                 case 3:
//                     lr_log_message("browser->max_connections: %d", browser->max_connections);
//                 case 2:
//                     lr_log_message("browser->chance: %d", browser->chance);
//                 case 1:
//                     lr_log_message("name: %s", name);
//             }
            if(scanresult < 4)
            {
                free(browser);
                lr_log_message("Non-matching line.");
                continue;
            }

            // Copy the char* fields into their own allocated memory and store that.
            browser->name = y_strdup(name);
            browser->user_agent_string = y_strdup(line + user_agent_offset);

            // Report the result
            //y_log_browser(browser);

            // Increment the global count of weights.
            y_browser_list_chance_total += browser->chance;
            lr_log_message("y_browseremulation.c: Adding weight: %d", y_browser_list_chance_total);

            // Add it to the list.
            if( y_browser_list_head == NULL )
                y_browser_list_head = browser;
            else
                previous_browser->next = browser;
            previous_browser = browser; // Replaces the previous tail element with the new one.
        } // end namespace
    } // end while loop

    return 0;
}


/*! \brief Choose a browser profile from a browser list at random using the defined weights in that list.

\param [in] browser_list_head The first entry of a previously constructed single-linked browser list.
\param [in] browser_list_chance_total The total of the browser weights in the list.
\returns a pointer to a randomly chosen struct browser, or NULL in case of errors.

\b Example:

\code
// This will set up some connection options for emulating different browser versions in a realistic fashion
{
    // Note: This will blow up if the setup code hasn't been called beforehand.
    y_browser* browser = y_choose_browser_from_list(y_browser_list_head, y_browser_list_chance_total);
    y_emulate_browser(browser);
}
\endcode

\note In most cases a call to the y_choose_browser() wrapper should suffice, instead.

\see y_choose_browser(), y_setup_browser_emulation_from_file(), y_setup_browser_emulation_from_parameters(), y_browseremulation.c, y_browser_list_head
\author Floris Kraak
*/
y_browser* y_choose_browser_from_list(y_browser* browser_list_head, int browser_list_chance_total)
{
    int i, lowerbound, cursor = 0;
    y_browser* browser = NULL;
    long roll;

    if( browser_list_chance_total < 1 )
    {
        lr_error_message("y_browseremulation.c: Browser list not initialised before call to y_choose_browser_from_list(). Cannot choose, ignoring.");
        return NULL;
    }

    // The upper bound of the rand() function is determined by the RAND_MAX constant.
    // RAND_MAX is hardcoded in loadrunner to a value of exactly 32767.
    // In fact, it doesn't even get #defined by loadrunner as as the C standard mandates.
    // (y_core.c does that instead, now  ..)
    // 
    // y_rand() depends on rand() for it's output, so that cannot go above 32767.
    // Update: y_rand() uses Y_RAND_MAX now - slightly over 1 billion.
    // Unfortunately, the list with weights that we use has numbers bigger than that. 
    // 
    // So we'll need to get a bit creative, and multiply the result of y_rand() with
    // the maximum (the total of the browser chances) divided by Y_RAND_MAX to scale things up again.
    // (At the cost of precision ..)
    // 
    // Update: y_rand() now returns a 31 bit number, giving it an upper bound of 2147483647 (Y_RAND_MAX).
    // That should reduce the need for this code by a bit ..
    // 

    roll = y_rand() % browser_list_chance_total;
    //lr_log_message("browser_list_chance_total = %d, RAND_MAX = %d, roll %d", browser_list_chance_total, RAND_MAX, roll);
    if( Y_RAND_MAX < browser_list_chance_total)
    {
        roll = roll * ((browser_list_chance_total / RAND_MAX));
    }
    //lr_log_message("Roll: %d", roll);

    for( browser = browser_list_head; browser != NULL; browser = browser->next)
    {
        cursor += browser->chance;
        // lr_log_message("Chance cursor: %d for browser: %s", cursor, browser->name);

        if(roll <= cursor)
        {
            //lr_log_message("Chosen browser:");
            //y_log_browser(browser);
            return browser;
        }
    }
    // This code should be unreachable.
    lr_error_message("y_browseremulation.c: Roll result out of bounds: roll: %d, cursor: %d, browser_list_chance_total %d", roll, cursor, browser_list_chance_total);
    return browser;
}

/*! \brief Choose a browser profile from the browser list at random using the defined weights.

This wrapper around y_choose_browser_from_list() uses the list pointed to by y_browser_list_head to choose from.

\returns a pointer to a randomly chosen struct browser, or NULL in case of errors.

\b Example:

\code
// This will set up some connection options for emulating different browser versions in a realistic fashion
{
    // Note: This will blow up if the setup code hasn't been called beforehand.
    y_browser* browser = y_choose_browser();
    y_emulate_browser(browser);
}
\endcode

\see y_choose_browser_from_list(), y_setup_browser_emulation_from_file(), y_setup_browser_emulation_from_parameters(), y_browseremulation.c, y_browser_list_head
\author Floris Kraak
*/
y_browser* y_choose_browser()
{
    return y_choose_browser_from_list(y_browser_list_head, y_browser_list_chance_total);
}

/*! \brief Emulate a specific browser.

This will set up the MAX_TOTAL_CONNECTIONS, MAX_CONNECTIONS_PER_HOST socket options and user_agent_string settings for the browser listed.

\b Example:
\code
// This will set up some connection options for emulating different browser versions in a realistic fashion
{
    // Note: This will blow up if the setup code hasn't been called beforehand.
    y_browser* browser = y_choose_browser();
    y_emulate_browser(browser);
}
\endcode

\see y_choose_browser, y_choose_browser_from_list, y_browser_emulation.c, web_set_sockets_option(), web_add_auto_header()

\param [in] new_browser Pointer to a browser struct for the browser to be emulated.
\returns 0 if no errors occurred, -1 in case of an error.
\author Floris Kraak
*/
int y_emulate_browser(y_browser* new_browser)
{
    char str_max_connections[12];
    char str_max_connections_per_host[12];
    int max_connections;
    static y_browser* previous_browser;
    y_browser* browser;

    if( new_browser == NULL )
    {
    	if( previous_browser == NULL )
    	{
	        lr_error_message("y_browser_emulation.c: Attempt to emulate the NULL browser: Ignored.");
	        return -1;
    	}
    	else
    	{
    		browser = previous_browser;
    	}
    }
    else
    {
    	browser = new_browser;
    	previous_browser = browser;
    }

    lr_log_message("Emulating browser:");
    y_log_browser(browser);

    // Loadrunner doesn't accept values higher than 50 for this sockets option, so we'll just log it and set it to 50.
    max_connections = browser->max_connections;
    if( max_connections > 50 )
    {
        lr_log_message("y_browser_emulation.c: Loadrunner does not support using more than 50 browser connections. Using 50 connections instead of %d.", max_connections);
        max_connections = 50;
    }

    snprintf(str_max_connections,          sizeof str_max_connections,          "%d", max_connections);
    snprintf(str_max_connections_per_host, sizeof str_max_connections_per_host, "%d", browser->max_connections_per_host);

    // Now finally set the correct settings for the chosen browser:
    web_set_sockets_option("MAX_CONNECTIONS_PER_HOST", str_max_connections_per_host);
    web_set_sockets_option("MAX_TOTAL_CONNECTIONS",    str_max_connections);
    web_add_auto_header("User-Agent", browser->user_agent_string);
    return 0;
}


// --------------------------------------------------------------------------------------------------
#endif // _Y_BROWSER_EMULATION_C_
