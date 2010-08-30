/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2010 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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


#ifndef _TRANSACTION_C
#define _TRANSACTION_C

//
// Transaction names take the form: '{action_prefix}_{transaction_nr}_{step_name}'
// This file helps do that automatically.
//
// 
/* 
    // Testcase:
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
*/



// Needed to compile this - the definition of LAST is missing if it's not included
#include "web_api.h"

// Other Ylib functions
#include "y_logging.c"
#include "y_string.c"
#include "y_loadrunner_utils.c"


// Variables //


// Never access these variables directly - names may change. 
// Use the get() and set() functions instead, if available. (otherwise, add them?)

char *_y_action_prefix = "";
int _y_add_group_to_trans = 0;      // whether to add the name of the vuser group to the transaction names. 1 = on, 0 = off.

    // We could allocate _block_transaction with malloc
    // but freeing that gets complicated quickly. Too quickly.
//char _block_transaction[100] = "";
int _y_transaction_nr = 0;
int _y_sub_transaction_nr = 0;

// Transaction status tracking
#define Y_TRANS_STATUS_NONE         0
#define Y_TRANS_STATUS_STARTED      1
#define Y_TRANS_STATUS_AUTO_STARTED 2
int _trans_status = Y_TRANS_STATUS_NONE;

// Transaction trigger support
typedef int (y_trigger_func)();
y_trigger_func *_y_trigger_start_trans = NULL;
y_trigger_func *_y_trigger_end_trans = NULL;

// Getters / Setters //

char *y_get_current_transaction_name()
{
    return lr_eval_string("{current_transaction}");
}


void y_set_current_transaction_name(char *trans_name)
{
    lr_save_string(lr_eval_string(trans_name), "current_transaction");
}


char *y_get_current_sub_transaction_name()
{
    return lr_eval_string("{current_sub_transaction}");
}


void y_set_current_sub_transaction_name(char *trans_name)
{
    lr_save_string(lr_eval_string(trans_name), "current_sub_transaction");
}


char *y_get_action_prefix()
{
    return _y_action_prefix;
}

void y_set_add_group_to_transaction(int add_group_to_trans)
{
    _y_add_group_to_trans = add_group_to_trans;
}


void y_set_action_prefix(char *action_prefix)
{
    lr_save_string(action_prefix, "y_action_prefix");
    _y_action_prefix = action_prefix;
}



int y_get_transaction_nr()
{
    return _y_transaction_nr;

}


int y_get_and_increment_transaction_nr()
{
    return ++_y_transaction_nr;
}


void y_set_transaction_nr(int trans_nr)
{
    _y_transaction_nr = trans_nr;
}


int y_get_sub_transaction_nr()
{
    return _y_sub_transaction_nr;
}


int y_get_and_increment_sub_transaction_nr()
{
    return ++_y_sub_transaction_nr;
}


void y_set_sub_transaction_nr(int trans_nr)
{
    _y_sub_transaction_nr = trans_nr;
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
//A call to //y_run_transaction_start_trigger() will fire the trigger too.
//
//To stop the trigger from firing call 'y_set_transaction_start_trigger(NULL);'
//
//Transaction end triggers are mirror images of transaction end triggers, 
//with the notable exception that the return //value is not ignored, but 
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

//
// Helper function to save the transaction end status before y_end_(sub_)transaction() closes them.
// 
y_save_transaction_end_status(char* transaction_name, const char* saveparam, int status)
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



//
// Action blocks. Name all transactions within an action block with a common prefix.
// Ajust to taste.
// 

void y_end_action_block()
{
    //if(strlen(_block_transaction))
    //{
    //    lr_end_transaction(_block_transaction, LR_AUTO);
    //}
    //_block_transaction[0] = '\0';
    y_set_action_prefix("");
}

void y_start_action_block(char *action_prefix)
{
    if(strlen(y_get_action_prefix()))
    {
        y_end_action_block();
    }

    y_set_action_prefix(action_prefix);
    y_set_transaction_nr(0);

    // Start a transaction to measure total time spend in this block
    // 
    //sprintf(_block_transaction, "%s_TOTAL", action_prefix);
    //lr_start_transaction(_block_transaction);
}


char *y_calculate_actual_action_prefix(const char *action_prefix)
{
    const char *seperator = "_";
    int group_len = 0;
    int prefix_len = strlen(action_prefix);
    char *buffer;


    // _vUserGroup is set only when _y_extra_logging is set.
    // See logging.c -> y_setup_logging().
    if( _y_add_group_to_trans && (_vUserGroup != NULL))
    {
        group_len = strlen(_vUserGroup);
    }

    // add room for the seperators
    if(prefix_len > 0)
    {
        prefix_len++;
    }
    if(group_len > 0)
    {
        group_len++;
    }

    // allocate memory -- note this needs to be free()'ed afterwards!
    buffer = y_mem_alloc(group_len + prefix_len+1);
    buffer[0] = '\0';

    // start concatenating things together
    if(group_len > 0)
    {
        strcat(buffer, _vUserGroup);
        strcat(buffer, seperator);
    }
    if(prefix_len > 0)
    {
        strcat(buffer, action_prefix);
        strcat(buffer,seperator);
    }

    return buffer;
}


//
// Generates the transaction name prefixed with a user defined action prefix and a transaction number.
// The result is saved in the "current_transaction" loadrunner parameter for use by some macro's.
//
// To use this scripts need to call y_start_action_block() - once at the start of each action.
//
// 
// Dirty trick that no longer needs to be used:
//#define lr_start_transaction(transaction_name) y_start_new_transaction_name(transaction_name, _y_action_prefix, _trans_nr++); \
//                                               lr_start_transaction(lr_eval_string("{current_transaction}"))
//#define lr_end_transaction(transaction_name, status) lr_end_transaction(lr_eval_string("{current_transaction}"), status)
//
void y_create_new_transaction_name(const char *transaction_name, const char *action_prefix, int transaction_nr)
{
    const int trans_nr_len = 2;    // eg. '01'
    char *actual_prefix = y_calculate_actual_action_prefix(action_prefix);
    int prefix_len = strlen(actual_prefix);
    int trans_name_size = prefix_len + trans_nr_len +1 + strlen(transaction_name) +1;
    char *actual_trans_name = y_mem_alloc( trans_name_size );

	if( transaction_nr >= 100 )
	{
		y_log_error("Transaction count too high (100+). Are you using y_start_action_block()?");
		lr_exit(LR_EXIT_VUSER, LR_FAIL);
	}

    sprintf(actual_trans_name, "%s%02d %s", actual_prefix, transaction_nr, transaction_name);
    free(actual_prefix);
    y_set_current_transaction_name(actual_trans_name);
    free(actual_trans_name);
}


// 
// Todo: Find a way to make this share more code with y_create_new_transaction_name()
//
void y_create_new_sub_transaction_name(const char *transaction_name, const char *action_prefix, 
                                       const int transaction_nr, const int sub_transaction_nr)
{
    const int trans_nr_len = 2;    // eg. '01'
    char *actual_prefix = y_calculate_actual_action_prefix(action_prefix);
    int prefix_len = strlen(actual_prefix);
    int trans_name_size = prefix_len + (2 * (trans_nr_len +1)) + strlen(transaction_name) +1;
    char *actual_trans_name = y_mem_alloc( trans_name_size );

	if( transaction_nr >= 100 )
	{
		y_log_error("Transaction count too high (100+). Are you using y_start_action_block()?");
		lr_exit(LR_EXIT_VUSER, LR_FAIL);
	}

    sprintf(actual_trans_name, "%s%02d_%02d %s", actual_prefix, transaction_nr, sub_transaction_nr, transaction_name);
    free(actual_prefix);
    y_set_current_sub_transaction_name(actual_trans_name);
    free(actual_trans_name);
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

void y_start_transaction(char *transaction_name)
{
    // This saves it's result in the 'current_transaction' parameter.
    y_create_new_transaction_name(transaction_name, 
                                y_get_action_prefix(), 
                                y_get_and_increment_transaction_nr());

    // Reset the sub transaction numbering.
    y_set_sub_transaction_nr(0);

	// Fire the start trigger. For complicated web_reg_find() / web_reg_save_param() 
	// statement collections that we want run right before starting every
	// transaction in a group of transactions.
	// Placed after the generation of the transaction name since that might be 
	// a nice thing to have available.for trigger authors.
	y_run_transaction_start_trigger();

    // Stops sub transactions from automagically
    // creating outer transactions for themselves.
    _trans_status = Y_TRANS_STATUS_STARTED;

    // For external analysis of the responsetimes.
    y_log_to_report(lr_eval_string("TimerOn {current_transaction}"));
    lr_start_transaction(lr_eval_string("{current_transaction}"));
}

// Note: This completely ignores the 'transaction_name' argument
// to retain compatibility with lr_end_transaction().
void y_end_transaction(char *transaction_name, int status)
{
    char *trans_name = lr_eval_string("{current_transaction}");

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
	y_save_transaction_end_status(trans_name, "current_transaction", status);
    lr_end_transaction(trans_name, status);

    // Tell our subtransaction support that there is no outer transaction
    // so if a sub-transaction is created it may have to fake this.
    _trans_status = Y_TRANS_STATUS_NONE;

    // For external analysis of the response times.
    y_log_to_report(lr_eval_string("TimerOff {current_transaction}"));

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
void y_start_sub_transaction(char *transaction_name)
{
    // if there is no outer transaction yet, fake one
    if( _trans_status == Y_TRANS_STATUS_NONE )
    {
        y_start_transaction(transaction_name);
        _trans_status = Y_TRANS_STATUS_AUTO_STARTED;
    }

    // This should not disrupt the numbering ..
    // .. but why is it needed??
    //y_set_transaction_nr( y_get_transaction_nr() -1 );

    y_create_new_sub_transaction_name(transaction_name, 
                                    y_get_action_prefix(),
                                    y_get_transaction_nr(),
                                    y_get_and_increment_sub_transaction_nr());

	// Fire the transaction start trigger.
	y_run_transaction_start_trigger();

    // For external analysis of the response times.
    y_log_to_report(lr_eval_string("TimerOn {current_sub_transaction}"));
    lr_start_sub_transaction(lr_eval_string("{current_sub_transaction}"), 
                             lr_eval_string("{current_transaction}"));
}

void y_end_sub_transaction(char *transaction_name, int status)
{
    char *trans_name = lr_eval_string("{current_sub_transaction}");

	// Fire the transaction end trigger.
	int trigger_result = y_run_transaction_end_trigger();
	if( status == LR_PASS && trigger_result != LR_PASS )
	{
		status = trigger_result;
	}

	// Save the end status of this transaction. It won't be available after ending it.
	y_save_transaction_end_status(trans_name, "last_sub_transaction_status", status);
    lr_end_sub_transaction(trans_name, status);

    // For external analysis of the response times.
    y_log_to_report(lr_eval_string("TimerOff {current_sub_transaction}"));

    // if we faked an outer transaction, fake closing it.
    //
    // Note: It might be an idea to move this to y_start_(sub_)transaction() instead, for
    // better grouping. That may not be without it's problems though.
    if( _trans_status == Y_TRANS_STATUS_AUTO_STARTED)
    {
        y_end_transaction(transaction_name, status);
    }
}




// Handy shortcuts //



//
// Shorthand for 
// "y_start_transaction(transaction); web_link(linkname); y_end_transaction(transaction)"
//
// Note: In order for the logging to report correct line numbers it is advised to use the macro
// version TRANS_WEB_LINK() found below.
//
// @author: Floris Kraak
// 
y_trans_web_link(char *transaction, char *linkname)
{
    char *link = lr_eval_string(linkname);
    char *tmp, *trans;
    
    if( !(strlen(link) > 0) )
    {
        lr_error_message("Zero-length link name - correlation error?");
        lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_AUTO);
        return;
    }
    
    tmp = y_mem_alloc(strlen(link) + strlen("Text=") +1);
    sprintf(tmp, "Text=%s", link);
    
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
                                                                          \
    if( !(strlen(link) > 0) )                                             \
    {                                                                     \
        lr_error_message("Zero-length link name - correlation error?");   \
        lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_AUTO);                 \
        return;                                                           \
    }                                                                     \
                                                                          \
    tmp = y_mem_alloc(strlen(link) + strlen("Text=") +1);                 \
    sprintf(tmp, "Text=%s", link);                                        \
                                                                          \
    trans = lr_eval_string(TRANSACTION);                                  \
    y_start_transaction(trans);                                           \
    web_link(link, tmp, LAST);                                            \
    y_end_transaction(trans, LR_AUTO);                                    \
                                                                          \
    free(tmp);                                                            \
} while(0)




//
// Deprecated - The below needs to be done in some other way that doesn't
// confuse the heck out of people ;-)
//



// Set up a series of chances that certain named steps will happen.
//
// The idea is that there is a .dat file listing names of steps and
// for each of those the chance that that step should be executed.
// The values in this .dat file are accessed 'on each occurrance' by the
// "step" and "stepchance" parameters.
//
// This function will iterate over the step parameter until it contains "END",
// and save each step chance to a seperate parameter with a name starting with
// 'step_chance_'.
//
// Before each step is executed a call is made to y_waterfall_random_weighted_continue()
// with the name of the step added. If the corresponding parameter was set up the
// value it contained is used as the weighted chance that the next part of the script
// will be executed.
//
// @author: Floris Kraak
//
y_setup_step_waterfall()
{
    char *step = lr_eval_string("{step}");
    char *stepchance = lr_eval_string("{stepchance}");
    char *head = "step_chance_";
    char *tmp;

    while ( step && (strcmp(step, "END") != 0) )
    {
        tmp = y_mem_alloc( strlen(head) + strlen(step) +1);
        // note: if the memory allocation fails we're in trouble!

        tmp[0] = '\0';       // This could be a sprintf() call instead
        strcat(tmp, head);
        strcat(tmp, step);
               
        lr_save_string(stepchance, tmp);
        free(tmp);

        // We're hiding a for() loop here ;-)
        step = lr_eval_string("{step}");
        stepchance = lr_eval_string("{stepchance}");
   }
}

//
// for full documentation see y_setup_step_waterfall()
//
// @see y_setup_step_waterfall 
// @author: Floris Kraak
//
y_waterfall_random_weighted_continue(char * stepname)
{
    char *head = "step_chance_";
    char *paramname = y_mem_alloc( strlen(head) + strlen(stepname) +3);
    char *chancestr;
    int chance = 100; // Default
    int rnum = (y_rand() % 100); // random number between 0 and 99

    lr_log_message("Weighted stop chance evaluation for %s", stepname);

    paramname[0] = '\0';
    strcat( paramname, "{");
    strcat( paramname, head);
    strcat( paramname, stepname);
    strcat( paramname, "}");

    chancestr = lr_eval_string(paramname);

    if( (strlen(chancestr) > 0) && (strcmp(chancestr, paramname) != 0) )
    {
        chance = atoi(chancestr);
    }
    free(paramname);

    //lr_log_message("rnum = %d, chance = %d", rnum, chance);

    if( rnum >= chance )
    {
        lr_log_message("Stop!");
        lr_exit(LR_EXIT_ACTION_AND_CONTINUE, LR_AUTO);
    }
    else {
        lr_log_message("No stop!");
    }

    return 0;
}


#endif // _TRANSACTION_C
