/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2014 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

/*
 * Documentation generated from this source code can be found here: http://randakar.github.io/y-lib/
 * Main git repitory can be found at https://github.com/randakar/y-lib
 */

 
#ifndef _Y_TRANSACTION_C_
//! \cond include_protection
#define _Y_TRANSACTION_C_
//! \endcond


/*! 
\file y_transaction.c
\brief ylib transaction library.

Allows you to extend your script with the following features:
+ Automatic transaction naming and numbering.
+ Transaction triggers: Code to be run when a transaction starts or stops.
+ Custom transaction implementations - Adding your own logging or time measurements to transactions.

Usage
-----
Include this file (or y_lib.c) in your script, call y_start_transaction() and y_end_transaction() everywhere you normally call the loadrunner variants.
Use search-and-replace to convert existing scripts. Add calls to y_start_transaction_block() and y_end_transaction_block() to set a transaction prefix.

Transaction naming
------------------
Y-lib transaction names take the form: '{transaction_prefix}_{transaction_nr}_{step_name}'.
Sub transaction names take the form:   '{transaction_prefix}_{transaction_nr}_{sub_transaction_nr}_{step_name}'.
Calling y_start_transaction_block() starts a block of transactions that all use the name of the block as the transaction prefix.

Optionally the vuser group name can be added to the transaction names as well. This allows differentiation between different types of users that are all running the same script otherwise.

Triggers
--------
A trigger is a function that will execute every time an ylib transaction starts or stops. You can define seperate triggers for transaction start, stop, and the sub transaction start and stop.
The return value of the trigger is used to determine whether the transaction should pass or fail, in the case of end transaction triggers.

Custom transaction implementations
----------------------------------
Ylib can be configured to run your own custom transaction start/stop implementation instead of the usual loadrunner functions whenever a transaction starts or stops.
This can be used for a variety of things; For example, calculating the 99th percentile responstime for a specific group of transactions can be done by starting a seperate transaction on top of the normal one for the transactions in question. A different example would be having all transactions log a timestamp with the transaction name whenever a transaction starts or stops.

Testcase
--------

\code
    y_setup_logging();                   // Initialisation. May be omitted. (testcase?)

    y_start_sub_transaction("alpha");    // starts trans '01 alpha' and subtrans '01_01 alpha'.
    y_end_sub_transaction("", LR_AUTO);  // ends both.

    y_start_action_block("one");         // starts action block "one".
    y_start_sub_transaction("beta");     // starts trans 'one_01 beta' and subtrans 'one_01_01 beta'.
    y_end_sub_transaction("", LR_AUTO);  // ends both.
    y_end_action_block();                // ends action block "one".

    y_set_add_group_to_transaction(1);   // start adding the groupname - 'None', when running inside vugen.

    y_start_sub_transaction("gamma");    // starts trans 'None_02 gamma' and subtrans 'None_02_01 gamma'.
    y_end_sub_transaction("", LR_AUTO);  // ends both.

    y_start_action_block("two");         // starts action block "two".
    y_start_sub_transaction("delta");    // starts trans 'None_two_01 delta' and subtrans 'None_two_01_01 delta'.
    y_end_sub_transaction("", LR_AUTO);  // ends both.
    y_end_action_block();                // ends action block "two".
    lr_abort();                          // end testcase ;-)
    return;
\endcode
*/

//! \cond function_removal
#define y_setup_step_waterfall 0_y_setup_step_waterfall_no_longer_exists_please_use_flow_lists
#define y_waterfall_random_weighted_continue 0_y_waterfall_random_weighted_continue_please_use_flow_lists
//! \endcond

// Needed to compile this - the definition of LAST is missing if it's not included.
#include "web_api.h"

// More C definitions
#include "vugen.h"

// Other Ylib functions
#include "y_logging.c"
#include "y_string.c"
#include "y_loadrunner_utils.c"


// Variables //


// Never access these variables directly - names may change. 
// Use the get() and set() functions instead, if available. (otherwise, add them?)
//! INTERNAL: Whether to add the name of the vuser group to the transaction names. 1 = on, 0 = off.
int _y_add_group_to_trans = 0;      // 
//! INTERNAL: Whether to create a graph detailing wasted time. Debugging option.
int _y_wasted_time_graph = 0;       

//! INTERNAL: Transaction counting for transaction blocks: \see y_start_transaction_block() and y_start_transaction()
int _y_transaction_nr = 1;
//! INTERNAL: Transaction counting for sub transactions: \see y_start_sub_transaction()
int _y_sub_transaction_nr = 1;

//! Transaction counting support for sessions. \see y_session_timer_start() and y_session_timer_end()
int y_session_transaction_count = -1;


//! Transaction status tracking
#define Y_TRANS_STATUS_NONE         0
//! Transaction status tracking
#define Y_TRANS_STATUS_STARTED      1
//! Transaction status tracking
#define Y_TRANS_STATUS_AUTO_STARTED 2
//! INTERNAL: Transaction status tracking
int _y_trans_status = Y_TRANS_STATUS_NONE;

//! Transaction trigger support typedef
typedef int (y_trigger_func)();
//! \see y_set_transaction_start_trigger()
y_trigger_func* _y_trigger_start_trans = NULL;
//! \see y_set_transaction_end_trigger()
y_trigger_func* _y_trigger_end_trans = NULL;
//! \see y_set_sub_transaction_start_trigger()
y_trigger_func* _y_trigger_start_sub_trans = NULL;
//! \see y_set_sub_transaction_end_trigger()
y_trigger_func* _y_trigger_end_sub_trans = NULL;

//! \see y_set_transaction_start_implementation()
typedef int (y_trans_start_impl_func)(char* trans_name);
//! \see y_set_transaction_end_implementation()
typedef int (y_trans_end_impl_func)(char* trans_name, int status);
//! Start transaction implementation pointer. Default lr_start_transaction(). \see y_set_transaction_start_implementation()
y_trans_start_impl_func* _y_trans_start_impl = &lr_start_transaction;
//! End transaction implementation pointer. Default lr_end_transaction(). \see y_set_transaction_end_implementation()
y_trans_end_impl_func* _y_trans_end_impl = &lr_end_transaction;



// Functions

/*! \brief Workaround for a bug in LR 11. 

If you have a script that does not contain any regular loadrunner transactions but leans exclusively on ylib instead a very interesting error occurs when running.
Adding calls to lr_start_transaction() and lr_end_transaction() that are never actually used is enough to stop that from occurring.
Alternatively, the "y_start_trans_impl_func" and "y_end_trans_impl_func" pointers could be initialized to NULL. But that would stop y_start/y_end_transaction() from
actually recording transactions.. so we're not going to do that. :)
*/
void __y_do_not_call_this_is_a_workaround_that_only_exists_to_prevent_a_null_dereference_error_in_vugen_when_running()
{
    lr_start_transaction("y_workaround_transaction_to_prevent_null_dereference");
    lr_end_transaction(  "y_workaround_transaction_to_prevent_null_dereference", LR_AUTO);
}


// Getters / Setters //
/*! Get the most recent transaction name as set by y_start_transaction().
\return The current full transaction name as set by y_start_transaction()
\note Returned pointer points to memory allocated by lr_eval_string().

\see y_start_transaction()
*/
char *y_get_current_transaction_name()
{
    return lr_eval_string("{y_current_transaction}");
}

/*! Store the name of the current transaction.

\param [in] trans_name The full name of the transaction.
\see y_start_transaction()
*/
void y_set_current_transaction_name(char *trans_name)
{
    lr_save_string(lr_eval_string(trans_name), "y_current_transaction");
}


/*! Get the most recent subtransaction name as set by y_start_sub_transaction().
\return The current full subtransaction name as set by y_start_sub_transaction()
\note Returned pointer points to memory allocated by lr_eval_string().

\see y_start_sub_transaction()
*/
char *y_get_current_sub_transaction_name()
{
    return lr_eval_string("{y_current_sub_transaction}");
}


/*! Store the name of the current subtransaction.

\param [in] trans_name The full name of the subtransaction.
\see y_start_sub_transaction()
*/
void y_set_current_sub_transaction_name(char *trans_name)
{
    lr_save_string(lr_eval_string(trans_name), "y_current_sub_transaction");
}


/*! Toggle using the vuser group name in ylib transaction names
Useful if you wish to set up two virtual user groups executing the exact same script with some kind of subtle different that makes it necessary to differentiate between the transactions from each group.
Default is off.

\param [in] add_group_to_trans Value determining if the group name is added to transaction name. Set to 1 it will be added, if set to 0 it will not be.

\see y_start_transaction()
*/ 
void y_set_add_group_to_transaction(int add_group_to_trans)
{
    _y_add_group_to_trans = add_group_to_trans;
}

//! \cond function_removal
// Complain loudly at compile time if somebody tries to use the old versions of these calls
#define y_set_action_prefix 0_y_set_action_prefix_no_longer_exists_please_use_action_prefix
#define y_get_action_prefix 0_y_get_action_prefix_no_longer_exists_please_use_y_get_transaction_prefix
//! \endcond

/*! Set the common transaction prefix for all subsequent transactions.

\param [in] transaction_prefix The transaction prefix to store.
Used automatically by y_start_transaction_block() and friends.

\see y_start_transaction_block()
*/
void y_set_transaction_prefix(char *transaction_prefix)
{
    // Bother. Let's do this the eminently simple and predictable way, then:
    lr_save_string(transaction_prefix, "y_transaction_prefix");
}

/*! Get the currently used transaction prefix.

In some cases you may wish to make decisions based on what kind of clickflow is currently executing. The transaction prefix may provide exactly what you need to determine that, if you use different transaction blocks for different clickflows.

\return the current transaction_prefix.
\note The returned pointer points to memory allocated by lr_eval_string()

\see y_start_transaction_block(), y_set_transaction_prefix()
*/
char *y_get_transaction_prefix()
{
    // Bother. Let's do this the eminently simple and predictable way, then:
    if(y_is_empty_parameter("y_transaction_prefix"))
    {
        y_set_transaction_prefix("");
        return "";
    }
    else 
        return lr_eval_string("{y_transaction_prefix}");
}

/*! Get the transaction number used for the next transaction started by y_start_transaction()

Useful for complicated flows where a little bit of handcrafted helper code is needed to keep the count correct.

\returns The transaction number used for the next transaction started by y_start_transaction()
\see y_start_transaction(), y_post_increment_transaction_nr()
*/
int y_get_next_transaction_nr()
{
    return _y_transaction_nr;
}

/*! Increment the transaction number used by the transaction naming code.

\deprecated - y_increment_transaction_nr() does the same thing.

\returns The transaction number used for the next transaction started by y_start_transaction()
\see y_start_transaction(), y_get_next_transaction_nr()
*/
int y_post_increment_transaction_nr()
{
    return _y_transaction_nr++;
}

/*! Increment the transaction number used by the transaction naming code.

Used internally.

\returns The transaction number used for the next transaction started by y_start_transaction()
\see y_start_transaction(), y_get_next_transaction_nr()
*/
int y_increment_transaction_nr()
{
    return _y_transaction_nr++;
}


void y_set_next_transaction_nr(int trans_nr)
{
    _y_transaction_nr = trans_nr;
}


int y_get_next_sub_transaction_nr()
{
    return _y_sub_transaction_nr;
}


int y_post_increment_sub_transaction_nr()
{
    return _y_sub_transaction_nr++;
}

int y_increment_sub_transaction_nr()
{
    return _y_sub_transaction_nr++;
}

void y_set_next_sub_transaction_nr(int trans_nr)
{
    _y_sub_transaction_nr = trans_nr;
}

// Complain loudly with a compiler error if people still use the old variants of the above.
// We renamed these on purpose, the semantics of these functions are subtly different from the old ones so existing scripts need to change.
// The most important change is that the internal transaction number now represents the *next* transaction number, not the previous one.
#define y_set_transaction_nr 0y_set_transaction_nr_no_longer_exists_please_use_y_set_next_transaction_nr
#define y_get_transaction_nr 0y_get_transaction_nr_no_longer_exists_please_use_y_get_next_transaction_nr
#define y_get_and_increment_transaction_nr 0y_get_and_increment_transaction_nr_no_longer_exists_please_use_y_increment_transaction_nr
#define y_get_sub_transaction_nr 0y_get_sub_transaction_nr_no_longer_exists_please_use_y_get_next_sub_transaction_nr
#define y_get_and_increment_sub_transaction_nr 0y_get_and_increment_sub_transaction_nr_no_longer_exists_please_use_y_increment_sub_transaction_nr
#define y_set_sub_transaction_nr 0y_set_sub_transaction_nr_no_longer_exists_please_use_y_set_next_sub_transaction_nr


int y_get_transaction_running()
{
    return _y_trans_status;
}


/******
//
//Users can now set trigger functions for the start and end of y_ transactions. 
//These triggers will run just before the transaction measurements start, and 
//just before the transaction measurements end.
//
//Usage:
//Define a new function, as follows:
//
//int register_save_params_trigger()
//{
//   web_reg_save_param("something", ...);
//   web_reg_save_param("same thing in a slightly different layout", ...);
//
//   return LR_PASS; // this is ignored for transaction start triggers
//}
//
//Then, at the point where this trigger should start firing call:
//
//y_set_transaction_start_trigger( &register_save_params_trigger );
//
//From then on out every call to y_start_transaction() and 
//y_start_sub_transaction() will fire the trigger. 
//A call to y_run_transaction_start_trigger() will fire the trigger too.
//
//To stop the trigger from firing call 'y_set_transaction_start_trigger(NULL);'
//
//Transaction end triggers are mirror images of transaction start triggers, 
//with the notable exception that the return value is not ignored, but 
//used to set the end status of the transaction before it ends.
//
*****/

void y_set_transaction_start_trigger( y_trigger_func *trigger_function )
{
    _y_trigger_start_trans = trigger_function;
}

void y_set_transaction_end_trigger( y_trigger_func *trigger_function )
{
    _y_trigger_end_trans = trigger_function;
}

void y_set_sub_transaction_start_trigger( y_trigger_func *trigger_function )
{
    _y_trigger_start_sub_trans = trigger_function;
}

void y_set_sub_transaction_end_trigger( y_trigger_func *trigger_function )
{
    _y_trigger_end_sub_trans = trigger_function;
}



// Transaction implementation change support
// For those people who for some reason feel compelled to implement lr_start_transaction() and lr_end_transaction themselves :p

// Definitions can be found further up. For reference:
// y_trans_start_impl_func* _y_trans_start_impl = &lr_start_transaction;
// y_trans_end_impl_func* _y_trans_end_impl = &lr_end_transaction;
// 
void y_set_transaction_start_implementation( y_trans_start_impl_func* trans_start_func )
{
   _y_trans_start_impl = trans_start_func;
}

void y_set_transaction_end_implementation( y_trans_end_impl_func* trans_end_func )
{
   _y_trans_end_impl = trans_end_func;
}

y_trans_start_impl_func* y_get_transaction_start_implementation()
{
   return _y_trans_start_impl;
}

y_trans_end_impl_func* y_get_transaction_end_implementation()
{
   return _y_trans_end_impl;
}


// End getters/setters



int y_run_transaction_start_trigger()
{
    if( _y_trigger_start_trans == NULL )
    {
        return 0;
    }
    else return _y_trigger_start_trans();
}

int y_run_transaction_end_trigger()
{
    if( _y_trigger_end_trans == NULL )
    {
        return 0;
    }
    else return _y_trigger_end_trans();
}

int y_run_sub_transaction_start_trigger()
{
    if( _y_trigger_start_sub_trans == NULL )
    {
        return 0;
    }
    else return _y_trigger_start_sub_trans();
}

int y_run_sub_transaction_end_trigger()
{
    if( _y_trigger_end_sub_trans == NULL )
    {
        return 0;
    }
    else return _y_trigger_end_sub_trans();
}


//
// Helper function to save the transaction end status before y_end_(sub_)transaction() closes them.
// 
void y_save_transaction_end_status(char* transaction_name, const char* saveparam, int status)
{
    int actual_status = lr_get_transaction_status(transaction_name);

    if( actual_status == LR_PASS )
    {
        // LR thinks everything is fine. If our code doesn't that becomes the new end status.
        lr_set_transaction_status(status);
    }
    else // in case of a LR reported fail status of some kind.
    {
        // The loadrunner reported status takes precendence as those errors are usually quite fundamental.
        status = actual_status;
    }

    if( actual_status == -16863 ) 
    {
        // Fix me: Lookup the corresponding LR constant.
        lr_log_message("Warning: Possible attempt to close a transaction that has not been opened!");
    }
    lr_save_int(status, saveparam);
}




/////////// END HELPERS //////////



int y_session_transaction_count_increment()
{
    return y_session_transaction_count++;
}

void y_session_transaction_count_report(char* session_name)
{
    lr_log_message( lr_eval_string("Transaction count for %s: %d"), session_name, y_session_transaction_count);
    lr_user_data_point( session_name, y_session_transaction_count);
}

void y_session_transaction_count_reset()
{
    y_session_transaction_count = 0;
}



//
// Transaction blocks. Prefix all transactions in a series with the same text.
// 

void y_start_transaction_block(char *transaction_prefix)
{
    lr_log_message("Starting transaction block %s", transaction_prefix);
    y_set_transaction_prefix(transaction_prefix);
    y_set_next_transaction_nr(1);

    // Start a transaction to measure total time spend in this block
    // 
    //snprintf(_block_transaction, strlen(transaction_prefix)+7, "%s_TOTAL", transaction_prefix);
    //lr_start_transaction(_block_transaction);
}

void y_end_transaction_block()
{
    lr_log_message("Ending transaction block %s", y_get_transaction_prefix());
    y_set_transaction_prefix("");
}


void y_pause_transaction_block()
{
    lr_log_message("Pausing transaction block %s", y_get_transaction_prefix());
    if( y_is_empty_parameter("y_transaction_prefix") )
    {
        lr_error_message("Attempt to pause transaction block when none has been started!");
        return;
    }
    lr_save_int( y_get_next_transaction_nr(), lr_eval_string("y_paused_transaction_block_{y_transaction_prefix}_trans_nr") );
    y_end_transaction_block();
}


void y_resume_transaction_block(char *transaction_prefix)
{
    char *storage_param;
    lr_log_message("Resuming transaction block %s", transaction_prefix);

    lr_save_string(transaction_prefix, "y_resumed_transaction_block");
    storage_param = lr_eval_string("y_paused_transaction_block_{y_resumed_transaction_block}_trans_nr");

    if( y_is_empty_parameter(storage_param) )
    {
        lr_error_message("Attempt to resume transaction block %s but no such block has been paused.", transaction_prefix);
        return;
    }

    y_start_transaction_block(transaction_prefix);
    y_set_next_transaction_nr(atoi(y_get_parameter(storage_param)));
}


// DEPRECATED
void y_start_action_block(char *transaction_prefix)
{
    y_start_transaction_block(transaction_prefix);
}


// DEPRECATED
void y_end_action_block()
{
    y_end_transaction_block();
}


//! \cond function_removal
// Complain loudly at compile time if somebody tries to use the old versions of this call
#define y_calculate_actual_action_prefix 0_y_calculate_actual_action_prefix_no_longer_exists_please_use_y_calculate_actual_transaction_prefix
//! \endcond


/*! \brief Transaction name factory helper.

Add together the transaction prefix, vuser group name (if applicable), and transaction number, seperated by "_", and return the result in a newly allocated piece of memory on the heap.
Automatically called by y_start_transaction() and friends as needed.

\param [in] transaction_prefix The current transaction prefix as given to y_start_transaction_block()
\returns A newly allocated piece of memory on the heap containing the complete prefix, ready for concatenation with the actual transaction name.

\warning the return value of this function needs to be freed using free().

\see y_start_transaction(), y_create_transaction_name()
*/
char *y_calculate_actual_transaction_prefix(const char *transaction_prefix)
{
    const char seperator[] = "_";
    const int seperator_len = sizeof seperator - 1; // strlen(seperator);
    int group_len = 0;
    int prefix_len = strlen(transaction_prefix);
    char *buffer;
    size_t buffer_size;

    // y_virtual_user_group is set only if y_setup() is called.
    // See y_loadrunner_utils.c
    y_setup();
    if( _y_add_group_to_trans )
    {
        group_len = strlen(y_virtual_user_group);
    }

    // add room for the seperators
    if(prefix_len > 0)
    {
        prefix_len += seperator_len;
    }
    if(group_len > 0)
    {
        group_len += seperator_len;
    }

    // allocate memory -- note this needs to be free()'ed afterwards!
    buffer_size = group_len + prefix_len + 1;
    buffer = y_mem_alloc(buffer_size);
    buffer[0] = '\0';

    // start concatenating things together
    {
        int len = 0;
        if(group_len > 0)
        {
            len = snprintf(buffer, buffer_size, "%s%s", y_virtual_user_group, seperator);
        }
        if(prefix_len > 0) 
        {
            snprintf(buffer + len, buffer_size-len,"%s%s", transaction_prefix, seperator);
        }
    }
    return buffer;
}


//
// Generates the transaction name prefixed with a user defined action prefix and a transaction number.
// The result is saved in the "y_current_transaction" loadrunner parameter for use by some macro's.
//
// To use this scripts need to call y_start_action_block() - once at the start of each action.
//
// 
// Dirty trick that no longer needs to be used:
//#define lr_start_transaction(transaction_name) y_start_new_transaction_name(transaction_name, _y_transaction_prefix, _trans_nr++); \
//                                               lr_start_transaction(lr_eval_string("{y_current_transaction}"))
//#define lr_end_transaction(transaction_name, status) lr_end_transaction(lr_eval_string("{y_current_transaction}"), status)
//
void y_create_new_transaction_name(const char *transaction_name, const char *transaction_prefix, int transaction_nr)
{
    const int trans_nr_len = 2;    // eg. '01'
    char *actual_prefix = y_calculate_actual_transaction_prefix(transaction_prefix);
    int prefix_len = strlen(actual_prefix);
    int trans_name_size = prefix_len + trans_nr_len +1 + strlen(transaction_name) +1;
    char *actual_trans_name = y_mem_alloc( trans_name_size );

    if( transaction_nr >= 100 )
    {
        lr_error_message("Transaction count too high (100+). Are you using y_start_action_block()?");
        lr_exit(LR_EXIT_VUSER, LR_FAIL);
    }

    snprintf(actual_trans_name, trans_name_size, "%s%02d_%s", actual_prefix, transaction_nr, transaction_name);
    free(actual_prefix);
    y_set_current_transaction_name(actual_trans_name);
    free(actual_trans_name);
}

void y_create_next_transaction_name( const char* transaction_name)
{
    y_create_new_transaction_name(transaction_name, y_get_transaction_prefix(), y_increment_transaction_nr());
}



// 
// Todo: Find a way to make this share more code with y_create_new_transaction_name()
//
void y_create_new_sub_transaction_name(const char *transaction_name, const char *transaction_prefix, 
                                       const int transaction_nr, const int sub_transaction_nr)
{
    const int trans_nr_len = 2;    // eg. '01'
    char *actual_prefix = y_calculate_actual_transaction_prefix(transaction_prefix);
    int prefix_len = strlen(actual_prefix);
    int trans_name_size = prefix_len + (2 * (trans_nr_len +1)) + strlen(transaction_name) +1;
    char *actual_trans_name = y_mem_alloc( trans_name_size );

    if( transaction_nr >= 100 )
    {
        lr_error_message("Transaction count too high (100+). Are you using y_start_action_block()?");
        lr_exit(LR_EXIT_VUSER, LR_FAIL);
    }

    snprintf(actual_trans_name, trans_name_size, "%s%02d_%02d_%s", actual_prefix, transaction_nr, sub_transaction_nr, transaction_name);
    free(actual_prefix);
    y_set_current_sub_transaction_name(actual_trans_name);
    free(actual_trans_name);
}


void y_create_next_sub_transaction_name(const char* transaction_name)
{
    y_create_new_sub_transaction_name(transaction_name,
                                    y_get_transaction_prefix(),
                                    y_get_next_transaction_nr()-1,
                                    y_increment_sub_transaction_nr());
}

//
// y_start_transaction() / y_end_transaction()
// These are drop-in replacements for the loadrunner functions 
// lr_start_transaction() and lr_end_transaction(). 
// 
// These variants will add a bit more logging to facilitate post-test analysis
// with external tools, and add a common prefix based on the action block (see above)
// as well as consistent numbering.
// 
int y_start_transaction(char *transaction_name)
{
    // This saves it's result in the 'y_current_transaction' parameter.
    y_create_next_transaction_name(transaction_name);

    // Reset the sub transaction numbering.
    y_set_next_sub_transaction_nr(1);

    // Fire the start trigger. For complicated web_reg_find() / web_reg_save_param() 
    // statement collections that we want run right before starting every
    // transaction in a group of transactions.
    // Placed after the generation of the transaction name since that might be 
    // a nice thing to have available.for trigger authors.
    y_run_transaction_start_trigger();

    // Stops sub transactions from automagically
    // creating outer transactions for themselves.
    _y_trans_status = Y_TRANS_STATUS_STARTED;

    //return lr_start_transaction(lr_eval_string("{y_current_transaction}"));
    return _y_trans_start_impl(lr_eval_string("{y_current_transaction}"));
}


int y_start_transaction_with_number(char *transaction_name, int transaction_number)
{
    y_set_next_transaction_nr(transaction_number);
    return y_start_transaction(transaction_name);
}



// Note: This completely ignores the 'transaction_name' argument
// to retain compatibility with lr_end_transaction().
int y_end_transaction(char *transaction_name, int status)
{
    char *trans_name = lr_eval_string("{y_current_transaction}");

    // Fire the transaction end trigger. For processing the results of 
    // complicated web_reg_find() / web_reg_save_param() statement 
    // collections that repeat for a group of transactions.
    // This fires before the actual end of the transaction so that it can 
    // still influence the transaction end status.
    int trigger_result = y_run_transaction_end_trigger();
    if( status == LR_PASS && trigger_result != LR_PASS )
    {
        lr_error_message("Transaction end trigger did not return LR_PASS");
        status = trigger_result;
    }

    // Save the end status of this transaction. It won't be available after ending it.
    y_save_transaction_end_status(trans_name, "y_last_transaction_status", status);

    // Debugging: report wasted time.
    // People who implement their own triggers will have to do this themselves.
    if( _y_wasted_time_graph && _y_trans_end_impl == lr_end_transaction )
        lr_user_data_point( lr_eval_string("wasted_{y_current_transaction}"), lr_get_transaction_wasted_time(trans_name));

    // End the transaction
    status = _y_trans_end_impl(trans_name, status);

    // Tell our subtransaction support that there is no outer transaction
    // so if a sub-transaction is created it may have to fake this.
    _y_trans_status = Y_TRANS_STATUS_NONE;

    if( y_session_transaction_count >= 0 ) // Values smaller than 0 means that transaction counting is disabled.
        y_session_transaction_count_increment();
    
    return status;
}

//
// y_start_sub_transaction() / y_end_sub_transaction()
// Like y_start_transaction() / y_end_transaction().
// 
// Can be called outside of a running top level transaction, 
// in which case it will create one as needed.
// 
// This is not meant an excuse to be lazy about adding top level transactions
// but as a fallback for situations where the sub transactions can not neccesarily
// be predicted. (Think 'sudden popups from GUI apps' and similar cases.)
// 
int y_start_sub_transaction(char *transaction_name)
{
    // if there is no outer transaction yet, fake one
    if( _y_trans_status == Y_TRANS_STATUS_NONE )
    {
        y_start_transaction(transaction_name);
        _y_trans_status = Y_TRANS_STATUS_AUTO_STARTED;
    }


    y_create_next_sub_transaction_name(transaction_name);

    // Fire the transaction start trigger.
    y_run_sub_transaction_start_trigger();

    // Actual sub transaction start
    return lr_start_sub_transaction(
        lr_eval_string("{y_current_sub_transaction}"), 
        lr_eval_string("{y_current_transaction}"));
}

int y_start_sub_transaction_with_number(char *transaction_name, int transaction_number)
{
    y_set_next_sub_transaction_nr(transaction_number);
    return y_start_sub_transaction(transaction_name);
}



int y_end_sub_transaction(char *transaction_name, int status)
{
    char *trans_name = lr_eval_string("{y_current_sub_transaction}");

    // Fire the transaction end trigger.
    int trigger_result = y_run_sub_transaction_end_trigger();
    if( status == LR_PASS && trigger_result != LR_PASS )
    {
        status = trigger_result;
    }

    // Save the end status of this transaction. It won't be available after ending it.
    y_save_transaction_end_status(trans_name, "y_last_sub_transaction_status", status);

    // Debugging: report wasted time.
    if( _y_wasted_time_graph ) 
        lr_user_data_point( lr_eval_string("wasted_{y_current_sub_transaction}"), lr_get_transaction_wasted_time(trans_name));

    // End the transaction
    status = lr_end_sub_transaction(trans_name, status);

    // if we faked an outer transaction, fake closing it.
    //
    // Note: It might be an idea to move this to y_start_(sub_)transaction() instead, for
    // better grouping. That may not be without it's problems though.
    if( _y_trans_status == Y_TRANS_STATUS_AUTO_STARTED)
    {
        y_end_transaction(transaction_name, status);
    }
    return status;
}

int y_get_last_transaction_status()
{
    char* last_trans_status_string = y_get_parameter_or_null("y_last_transaction_status");
    if( last_trans_status_string == NULL )
    {
        return LR_AUTO; // No earlier transaction, the parameter doesn't even exist.
    }
    else
    {
        return atoi(last_trans_status_string);
    }
}



//! Session timer variable for session timer support.
merc_timer_handle_t y_session_timer = NULL;


void y_session_timer_start(char* session_name)
{
    lr_save_string(session_name, "y_session_name");
    y_session_transaction_count_reset();

    y_session_timer = lr_start_timer();
}

// Meet sessie duur en forceer een pause tot het einde van de sessie, indien nodig.
#define Y_NO_PAUSE    0
#define Y_FORCE_PAUSE 1


void y_session_timer_end(int required_session_duration, int force_pause)
{
    double measured_duration;

    if( y_session_timer == NULL )
    {
        lr_error_message("Error: y_session_timer_end() called without matching call to y_session_timer_end()!");
        lr_set_transaction( "__y_sesssion_timer_end_call_without_y_session_timer_end_call", 0, LR_FAIL);
        return;
    }
    else
    {
        double measured_duration = lr_end_timer(y_session_timer);
        // Calculate how much time remains until the session should end.
        int remaining_time = required_session_duration - measured_duration;

        // Reset the timer insofar lr_end_timer() hasn't done that already.
        y_session_timer = NULL;

        lr_user_data_point( lr_eval_string("y_session_duration_{y_session_name}"), measured_duration);
        if ( remaining_time > 0 ) 
        {
            if( force_pause == Y_FORCE_PAUSE )
            {
                lr_force_think_time(remaining_time);
            }
        }
        else
        {
            lr_error_message( lr_eval_string("WARNING!: Measured duration of session {y_session_name} (%f) exceeded specified maximum of %d seconds!"), measured_duration, required_session_duration);
            lr_set_transaction( lr_eval_string("_{y_session_name}_session_duration_overrun"), measured_duration, LR_FAIL);
        }
    }

    y_session_transaction_count_report(lr_eval_string("y_transaction_count_{y_session_name}"));
}


//
// Shorthand for 
// "y_start_transaction(transaction); web_link(linkname); y_end_transaction(transaction)"
//
// Note: In order for the logging to report correct line numbers it is advised to use the macro
// version TRANS_WEB_LINK() found below.
//
// @author: Floris Kraak
// 
void y_trans_web_link(char *transaction, char *linkname)
{
    char *link = lr_eval_string(linkname);
    char *tmp, *trans;
    size_t size;

    if( !(strlen(link) > 0) )
    {
        lr_error_message("Zero-length link name - correlation error?");
        lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_AUTO);
        return;
    }
    
    size = strlen(link) + strlen("Text=") +1;
    tmp = y_mem_alloc(size);
    snprintf(tmp, size, "Text=%s", link);

    trans = lr_eval_string(transaction);
    y_start_transaction(trans);
    web_link(link, tmp, LAST);
    y_end_transaction(trans, LR_AUTO);
    
    free(tmp);
}



//
// Shorthand for 
// "y_start_transaction(transaction); web_link(linkname); y_end_transaction(transaction)"
// Macro version. Use this to preserve line numbers in the virtual user log.
// For the regular version see y_trans_web_link() above.
// 
// Use as if calling a function named TRANS_WEB_LINK() with two arguments.
// (Being able to just search/replace y_trans_web_link( with Y_TRANS_WEB_LINK( in
// a vUser script is a feature, so keep the interfaces and functionality identical
// please.)
// 
#define Y_TRANS_WEB_LINK( TRANSACTION, LINKNAME )                         \
do {                                                                      \
    char *link = lr_eval_string(LINKNAME);                                \
    char *tmp, *trans;                                                    \
    size_t size;                                                          \
                                                                          \
    if( !(strlen(link) > 0) )                                             \
    {                                                                     \
        lr_error_message("Zero-length link name - correlation error?");   \
        lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_AUTO);                 \
        return;                                                           \
    }                                                                     \
                                                                          \
    size = strlen(link) + strlen("Text=") +1;                             \
    tmp = y_mem_alloc(size);                                              \
    snprintf(tmp, size, "Text=%s", link);                                 \
                                                                          \
    trans = lr_eval_string(TRANSACTION);                                  \
    y_start_transaction(trans);                                           \
    web_link(link, tmp, LAST);                                            \
    y_end_transaction(trans, LR_AUTO);                                    \
                                                                          \
    free(tmp);                                                            \
} while(0)


/*! \brief Add the dynatrace header required for instrumenting transactions with dynatrace.
\param [in] transaction_name Loadrunner transaction name - value the "NA" part of the header should take.
\param [in] additional_headers Any additional parts of the header spec you wish to add.

This function will fill in the "VU" (virtual user id) and "ID" parts of the header itself using the current
virtual user id and a generated unique id based on the current time as reported by web_save_timestamp().

\see https://community.dynatrace.com/community/display/DOCDT63/Integration+with+Web+Load+Testing+and+Monitoring+Tools



\b Usage:
\code
int customer_start_transaction_trigger()
{
	// Add the dynatrace header if -dynatrace_enabled was set to 1 as a script argument
	y_add_dynatrace_header("{y_current_transaction}", "PC={y_current_transaction}");
	return 0;
}

vuser_init()
{
	// This will add the dynatrace header to the next transaction if -dynatrace_enabled was 
	// set to a non-zero value in the attributes.
	y_set_transaction_start_trigger( &customer_start_transaction_trigger );
}
\endcode

\author Floris Kraak
*/
void y_add_dynatrace_header(char* transaction_name, char* additional_headers)
{
	{
		char* arg;
		if( ((arg = lr_get_attrib_string("dynatrace_enabled")) == NULL) || (atoi(arg) < 1) )
		{
			lr_log_message("Dynatrace headers disabled.");
			return;
		}
		else
			lr_log_message("Dynatrace headers enabled!");
	}
	web_remove_auto_header("X-dynaTrace", "ImplicitGen=No", LAST);
	
	if( transaction_name == NULL)
	{
		lr_error_message("y_add_dynatrace_header(): transaction_name argument is NULL.");
		return;
	}
	else if ( additional_headers == NULL )
	{
		lr_error_message("y_add_dynatrace_header(): additional_arguments argument is NULL.");
		return;
	}
	else
	{
		char vuserstring[10];

		{
			int vuserid, scid;
			char *groupid;
			lr_whoami(&vuserid, &groupid, &scid);
			itoa(vuserid,vuserstring,10);
		}
		web_save_timestamp_param("y_dynatrace_timestamp", LAST);
		
		{
			char* timestamp = lr_eval_string("{y_dynatrace_timestamp}");
			int header_length = strlen(transaction_name)+4 + strlen(vuserstring)+4 + strlen(timestamp)+4 + strlen(additional_headers)+1;
			char* header = (char*) y_mem_alloc(sizeof(char) * header_length);
			snprintf(header, header_length, "NA=%s;VU=%s;%ID=%s;%s", transaction_name, vuserstring, timestamp, additional_headers);		
			web_add_auto_header("X-dynaTrace", header);
			free(header);
		}
	}
}

#endif // _Y_TRANSACTION_C_
