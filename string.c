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
#ifndef _STRING_C
#define _STRING_C

extern char *strupr ( char *string ); 

//
// Allocates a block of memory for a string
// Adds some simple checks to catch common errors.
//
char *memAlloc(int size)
{
	char *buff;
	int mem = size * sizeof(char);
	
	if(mem < 0)
	{
		lr_error_message("Requested negative amounts (%d) of memory! Bailing out ..", mem);
		return NULL;
	}
	//lr_output_message("Dynamic allocation of %d bytes of memory", mem);
	
	if ((buff = (char *)malloc(mem)) == NULL) 
	{
		// Fixme: implement some generic error handling facility to send this stuff to.
		// Just for the record though: If this happens you're pretty much screwed anyway.
		lr_error_message("Insufficient memory available, requested %d", mem);
	}
	return buff;
}

//
// Allocates a character array and initializes all elements to zero
// Adds some simple checks to catch common errors.
//
char *arrayAlloc(int length, int bytesPerChar)
{
	char *buff;
	int size = bytesPerChar ; // * sizeof(char);
	int mem = length * size;

	if(mem < 0)
	{
		lr_error_message("Requested negative amounts (%d) of memory! Bailing out ..", mem);
		return NULL;
	}
	//lr_output_message("Dynamic allocation of %d bytes of memory", mem);
	
	if ((buff = (char *)calloc(length, size)) == NULL)
	{
		// Fixme: implement some generic error handling facility to send this stuff to.
		lr_error_message("Insufficient memory available, requested %d", mem);
	}
	return buff;
}

//
// Convert a single character 0-9 to an int
//
// @author Floris Kraak
//
int getIntFromChar(char character)
{
	char tmp[2];
	tmp[0] = character;
	tmp[1] = '\0';
	return atoi(tmp);	
}

// Get the content of the parameter named "paramName" and return it as a char *
//
// Todo: Make a derivative function getParameterNoZeroes() which uses lr_eval_string_ext()
// and replaces all \x00 characters in the result with something else, like Array_Get_No_Zeroes() does.
// Then every user of this function should start using it.
// Alternative: Make a function that cleans parameters of embedded null bytes and tell users to use that
// before doing anything else with possible tainted values.
// I think I like that one better to be honest. For the Array_ subset it's not that bad given the better
// memory management but for this one .. muh.
//
// @author Floris Kraak
//
char* getParameter(const char* paramName)
{
   char *parameter;
   char *tmp = memAlloc( strlen(paramName) +3 ); 
                         // parameter name + "{}" + '\0' (end of string)
   sprintf( tmp, "{%s}", paramName );   
   parameter = lr_eval_string(tmp);
   free(tmp);
   
   return parameter;
}

//
// Clean a parameter by removing any embedded \x00 (null) characters from it.
// Any such characters are replaced with ' '.
// Since this changes existing parameters be careful what types of parameters 
// you use this on.
//
void cleanseParameter(const char* paramName)
{
   char *tmp = memAlloc( strlen(paramName) +3 ); 
                         // parameter name + "{}" + '\0' (end of string)
   char *result;
   unsigned long resultLen;
   size_t resultStrlen;

   //lr_log_message("cleanseParameter(%s)", paramName );

   // Get the contents of the parameter using lr_eval_string_ext() - we can't use the
   // regular version if we expect to find \x00 in there.
   sprintf( tmp, "{%s}", paramName );
   lr_eval_string_ext(tmp, strlen(tmp), &result, &resultLen, 0, 0, -1);
   free (tmp);

    // Replace NULL bytes (\x00) in the input with something else..
   for( resultStrlen = strlen(result);
		resultStrlen < resultLen;
		resultStrlen = strlen(result))
   {
	   result[resultStrlen] = ' ';
   }

   lr_eval_string_ext_free(&result);

   // Put the result back into the original parameter.
   lr_save_string(result, paramName);
   return;
}

//
// Convert a parameter to UPPERCASE.
//
// @author Floris Kraak
//
upperCaseParameter(const char* paramName)
{
   char *result = getParameter(paramName);
   strupper(result);
   lr_save_string(result, paramName);
   /*
   char *buffer = getParameter(paramName);
   int i;

   for(i=0; i < strlen(buffer); i++)
   {
      buffer[i] = toupper(buffer[i]);
   }

   lr_save_string(buffer, paramName);
   */
}


// Cut the end off a string based on a search string,
// and store the resulting head in a parameter.
//
// @author Floris Kraak
//
head( const char *originalParameter, const char *search, const char *resultParameter )
{
   char *buffer;
   char *original = getParameter(originalParameter);
   char *posPtr = (char *)strstr(original, search);
   int pos = (int)(posPtr - original);

   //lr_log_message("head: original=%s, search=%s, resultParam=%s", original, search, resultParam);

   if( posPtr == NULL )
   {
      lr_save_string(original, resultParameter);
      return;
   }
   //lr_log_message("pos = %d", pos);
   
   // Copy the original to a temporary buffer
   buffer = memAlloc(strlen(original)+1);
   strcpy(buffer, original);
   buffer[pos] = '\0';                             // make the cut
   lr_save_string(buffer, resultParameter);        // save the result
   free(buffer);
}


// Cut the head off a string based on a search string,
// leaving only the tail. Store the result in a parameter.
//
// @author Floris Kraak
//
tail( const char *originalParameter, const char *search, const char *resultParameter)
{
	char *original = getParameter(originalParameter);

	char *posPtr = (char *)strstr(original, search);
	int pos = (int)(posPtr - original);

	//lr_log_message("tail: original=%s, search=%s, resultParam=%s", original, search, resultParameter);

	if( posPtr == NULL )
	{
		lr_save_string(original, resultParameter);
		return;
	}

	//lr_log_message("pos = %d", pos);

	posPtr = posPtr + strlen(search);
	lr_save_string(posPtr, resultParameter);
}

//
// Same as tail(), but don't stop at the first match, rather use the last match instead.
// 
// @author Floris Kraak
//
tail_tail( const char *originalParameter, const char *search, const char *resultParameter)
{
	char *result = getParameter(originalParameter);
	char *posPtr;
	//int pos;

	if( search == NULL || strlen(search) == 0 )
	{
		lr_save_string(result, resultParameter);
		//lr_log_message("Warning: Empty search parameter passed to tail_tail()");
		return;
	}

	//lr_log_message("tail_tail: original=%s, search=%s, resultParam=%s", original, search, resultParameter);

	do 
	{
		posPtr = (char *)strstr(result, search);
		//pos = (int)(posPtr - result);
		//lr_log_message("pos = %d", pos);

		// not found, save what we have as the result.
		if( posPtr == NULL )
		{
			lr_save_string(result, resultParameter);
			return;
		}
		// found, update the result pointer and go find more..
		result = posPtr + strlen(search);
	} 
	while(1);
}


//
// Split a string in two based on a search parameter.
//
// Note: Unlike the others this one does not use parameter, but raw char pointers instead.
// This mostly to accomodate the primary user - Array_Split();
// Both output buffers need to be pre-allocated! 
//
// For the parameter version use split().
//
// @author Floris Kraak
//
split_str( const char *original, const char *seperator, char *left, char *right)
{
	char *buffer;
	char *posPtr = (char *)strstr(original, seperator);
	int pos = (int)(posPtr - original);;

	//lr_log_message("split_str: original=%s, search=%s", original, search);

	if( posPtr == NULL )
	{
		// Copy the original to the left hand output buffer.
		strcpy(left, original);
		return;
	}
	//lr_log_message("pos = %d", pos);

	// Copy the left hand using pos bytes from the original
	strncpy(left, original, pos);
	left[pos] = '\0'; // make the cut by putting a null character at the end.

	// Copy the right hand side starting from the position 
	// just after the found string.
	strcpy(right, posPtr+strlen(seperator));
}

//
// Split a string in two based on a seperating string.
//
// @author Floris Kraak
//
split( const char *originalParameter, const char *seperator, const char *leftParameter, const char *rightParameter)
{
    char *item = getParameter(originalParameter);
	int len = strlen(item);

	if( len < strlen(seperator) )
	{
		// Since the seperator doesn't even fit in the item, searching is pointless.
		// Store the original in the left hand parameter, the right hand is empty.
		lr_save_string(originalParameter, leftParameter);
		lr_save_string("", rightParameter);
		return;

	}
	else
	{

		// Left hand side
		// If the seperator isn't found the full original string gets stored here.
		// Don't forget the 0 byte at the end though..
		char *left = memAlloc(len+1);

		// Right hand side
		// If the seperator gets found in position 1 the remainder of the
		// original string gets stored here (and left gets a zero-length string).
		char *right = memAlloc(len-strlen(seperator)+1);

		// Start off with zero-length strings. We can safely assume
		// both variables contain garbage when freshly allocated.
		left[0] = '\0';
		right[0] = '\0';

		// This is where the magic happens.
		split_str(item, seperator, left, right);

		// Store the results in parameters.
		lr_save_string(left, leftParameter);
		free(left);
		lr_save_string(right, rightParameter);
		free(right);
	}
}

//
// Remove leading and trailing whitespace from a parameter
// Does not support Unicode so use with care
//
// @author: Floris Kraak
//
chop( const char* parameter )
{
	char *result;
	int i = 0;
	char character;
	
	//lr_output_message( "chop(%s)", parameter);	
	result = getParameter(parameter);

	// chop leading whitespace
	character = result[i];
	while( (character == ' ') || (character == '\r') || (character == '\n') || (character == '\t') )
	{
		character = result[++i];
	}
	result += i;
	
	//lr_output_message("result after removal of leading whitespace: %s", result);
	
	// chop trailing whitespace
	i = strlen(result)-1;	
	character = result[i];
	while( (i >= 0) &&
	       (  (character == ' ' ) ||
		      (character == '\r') ||
			  (character == '\n') ||
			  (character == '\t') ) )
	{
		character = result[--i];
	}
	result[i+1] = '\0';
	
	lr_save_string(result, parameter);
}

// Search and replace.
// Find 'search' in 'parameter' and replace it with 'replace'.
// Replaces the originally passed-in parameter with the new one.
//
// Note: This one has a built-in search/replace limit when
// search > replace. It won't do it more than 'limit' times.
//
// @author Floris Kraak
//
replace( const char *parameter, const char *search, const char *replace)
{
   int slen, rlen, plen;      // lengte van search, replace, en basis string
   int i = 0;                 // aantal replacements
   int limit = 1000;          // replacement limiet als replace > search

   char *c;                   // punt waar change moet beginnen
   char *cend;                // einde van de change = c+slen
   char *last;                // de \0 slotbyte van input string
   char *buffer;              // buffer voor bewerkingen
   char *string;              // originele string waar we in zoeken

   if (!search || !replace)      return;   // ongeldige search of replace
   if (!strcmp(search, replace)) return;   // search == replace: geen wijziging

   slen = strlen (search);          // de lengte van search
   rlen = strlen (replace);         // de lengte van replace

   string = getParameter(parameter); // <-- memory allocated by loadrunner, too small if replace > search
   plen = strlen(string);

   //lr_log_message("replace(%s, %s, %s) - slen %d, rlen %d, plen %d", parameter, search, replace, slen, rlen, plen);
	
   if ( rlen > slen)
   {
      // Reserve memory for -limit- replacements.
      buffer = memAlloc( plen + ((rlen-slen) * limit) );
      sprintf( buffer, "%s", string );
   }
   else
   {
      // No need to reserve memory
      buffer = string;
   }

   last = buffer + strlen(buffer) +1;      // het einde van de string, de null byte
   c = buffer;                             // initialiseer c - buffer moet intact blijven

   while (c = (char*) strstr(c, search))   // doorzoek de search string vanaf waar je gebleven bent
   {
      //lr_output_message("c:%s", c);
 
      i++;
      if (slen != rlen)                    // zijn search en replace van verschillende lengte?
      {
         if( i >= limit )
         {
            lr_log_message("Unable to handle more than %d search-and-replaces, apologies for the inconvenience.", limit);
            break;
         }
         cend = c+slen;                        // cend is de plaats van waaraf de string moet opschuiven
         //lr_output_message("memmove(dest=%d, src=%d, size=%d)", (long)(c+rlen), (long)(cend), (long)(last-cend) );
         memmove (c+rlen , cend, last-cend);   // verplaats de rest van de string - als de geheugenberekening niet klopt gaat dit stuk!
         //lr_output_message("memmove completed, result = %s", c);
         last = last - slen + rlen;            // en bereken het nieuwe eindpunt
      }
      memcpy (c, replace, rlen);         // voeg replace toe over search heen
   }
   lr_save_string(buffer, parameter);

   if( rlen > slen )
   {
      free(buffer);
   }
}


// Lightweight alternative to the replace() function.
// Remove all occorrances of 'removeMe' in the parameter named 'paramName'
// stores the result in the original parameter
removeStringFromParameter(const char* removeMe, const char* paramName)
{
   char* parameter;
   char* tmp;
   int removePtr;

   //lr_log_message("removeStringFromParameter( remove:%s, parameter:%s )", removeMe, paramName);

   // fetch the contents of the parameter to change
   parameter = getParameter(paramName);

   // while we find occurrances of the string we're looking for
   while ( removePtr = strstr( parameter, removeMe ) )
   {
      // calculate where data ends that we want to remove
      tmp = (char *)removePtr + strlen(removeMe);

      // copy the characters between aforementioned point and end-of-string to the place
      // where we found our offending content.
      strcpy( removePtr, tmp);
   }

   // store it in the original parameter
   lr_save_string( parameter, paramName );
}


// Usage: randomStringBuffer("parameterNameWhichGetsTheRandomValue", minimumlength, maximumlength);
//
// ex. randomString("randomFeedback", 100, 200); will fill a parameter with between 100 and 200 characters.
// to use the random value, use the parameter name provided.
//
randomStringBuffer(const char *parameter, int minimumLength, int maximumLength)
{
   const char characterSet[] = { 
                     'a','b','c','d','e','f','g','h','i','j','k','l','m',
                     'n','o','p','q','r','s','t','u','v','w','x','y','z',
                     'A','B','C','D','E','F','G','H','I','J','K','L','M',
                     'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
                     '1','2','3','4','5','6','7','8','9','0','?','!','-',
                     ',','.',';' };
                     /*
                     '`','~','@','#','$','%','^','&','*','(',')',
                     '=','_','+','[',']','{','}','|',':','/',
                     '<','>',  };
                     */

   char *buffer;
   int charSetSize = 68; // length of the above array
   int length = 0;
   int max = 0;

   char randomNumber;
   int lettersInWord;

   // error checks - lots of code that saves us headaches later
   if( minimumLength < 1 ) {
      lr_error_message( "minimumLength smaller than 0 (%d)", minimumLength );
   }
   else if( maximumLength < 1 ) {
      lr_error_message( "maximumLength smaller than 0 (%d)", maximumLength );
   }
   else if( maximumLength > (1024 * 1024) ) {
      lr_error_message( "maximumLength too big (%d)", maximumLength );
   }
   else if( maximumLength < minimumLength ) {
      lr_error_message( "minimumLength (%d) bigger than maximumLength (%d)", minimumLength, maximumLength );
   }
   else if(maximumLength > minimumLength) {
      max = (rand() % (maximumLength-minimumLength)) + minimumLength;
   }
   else if(maximumLength == minimumLength) {
      max = maximumLength;
   }
   else {
      lr_error_message("This can never happen. If we reach this point it's a bug.");
   }

   // if we got an error
   if( max == 0 )
   {
      lr_set_transaction_status(LR_FAIL);
      // Not sure this is the right exit code that we want to use here, but ok.
      lr_exit(LR_EXIT_ITERATION_AND_CONTINUE, LR_FAIL);
   }
   
   // get memory for the buffer
   buffer = (char *)memAlloc( max +1 );
   if ( !buffer )
   {
      lr_error_message("Failed to allocate buffer for random string!");
      lr_exit(LR_ABORT, LR_FAIL);
   }
   

   while ( length < max )
   {
      lettersInWord = ((rand() % 8) + 2);

      while ( lettersInWord-- && (length < (max-1)) )
      {
         randomNumber = (char) (rand() % charSetSize);
         buffer[length++] = characterSet[randomNumber];
      }

      buffer[length++] = ' ';
   }

   buffer[length++] = '\0';

   lr_save_string(buffer, parameter);
   free(buffer);

   return 0;
}


#endif // _STRING_C