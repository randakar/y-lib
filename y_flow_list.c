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

 
/*! 
\file y_flow_list.c
\brief Business logic / clickflow support.

This file implements flow lists.
A flow list is is a lists of actions (clickflow) that the script will choose from at random, using a set of predetermined weights.
An 'action' is defined as a function that takes no arguments and returns an int - such as an action block, or a function without a specifically defined return type.
\note The weights can be percentages, hit counts, or whatever you like, as long as the total of the weights does not exceed Y_RAND_MAX.

\b Example:
\code
int browse()
{
    y_start_transaction("browse");
    web_link("...", LAST);
    y_end_transaction("", LR_AUTO);
}

buy()
{
    y_start_transaction("buy");
    web_link("...", LAST);
    y_end_transaction("", LR_AUTO);
}

// Define other actions as required.

loadtest_flow()
{
    // Define the weights
    static y_flow flow_list[] = {
        { 0, "action block for browsing the site",  browse,          3700 },   // 37.00%
        { 1, "action block for buying a product",   buy,             2100 },   // 21.00%
        { 2, "action block for checking out",       checkout,         200 },   //  2.00%
        { 3, "function for resetting the database", wipe_database,   2500 },   // 25.00%
        { 4, "end the script for no reason",        lr_abort,        1500 } }; // 15.00%
    const int flow_count = (sizeof flow_list / sizeof flow_list[0]); 

    // Choose a flow to execute
    y_flow *chosen_flow = y_choose_flow(flow_list, flow_count);

    // Change the next transaction number to make sure the numbering stays contiguous.
    // (This is optional. You can use that number for whatever you want, this is just a common use for that field.)
	y_set_next_transaction_nr( y_get_next_transaction_nr() + chosen_flow->number);

    // Execute the chosen flow.
    y_exec_flow(chosen_flow);
}
\endcode
\see y_choose_flow, y_exec_flow, y_set_next_transaction_nr, y_get_next_transaction_nr.
\author Floris Kraak
*/
#ifndef _Y_FLOW_C_
//! \cond include_protection
#define _Y_FLOW_C_
//! \endcond

#include "y_core.c"

/*! \brief Type definition of a function pointer to the flow to execute.

This matches the prototype for Action() blocks, so you can assign a action blocks to variables of this type.
*/
typedef int (y_flow_func)();


/*! \brief Descriptor for one single flow in a flow list. 

This wraps the y_flow_func function pointer in a structure that gives the flow a name and a weight, as well as an (arbitrary) number.
The chance of a specific flow getting executed is 1 in the total of the weights in the list of flows.

\see y_flow_list.c
*/
struct y_struct_flow
{
    //! A number. Not used by ylib itself, use it however you want.
    int number;
	//! The name of the flow.
    char *name;
    //! A function pointer to the code to execute.
    y_flow_func *function;
    //! The weight assigned to this piece of code, determining how likely it is to be executed.
    int weight;            
};


/*! \brief Descriptor for one single flow in a flow list. 

Hides the exact type for y_flow (y_struct_flow) from code using flow lists.
This struct wraps the y_flow_func function pointer in a structure that gives the flow a name and a weight, as well as an (arbitrary) number.
The chance of a specific flow getting executed is 1 in the total of the weights in the list of flows.
*/
typedef struct y_struct_flow y_flow;


/*! \brief Calculate the total of the weights in a given flow list.

Given a flowlist of a specified length, calculate the total weight of all flows in a flow list.
This is used by other ylib functions. Calling it directly from a script should normally not be needed.

\see y_flow_list.c, y_choose_flow()
\param [in] flow_list An array of y_flow structs, each describing a specific choice in a clickpath.
\param [in] flow_count The number of flows in the list.
\author Floris Kraak
*/
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


/*! \brief Choose a flow from a list of flows. 
\param [in] flow_list An array of y_flow structs, each describing a specific choice in a clickpath.
\param [in] flow_count The number of flows in the list.
\return a pointer to a randomly chosen flow, or NULL if somehow the weights don't add up. (flow list corrupt, flow_count incorrect, etc.)

\see y_flow_list.c, y_exec_flow()
\note The flow_count argument should exactly match the number of flows in the array or the script might blow up with MEMORY_ACCESS_VIOLATION errors.
\author Floris Kraak
*/
y_flow* y_choose_flow(y_flow flow_list[], int flow_count)
{
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

/*! \brief Execute a flow from a flow list.

Usually this is a flow as determined by y_choose_flow().
\see y_flow_list.c, y_choose_flow()
\param [in] chosen_flow The flow to execute.
\return the flow's function return value.
\author Floris Kraak
*/
int y_exec_flow(y_flow *chosen_flow)
{
    if(chosen_flow == NULL)
    {
        lr_log_message("Warning: Cannot execute NULL flow.");
    }
    else if(chosen_flow->name == NULL)
    {
        lr_log_message("Warning: Cannot execute a flow without a name!");
    }
    else if(chosen_flow->function == NULL)
    {
        lr_log_message("Warning: Cannot execute NULL flow function for flow %s", chosen_flow->name);
    }
    else
    {       
        y_flow_func *flow_function = chosen_flow->function;
        return flow_function(); // Run the flow.
    }
    return 0;
}

/*! \brief Fetch a specific item from a flow list, by name.
\param [in] flow_name The name of the flow to fetch.
\param [in] flow_list The flow list in question.
\param [in] flow_count The number of flows in the list.
\return A pointer to the requested flow.
\see y_flow_list.c, y_choose_flow(), y_exec_flow()
*/
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

/*! \brief Function implementing the "do nothing" flow.
\return Always zero.
\see y_flow_list.c, y_choose_flow(), y_exec_flow()
*/
int y_do_nothing_flow()
{
    return 0;
}

#endif // _Y_FLOW_C_
