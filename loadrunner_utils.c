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
#ifndef _LOADRUNNER_UTILS_C
#define _LOADRUNNER_UTILS_C

#include "string.c"
// #include "web_submit_data_buffer.h"


//
// This file contains loadrunner specific helper funtions.
//
// See also: string.c, logging.c, transaction.c, profile.c - all of which
// contain helper functions (grown out of this file) which cover a specific topic.
//



//////////////////////////// Param array functions. ////////////////////////////


//
// Retrieve the number of saved elements for the parameter defined in *pArrayName
// 
// Superseded by the LR 9 function lr_paramarray_len() but kept for compatibility reasons.
//
int Array_Count( const char *pArrayName )
{
	// -- Loadrunner 9 and upwards
    // return lr_paramarr_len(pArrayName);

	// -- Loadrunner 8 and below
	int result;
	char *tmp = memAlloc( strlen(pArrayName) + strlen("_count") +4 );  // 4 characters added: { _ } \0
	
	sprintf(tmp , "{%s_count}" , pArrayName );
	result = atoi(lr_eval_string(tmp));
	free(tmp);
	return result;
}



// Get a specific element from a parameter list.
//
// Note: If the data inside the parameter contains embedded null (\x00) characters you may have an issue
// processing the return value. See also Array_Get_No_Zeroes().
//
// Superseded by the LR 9 function lr_paramarr_idx() but kept for compatibility reasons.
//
char *Array_Get( const char *pArray, const int pIndex )
{
	//-- Loadrunner 9 and upwards
	// return lr_paramarr_idx(pArray, pIndex);

	//-- Loadrunner 8 and below
	int size = Array_Count( pArray );
	char *tmp;
	char *result;
	
	//lr_log_message("Array_Get(%s,%d)", pArray, pIndex );
	
	if ( (pIndex > size) || (pIndex < 1) )
	{
		lr_error_message("Index out of bounds");
		lr_abort();
	}
	
	// This breaks if the index number is 10 billion or more  ;-)
	// I presume we have run out of memory by then ..
	tmp = memAlloc( strlen(pArray) +10 +4 ); // 10 characters for the index, 4 characters added: { _ } \0
	sprintf( tmp , "{%s_%d}" , pArray , pIndex );
	result = lr_eval_string(tmp);
	free (tmp);	
	return result;
}


//
// As Array_Get(), but it filters embedded zeroes from the input, replacing them with 
// a simple space: ' '.
// It's not ideal, but better than having your script break on this type of idiocy.
//
//
// Note: The output of this needs to be freed using lr_eval_string_ext_free();
// See also the loadrunner documentation regarding lr_eval_string_ext();
//
char *Array_Get_No_Zeroes( const char *pArray, const int pIndex )
{
	int size = Array_Count( pArray );
	char *tmp;
	char *result;
	unsigned long resultLen;
	size_t resultStrlen;
	
	//lr_log_message("Array_Get_No_Zeroes(%s,%d)", pArray, pIndex );
	
	if ( (pIndex > size) || (pIndex < 1) )
	{
		lr_error_message("Index out of bounds");
		lr_abort();
	}
	
	// This breaks if the index number is 10^12 or more  ;-)
	// I presume we have run out of memory by then ..
	tmp = memAlloc( strlen(pArray) +12 +4 ); // 12 characters for the index with 4 characters added: { _ } \0
	sprintf( tmp , "{%s_%d}" , pArray , pIndex );
	lr_eval_string_ext(tmp, strlen(tmp), &result, &resultLen, 0, 0, -1);
	free (tmp);	


    // Replace NULL bytes (\x00) in the input with something else..
	for( resultStrlen = strlen(result);
		 resultStrlen < resultLen;
		 resultStrlen = strlen(result))
	{
		result[resultStrlen] = ' ';
	}

	return result;
}

//
// Save a string value in array pArray at index pIndex.
// This does not update the count value (size) of the array.
// 
Array_Save( const char* value, const char* pArray, const int pIndex )
{
	int len = strlen(pArray) +3;
	char *result;
	int power = pIndex;

	while(power = (power / 10))
	{
		len++;
	}

	result = memAlloc(len);
	sprintf(result, "%s_%d", pArray, pIndex);
	lr_save_string(value, result);
	free(result);
}

//
// Update the array count (size).
//
// 
Array_Save_Count( const int count, const char *pArray )
{
	const char *ctpf = "_count";
	int len = strlen(pArray) + strlen(ctpf) + 1;
	char *result = memAlloc(len);
	sprintf(result, "%s%s", pArray, ctpf);
	lr_save_int(count, result);
	free(result);
}


//
// Add an element to an array at the end of the list.
// Note: Do not use this in a loop as it will update the count every time it's called.
// For one-offs this is fine though.
//
Array_Add( const char* value, const char* pArray )
{
	int size = Array_Count(pArray);
	// hmm - should we check if the array does not exist?
	// Maybe not - there are cases where we care, and there are cases where we don't.
	size++;
	Array_Save(value, pArray, size);
	Array_Save_Count(size, pArray);
}

Array_Add_Array( const char *pArrayFirst, const char *pArraySecond, const char *resultArray)
{
	int size_first = Array_Count(pArrayFirst);
	int size_second = Array_Count(pArraySecond);
	int size_total = size_first + size_second;
	int i = 1;
	int j = 1;

	lr_log_message("Array_AddArray(%s, %s, %s)", pArrayFirst, pArraySecond, resultArray);
	lr_log_message("size_total = %d, i = %d", size_total, i);

	for(i=1; i <= size_total; i++)
	{
		char *value;
		lr_log_message("Iteration %i, j=%d", i, j);

		if( i <= size_first )
		{
			value = Array_Get_No_Zeroes(pArrayFirst,i);
		}
		else
		{
			value = Array_Get_No_Zeroes(pArraySecond,j);
			j++;
		}

		Array_Save(value, resultArray, i);
		lr_eval_string_ext_free(&value);
	}
	Array_Save_Count(size_total, resultArray);
}



// Get a random element from a parameter list
//
// Superseded by the LR 9 function lr_paramarr_random() but kept for compatibility reasons.
//
// Also deprecated. See Array_Get_Random_No_Zeroes() as for why.

char *Array_Get_Random( const char *pArray )
{
	//return lr_paramarr_random(pArray);

	// -- Loadrunner 8 and below
	int index;
	int count = Array_Count( pArray );
	
	//lr_log_message("Array_Get_Random(%s)", pArray);
	
	if( count < 1 )
	{
		lr_log_message("No elements found in parameter array!");
		return NULL;
	}
	
	index = (rand() % count) +1;
	return Array_Get(pArray, index);
}


char *Array_Get_Random_No_Zeroes( const char *pArray )
{
	//return lr_paramarr_random(pArray); 
	// I don't think the LR function is actually an good substitute in this case..

	// -- Loadrunner 8 and below
	int index;
	int count = Array_Count( pArray );
	
	//lr_log_message("Array_Get_Random(%s)", pArray);
	
	if( count < 1 )
	{
		lr_log_message("No elements found in parameter array!");
		return NULL;
	}
	
	index = (rand() % count) +1;
	return Array_Get_No_Zeroes(pArray, index);
}


//
// Choose an element at random from a saved parameter list and store it in
// a parameter with the same name.
//
// Returns 1 on success, 0 when there is an issue of some kind
// such as "not enough elements to choose from."
//
int Array_Pick_Random( const char *pArray )
{
    if(Array_Count(pArray))
    {
        char *result = Array_Get_Random_No_Zeroes(pArray);
        lr_save_string(result, pArray );
        lr_eval_string_ext_free(&result);
        return 1;
    }
    else
    {
        lr_save_string("", pArray);
        //lr_output_message("Array_Pick_Random(): Unknown parameter list");
        return 0;
    }
}


// Dump the contents of a list of saved parameters to std out (the run log)
//
Dump_Array( const char *pArrayName )
{
	int i;
	int count;

	for ( i=1 ; i <= Array_Count(pArrayName); i++ )
	{
		char *msg = Array_Get_No_Zeroes(pArrayName, i);
		lr_output_message("%s" , msg);
		lr_eval_string_ext_free(&msg);
	}
}


//
// Save the contents of a const char * variable to a parameter array.
// As if using web_reg_save_param() with "Ord=All" as an option, but
// for strings.
// This is especially useful when there is a need to save lists of parameters
// from sections of the application output (rather than the entire thing).
// Think: Drop-down boxes.
// 
// Arguments: 
//   search: The string to be searched from.
//   LB: Left Boundary (match)
//   RB: Right Boundary (match)
//   pArrayName: Name of resulting parameter array.
// 
// Note: This does not understand the entire web_reg_save_param() syntax,
// notably left/right boundaries are simple strings rather than strings-with-text-flags..
//
Array_Save_Param_List(const char *sourceParam, const char *LB, const char *RB, const char *destArrayParam)
{
	int i = 0;
	char *source = getParameter(sourceParam);
	int buflen = strlen(source)+1;
	char *buffer = memAlloc(buflen);
	char *next = buffer;
	memcpy(buffer, source, buflen);

	while( next = (char *)strstr(next, LB) )
	{
		int end = strstr(next, RB);
		if(!end) 
			break;
		buffer[end - (int)buffer] = '\0';

		i++;
		Array_Save(next+strlen(LB), destArrayParam, i);
		next = (char *)(end + strlen(RB));
	}
	free(buffer);
	Array_Save_Count(i, destArrayParam);
}

// Search array 'pArrayName' for string 'search' and build a new result array
// containing only parameters containing the string.
//
// Just call it 'grep' :P
//
Array_Grep( const char *pArrayName, const char *search, const char *resultArrayName)
{
	int i, j = 1;
	char *item;
	int size = Array_Count(pArrayName);

	//lr_log_message("Array_Grep(%s, %s, %s)", pArrayName, search, resultArrayName);

	for( i=1; i <= size; i++)
	{
		item = Array_Get_No_Zeroes(pArrayName, i);
		if( strstr(item, search) )
		{
			Array_Save(item, resultArrayName, j++);
		}
		lr_eval_string_ext_free(&item);
	}

	Array_Save_Count(j-1, resultArrayName);
}

// Search array 'pArrayName' for string 'search' and build a new result array
// containing only parameters that do NOT contain the string.
//
// Reverse grep, in other words.
//
Array_Filter( const char *pArrayName, const char *search, const char *resultArrayName)
{
	int i, j = 1;
	char *item;
	int size = Array_Count(pArrayName);

	//lr_log_message("Array_Filter(%s, %s, %s)", pArrayName, search, resultArrayName);

	for( i=1; i <= size; i++)
	{
		item = Array_Get_No_Zeroes(pArrayName, i); // Some pages contain \x00 in the input. Ugh.
		if( strstr(item, search) == NULL )
		{
			Array_Save(item, resultArrayName, j++);
		}
		lr_eval_string_ext_free(&item);
	}

	Array_Save_Count(j-1, resultArrayName);
}

//
// Merge two arrays into a single array. They have to be of the same length.
//
// The resulting items are each item from the right array appended to the item with
// the same index in the left array, with an optional glue separator in the middle for
// convenient re-splitting later.
//
// This thing is mostly created to facilitate situations where you have a list of links
// but the titles of those links are really hard to capture in the same parameter. With this
// function you can just look for them separately, merge the results, then pick one link based on
// the title or something and then split the resulting link back for further processing ..
//
int Array_Merge( const char *pArrayNameLeft, const char *pArrayNameRight, const char *separator, const char *resultArray)
{
	int i = 1;
	char *param;
	int length = Array_Count(pArrayNameLeft);

	if( length != Array_Count(pArrayNameRight) )
	{
		// If the sizes aren't the same there's a good chance numbers won't line up on both sides.
		// We definitely don't want to end up with records merged that don't actually correspond to each other!
		lr_error_message("Unable to merge arrays %s and %s - sizes unequal!", pArrayNameLeft, pArrayNameRight);
		return 0;
	}

	//lr_save_string(separator, "_sep");

	for( i=1; i <= length; i++)
	{
		char *left = Array_Get_No_Zeroes(pArrayNameLeft, i);
		char *right = Array_Get_No_Zeroes(pArrayNameRight, i);
		char *result = memAlloc(strlen(left)+strlen(separator)+strlen(right)+1);

		sprintf(result, "%s%s%s", left, separator, right);
		lr_eval_string_ext_free(&left);
		lr_eval_string_ext_free(&right);
		Array_Save(result, resultArray, i);
		free(result);

		//lr_save_string( left, "_left");
		//lr_save_string( right, "_right");
		//Array_Save( lr_eval_string("{_left}{_sep}{_right}") , resultArray, i);
	}

	//lr_save_int(i-1, lr_eval_string("{_resultArray}_count") );
	Array_Save_Count(i-1, resultArray);

	return 1;
}

void Array_Split(const char *pInputArray, const char *seperator, const char *pArrayNameLeft, const char *pArrayNameRight)
{
	int i = 1;
	int size = Array_Count(pInputArray);

	//lr_log_message("Array_Filter(%s, %s, %s)", pArrayName, search, resultArrayName);

	for( i=1; i <= size; i++)
	{
		char *item = Array_Get_No_Zeroes(pInputArray, i);
		int len = strlen(item);
		char *left = memAlloc(len);
		char *right = memAlloc(len);

		left[0] = '\0';
		right[0] = '\0';

		// This is where the magic happens - see string.h
		split_str(item, seperator, left, right);
		lr_eval_string_ext_free(&item);

		Array_Save(left, pArrayNameLeft, i);
		free(left);
		Array_Save(right, pArrayNameRight, i);
		free(right);
	}

	Array_Save_Count(i-1, pArrayNameLeft);
	Array_Save_Count(i-1, pArrayNameRight);
}

//// Time/date/stamp functions

typedef long time_t;

time_t timestamp()
{
	return time(NULL);
}


//// Random number generator control ////

//
// Roll a random number between 0 and randMax, and tell us if that number falls 
// between the lower and upper bounds or not.
//
// Zero = No.
// One = Yes.
// Negative return values are errors and generally mean that arguments make no sense
//
// Fixme: Think up a way to automatically initialize the random number generator instead
// of just relying on the user to put it in vuser_init().
// (Hint to the user: Please call srand() with something sufficiently random in vuser_init().)
//
int randBetween(int lowerbound, int upperbound, int randMax)
{
	int roll;

	if( 
		(0 > lowerbound ) ||
		(lowerbound > upperbound) ||
		(upperbound > randMax) ||
		(randMax <= 0)
	  )
	{
		lr_error_message("randBetween called with nonsensical arguments: ( 0 <= %d < %d <= %d ) == FALSE",
						lowerbound, upperbound, randMax);
		return -1;
	}

	roll = rand() % randMax;
	if( (roll >= lowerbound) && (roll < upperbound) )
	{
		return 1;
	}

	return 0;
}



////// Controller / Scenario related //////


// This will fetch attributes from the vUser's command line (as set in the scenario!)
// and store them in a parameter of the same name.
// See also saveAttributeToParameter()
//
// Arguments:
//   - param: the name of the attribute to get AND
//            the name of the parameter to store the retrieved value in.
//
// @author: Floris Kraak
//
saveAttribute( char* param )
{
   saveAttributeToParameter( param, param );
}


// This will fetch attributes from the vUser's command line (as set in the scenario!)
// and store them in a parameter.
// See also saveAttribute()
//
// These attributes are defined in the details of the vUser group
// They take the form "-attributename value"
//
// Arguments:
//   - attrib: The name of the attribute to fetch
//   - param : The name of the parameter to store the attribute in.
//
// @author: Floris Kraak
//
saveAttributeToParameter( char* attrib, char* param )
{
   char *tmp;

   if( (tmp = lr_get_attrib_string(attrib)) != NULL )
   {
      lr_save_string(tmp, param);
   }
}



#endif // _LOADRUNNER_UTILS_C

