/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2013 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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
#ifndef _FLOW_C
#define _FLOW_C

#include "y_loadrunner_utils.c"

//
// For example usage, see the bottom of this file.
//


// This is a function pointer to the flow to execute.
// Or at least, the type definition of such.
// This matches the prototype for Action() blocks.
// We can assign a action blocks to variables of this type.
typedef int (y_flow_func)();


// Wrap the function pointer in a structure that 
// gives the flow a name and a weight - how much chance it 
// has that it will be executed.
//
// The chance of a specific flow getting executed is 1 in 
// the total of the weights in the list of flows.
//
// (Note to self: Implement support for time accounting)
// (Note to self: Disregard the previous note, just port the PIF time accounting support to ylib)
struct y_struct_flow
{
    int number;
    char *name;
    y_flow_func *function;
    int weight;
};


// For clarity purposes we hide the exact type of a flow here.
typedef struct y_struct_flow y_flow;


// Given a flow list of a specified length, calculate what number all flow
// weights added together add up to.
// Do not call directly unless you have checked the flow list for sanity beforehand.
int y_calc_flow_weight_total(y_flow flow_list[], int flow_count)
{
    int i, total = 0;
    for(i=0; i < flow_count; i++)
    {
        y_flow* flow = &flow_list[i];
        total += flow->weight;
    }
    lr_log_message("y_flow: Combined total of weights is: %d", total);
    return total;
}


//
// Choose a flow from a list of flows. 
// Returns a pointer to a randomly chosen flow, or NULL if
// somehow the weights don't add up to 100 (or more).
//
// The flow_count argument should exactly match the number of flows
// in the array or the script might blow up with MEMORY_ACCESS_VIOLATION errors.
//
// Do not call directly unless you have checked the flow list for sanity beforehand.
y_flow* y_choose_flow(y_flow flow_list[], int flow_count)
{
    //y_flow **flow_list = flow_list_ptr;
    int i, lowerbound = 0;
    int cursor = 0;
    int roll = y_rand() % y_calc_flow_weight_total(flow_list, flow_count);

    lr_log_message("Roll: %d", roll);

    for(i=0; i < flow_count; i++)
    {
        y_flow *flow = &flow_list[i];
        int weight = flow->weight;
        cursor += weight;

        lr_log_message("weight cursor: %d", cursor);
        if(roll < cursor)
        {
            return flow;
        }
    }
    return NULL;
}

//
// Execute a chosen flow, provided the pointer to the flow passed in is not NULL.
// This will setup a transaction to measure the duration of the transaction as well.
//
void y_exec_flow(y_flow *chosen_flow)
{
    if(chosen_flow == NULL)
    {
        lr_log_message("Warning: Cannot execute NULL flow.");
    }
    else if(chosen_flow->name == NULL)
    {
        lr_log_message("Warning: Cannot execute flow without a name!");
    }
    else if(chosen_flow->function == NULL)
    {
        lr_log_message("Warning: Cannot execute NULL flow function for flow %s", 
            chosen_flow->name);
    }
    else
    {       
        y_flow_func *flow_function = chosen_flow->function;

        // Run the flow.
        flow_function();
    }
}

// Fetch a specific item from a flow list array, by name.
// You shouldn't need this. Or at least, not often. I've done without for years ;-)
y_flow* y_get_flow_by_name(char *flow_name, y_flow flow_list[], int flow_count)
{
    int i;
    lr_log_message("y_get_flow_by_name(%s)", flow_name);

    for(i=0; i < flow_count; i++)
    {
        y_flow *flow = &flow_list[i];

        //lr_log_message("Found name: %s", flow->name);
        if( strcmp(flow_name, flow->name) == 0 )
        {
            //lr_log_message("Match: %s", flow->name);
            return flow;
        }
    }
    lr_log_message("Name not found: %s", flow_name);
    return NULL;
}


/*
//
// Example usage
//

Scenario()
{
    // Define the weights
    static y_flow flow_list[] = {
        { 0, "action block for browsing the site",  Action_browse,   3700 },   // 37.00%
        { 1, "action block for buying a product",   Action_buy,      2100 },   // 21.00%
        { 2, "action block for checking out",       Action_checkout,  200 },   //  2.00%
        { 3, "function for resetting the database", wipe_database,   2500 },   // 25.00%
        { 4, "end the script for no reason",        lr_abort,        1500 } }; // 15.00%
    const int flow_count = (sizeof flow_list / sizeof flow_list[0]); 

    // Choose a flow to execute
    y_flow *chosen_flow = y_choose_flow(flow_list, flow_count);

    // Execute the chosen flow.
    y_exec_flow(chosen_flow);
}    
*/
// --------------------------------------------------------------------------------------------------
#endif // _FLOW_C
