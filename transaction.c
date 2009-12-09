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
#ifndef _TRANSACTION_C
#define _TRANSACTION_C

// Needed to compile this - the definition of LAST is missing if it's not included
#include "web_api.h"

// Other LoadLib functions
#include "logging.c"
#include "string.c"


// Transaction names take the form: '{action_prefix}_{transaction_nr}_{step_name}'
// This file helps do that automatically.



// Never access these variables directly - names may change. 
// Use the get() and set() functions instead, if available. (otherwise, add them?)
char *_action_prefix = "";
char _block_transaction[100] = "";
int _transaction_nr = 0;
int _sub_transaction_nr = 0;
int _fake_sub_trans = 0;

//
// Action blocks. Name all transactions within an action block with a common prefix.
// Ajust to taste.
// 


void start_action_block(char *action_prefix)
{
    if(!strlen(get_action_prefix()))
    {
        end_action_block();
    }

    set_action_prefix(action_prefix);
    set_transaction_nr(1);

    // We could allocate _block_transaction with malloc
    // but freeing that gets complicated quickly. Too quickly.
    sprintf(_block_transaction, "%s_TOTAL", action_prefix);
    lr_start_transaction(_block_transaction);
}

void end_action_block()
{
    if(strlen(_block_transaction))
    {
        lr_end_transaction(_block_transaction, LR_AUTO);
    }
    _block_transaction[0] = '\0';
    set_action_prefix("");
}

//
// start_transaction() / end_transaction()
// These are drop-in replacements for the loadrunner functions 
// lr_start_transaction() and lr_end_transaction(). 
// 
// These variants will add a bit more logging to facilitate post-test analysis
// with external tools, and add a common prefix based on the action block (see above)
// as well as consistent numbering.
// 

void start_transaction(char *transaction_name)
{
    // This saves it's result in the 'current_transaction' parameter.
    create_new_transaction_name(transaction_name, 
                                get_action_prefix(), 
                                get_and_increment_transaction_nr());

    // Reset the sub transaction numbering.
    // This also stops sub transactions from automagically
    // creating outer transactions for themselves.
    set_sub_transaction_nr(1);

    // For external analysis of the responsetimes.
    log_to_report(lr_eval_string("TimerOn {current_transaction}"));
    lr_start_transaction(lr_eval_string("{current_transaction}"));
}

// Note: This completely ignores the 'transaction_name' argument
// to retain compatibility with lr_end_transaction().
void end_transaction(char *transaction_name, int status)
{
    char *trans_name = lr_eval_string("{current_transaction}");

    // Save the end status of this transaction. It won't be available after ending it.
    lr_save_int(lr_get_transaction_status(trans_name), "last_transaction_status");

    lr_end_transaction(trans_name, status);

    // Tell our subtransaction support that there is no outer transaction
    // so if a sub-transaction is created it may have to fake this.
    set_sub_transaction_nr(0);

    // For external analysis of the response times.
    log_to_report(lr_eval_string("TimerOff {current_transaction}"));

}


//
// start_sub_transaction() / end_sub_transactio()
// Like start_transaction() / end_transaction().
// 
// Can be called outside of a running top level transaction, 
// in which case it will create one as needed.
// 
// This is not meant an excuse to be lazy about adding top level transactions
// but as a fallback for situations where the sub transactions can not neccesarily
// be predicted. (Think 'sudden popups from GUI apps' and similar cases.)
// 
void start_sub_transaction(char *transaction_name)
{
    // if there is no outer transaction yet, fake one
    if( get_sub_transaction_nr() == 0 )
    {
        start_transaction(transaction_name);
        _fake_sub_trans = 1;

        // This should not disrupt the numbering ..
        set_transaction_nr( get_transaction_nr() -1 );
    }


    create_new_sub_transaction_name(transaction_name, 
                                    get_action_prefix(),
                                    get_transaction_nr(),
                                    get_and_increment_sub_transaction_nr());

    // For external analysis of the response times.
    log_to_report(lr_eval_string("TimerOn {current_sub_transaction}"));
    lr_start_sub_transaction(lr_eval_string("{current_sub_transaction}"), 
                             lr_eval_string("{current_transaction}"));
}

void end_sub_transaction(char *transaction_name, int status)
{
    char *trans_name; 

    trans_name = lr_eval_string("{current_sub_transaction}");

    // Save the end status of this transaction. It won't be available after ending it.
    lr_save_int(lr_get_transaction_status(trans_name), "last_sub_transaction_status");

    lr_end_sub_transaction(trans_name, status);

    // For external analysis of the response times.
    log_to_report(lr_eval_string("TimerOff {current_sub_transaction}"));

    // if we faked an outer transaction, fake closing it.
    //
    // Note: It might be an idea to move this to start_(sub_)transaction() instead, for
    // better grouping. That may not be without it's problems though.
    if( _fake_sub_trans == 1 )
    {
        _fake_sub_trans = 0;
        end_transaction(transaction_name, status);
    }
}


//
// Generates the transaction name prefixed with a user defined action prefix and a transaction number.
// The result is saved in the "current_transaction" loadrunner parameter for use by some macro's.
//
// To use this scripts need to call setup_autonumbering() - see below - once at the start of each action and
// put the next three lines somewhere convenient - globals.h or the top of any action section will do.
// 
//#define lr_start_transaction(transaction_name) start_new_transaction_name(transaction_name, _action_prefix, _trans_nr++); \
//                                               lr_start_transaction(lr_eval_string("{current_transaction}"))
//#define lr_end_transaction(transaction_name, status) lr_end_transaction(lr_eval_string("{current_transaction}"), status)
//
void create_new_transaction_name(const char *transaction_name, const char *action_prefix, int transaction_nr)
{
    const int trans_nr_len = 2;    // eg. '01'
    int trans_name_size = strlen(action_prefix) +1 + trans_nr_len +1 + strlen(transaction_name) +1;
    char *actual_trans_name = memAlloc( trans_name_size );

    sprintf(actual_trans_name, "%s_%02d %s", action_prefix, transaction_nr, transaction_name);
    set_current_transaction_name(actual_trans_name);

    free(actual_trans_name);
}

void create_new_sub_transaction_name(const char *transaction_name, const char *action_prefix, 
                                     const int transaction_nr, int sub_transaction_nr)
{
    const int trans_nr_len = 2;    // eg. '01'
    int trans_name_size = strlen(action_prefix) +1 + (2 * (trans_nr_len +1)) + strlen(transaction_name) +1;
    char *actual_trans_name = memAlloc( trans_name_size );

    sprintf(actual_trans_name, "%s_%02d_%02d %s", action_prefix, transaction_nr, sub_transaction_nr, transaction_name);
    set_current_sub_transaction_name(actual_trans_name);

    free(actual_trans_name);
}


// Getters / Setters //

char *get_current_transaction_name()
{
    return lr_eval_string("{current_transaction}");
}

void set_current_transaction_name(char *trans_name)
{
    lr_save_string(lr_eval_string(trans_name), "current_transaction");
}


char *get_current_sub_transaction_name()
{
    return lr_eval_string("{current_sub_transaction}");
}

void set_current_sub_transaction_name(char *trans_name)
{
    lr_save_string(lr_eval_string(trans_name), "current_sub_transaction");
}


char *get_action_prefix()
{
    return _action_prefix;
}

void set_action_prefix(char *action_prefix)
{
    _action_prefix = action_prefix;
}



int get_transaction_nr()
{
    return _transaction_nr;

}

int get_and_increment_transaction_nr()
{
    return _transaction_nr++;
}

void set_transaction_nr(int trans_nr)
{
    _transaction_nr = trans_nr;
}


int get_sub_transaction_nr()
{
    return _sub_transaction_nr;
}

int get_and_increment_sub_transaction_nr()
{
    return _sub_transaction_nr++;
}

void set_sub_transaction_nr(int trans_nr)
{
    _sub_transaction_nr = trans_nr;
}



// Handy shortcuts //



//
// Shorthand for "start transaction(transaction); web_link(linkname); end_transaction(transaction)"
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
    
    tmp = memAlloc(strlen(link) + strlen("Text=") +1);
    sprintf(tmp, "Text=%s", link);
    
    trans = lr_eval_string(transaction);
    start_transaction(trans);
    web_link(link, tmp, LAST);
    end_transaction(trans, LR_AUTO);
    
    free(tmp);
}



//
// Shorthand for "start transaction(transaction); web_link(linkname); end_transaction(transaction)"
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
  char *link = lr_eval_string(LINKNAME);                                  \
    char *tmp, *trans;                                                    \
                                                                          \
    if( !(strlen(link) > 0) )                                             \
    {                                                                     \
        lr_error_message("Zero-length link name - correlation error?");   \
        lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_AUTO);                 \
        return;                                                           \
    }                                                                     \
                                                                          \
    tmp = memAlloc(strlen(link) + strlen("Text=") +1);                    \
    sprintf(tmp, "Text=%s", link);                                        \
                                                                          \
    trans = lr_eval_string(TRANSACTION);                                  \
    start_transaction(trans);                                             \
    web_link(link, tmp, LAST);                                            \
    end_transaction(trans, LR_AUTO);                                      \
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
        tmp = memAlloc( strlen(head) + strlen(step) +1);
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
    char *paramname = memAlloc( strlen(head) + strlen(stepname) +3);
    char *chancestr;
    int chance = 100; // Default
    int rnum = (rand() % 100); // random number between 0 and 99

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
