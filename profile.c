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
#ifndef _PROFILE_C
#define _PROFILE_C

#include "transaction.c"

//
// Todo: Write documentation
// For example usage, see the bottom of this file.
//

// This is a function pointer to the profile to execute.
// Or at least, the type definition of such.
// This matches the prototype for Action() blocks for a reason, btw.
typedef int (y_profile_func)();

// Wrap the function pointer in a structure that 
// gives the profile a name and a weight - how much chance it 
// has that it will be executed.
//
// Note that all chances combined in a list of profiles should 
// add up to exactly 100.
//
// If they don't do realize the random chooser will pick a number between
// 0 and 99 and use that to pick something. If it picks 99 and your profile list
// goes to 98 the profile chooser just does nothing.
//
// Also if I get around to adding time accounting to this not adding up to 100%
// might be bad. If you really want to have a profile that does nothing, add
// it to the list. ;-)
//
// (Note to self: Implement time accounting)
struct y_struct_profile
{
    int number; // position in the profile list, if applicable.
    char *name;
    y_profile_func *profileFunc;
    int chance;
};

// For clarity purposes we hide the exact type of a profile with 
// a typedef
typedef struct y_struct_profile y_profile;

//
// Choose a profile from a list of profiles. 
// Returns a pointer to a randomly chosen profile, or NULL if
// somehow the chances don't add up to 100 (or more).
//
// The profile_count argument should exactly match the number of profiles
// in the array or the script might blow up with MEMORY_ACCESS_VIOLATION errors.
//
y_profile *y_choose_profile(y_profile *profile_list[], int profile_count)
{
    //y_profile **profile_list = profile_list_ptr;
    int i, lowerbound = 0;
    int cursor = 0;
    int roll = rand() % 100;

    lr_log_message("Roll: %d", roll);

    for(i=0; i < profile_count; i++)
    {
        y_profile *prof = profile_list[i];
        int profileChance = prof->chance;
        cursor += profileChance;

        lr_log_message("Chance cursor: %d", cursor);
        if(roll < cursor)
        {
            return prof;
        }
    }
    return NULL;
}

//
// Execute a chosen profile, provided the pointer to the profile passed in is not NULL.
// This will setup a transaction to measure the duration of the transaction as well.
//
void y_exec_profile(y_profile *chosenProfile)
{
    if(chosenProfile == NULL)
    {
        lr_log_message("Warning: Cannot execute NULL profile.");
        return;
    }
    else if(chosenProfile->name == NULL)
    {
        lr_log_message("Warning: Cannot execute profile without a name!");
        return;
    }
    else if(chosenProfile->profileFunc == NULL)
    {
        lr_log_message("Warning: Cannot execute NULL profile function for profile %s", 
            chosenProfile->name);
    	return;
    }
    else
    {   	
        y_profile_func *profile_function = chosenProfile->profileFunc;
        char *savedTransactionName;

        // Start a "profile" transaction to enable measuring how long
        // a profile needs to execute fully.
        //
        // Profile transaction names need a bit of magic as they are 
        // not quite like regular transactions. We use the profile number
        // instead of the transaction number - profile transactions are 
        // singular anyway.
        y_start_action_block("__Profiel_");
        y_set_transaction_nr(chosenProfile->number);
        y_start_transaction(chosenProfile->name);

        // The current transaction name is likely to get overwritten
        // as the profile starts running it's own set of transactions.
        // Save it here.
        savedTransactionName = get_current_transaction_name();

        // Run the profile.
        profile_function();

        // Restore the saved transaction name.
        y_set_current_transaction_name(savedTransactionName);

        // End the transaction
        y_end_transaction(chosenProfile->name, LR_AUTO);
        y_end_action_block();
    }
}

/*

//
// Example usage
//

Profiel()
{
    // const int profile_count = 5; // doesn't work, grr.
    #define PROFILE_COUNT 5
    
    // Define the weights
    static y_profile fo_pwm  = { "FO_PWM",  Profiel_FO_PWM,  37 };
    static y_profile fo_pam  = { "FO_PAM",  Profiel_FO_PAM,  21 };
    static y_profile fo_sead = { "FO_SEAD", Profiel_FO_SEAD,  2 };
    static y_profile mo      = { "MO",      Profiel_MO,      25 };
    static y_profile staff   = { "STAFF",   Profiel_Staff,   15 };
    static y_profile* profile_list[PROFILE_COUNT];

    profile_list[0] = &fo_pwm;
    profile_list[1] = &fo_pam;
    profile_list[2] = &fo_sead;
    profile_list[3] = &mo;
    profile_list[4] = &staff;

    // Choose a profile to execute
    y_profile *chosenProfile = y_choose_profile(profile_list, PROFILE_COUNT);
    
    // Execute the chosen profile.
    y_exec_profile(chosenProfile);
    
    return 0;
}
*/