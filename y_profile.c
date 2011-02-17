/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2011 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

#include "y_loadrunner_utils.c"

//
// Todo: Write documentation
// For example usage, see the bottom of this file.
//


// This is a function pointer to the profile to execute.
// Or at least, the type definition of such.
// This matches the prototype for Action() blocks.
// We can assign a action blocks to variables of this type.
typedef int (y_profile_func)();


// Wrap the function pointer in a structure that 
// gives the profile a name and a weight - how much chance it 
// has that it will be executed.
//
// The chance of a specific profile getting executed is 1 in 
// the total of the weights in the list of profiles.
//
// (Note to self: Implement support for time accounting)
struct y_struct_profile
{
    int number;
    char *name;
    y_profile_func *profileFunc;
    int chance;
};


// For clarity purposes we hide the exact type of a profile here.
typedef struct y_struct_profile y_profile;


// Given a profile list of a specified length, calculate what number all profile
// chances added together add up to.
// Do not call directly unless you have checked the profile list for sanity beforehand.
int y_calculate_max_chance(y_profile profile_list[], int profile_count)
{
    int i, total = 0;
    for(i=0; i < profile_count; i++)
    {
        y_profile* prof = &profile_list[i];
        total += prof->chance;
    }
    lr_log_message("y_profile: Combined total of chances is: %d", total);
    return total;
}


//
// Choose a profile from a list of profiles. 
// Returns a pointer to a randomly chosen profile, or NULL if
// somehow the chances don't add up to 100 (or more).
//
// The profile_count argument should exactly match the number of profiles
// in the array or the script might blow up with MEMORY_ACCESS_VIOLATION errors.
//
// Do not call directly unless you have checked the profile list for sanity beforehand.
y_profile* y_choose_profile(y_profile profile_list[], int profile_count)
{
    //y_profile **profile_list = profile_list_ptr;
    int i, lowerbound = 0;
    int cursor = 0;
    int roll = y_rand() % y_calculate_max_chance(profile_list, profile_count);

    lr_log_message("Roll: %d", roll);

    for(i=0; i < profile_count; i++)
    {
        y_profile *prof = &profile_list[i];
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
    }
    else if(chosenProfile->name == NULL)
    {
        lr_log_message("Warning: Cannot execute profile without a name!");
    }
    else if(chosenProfile->profileFunc == NULL)
    {
        lr_log_message("Warning: Cannot execute NULL profile function for profile %s", 
            chosenProfile->name);
    }
    else
    {       
        y_profile_func *profile_function = chosenProfile->profileFunc;

        // Run the profile.
        profile_function();
    }
}

/*
//
// Example usage
//

Profile()
{
	// Define the weights
    static y_profile profile_list[] = {
		{ 0, "FO_PWM",  Profile_FO_PWM,  3700 },   // 37.00%
		{ 1, "FO_PAM",  Profile_FO_PAM,  2100 },   // 21.00%
		{ 2, "FO_SEAD", Profile_FO_SEAD,  200 },   //  2.00%
		{ 3, "MO",      Profile_MO,      2500 },   // 25.00%
	    { 4, "STAFF",   Profile_Staff,   1500 } }; // 15.00%
	const int profile_count = (sizeof profile_list / sizeof profile_list[0]); 

	// Choose a profile to execute
	y_profile *chosenProfile = y_choose_profile(profile_list, profile_count);

	// Execute the chosen profile.
	y_exec_profile(chosenProfile);
}    
*/
