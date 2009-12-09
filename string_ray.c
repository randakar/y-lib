/*
 * LoadRunLib Loadrunner function library.
 * 
 * Copyright (c) 2005-2009 Floris Kraak
 *
 */
#ifndef _STRING_C
#define _STRING_C

extern char *strupr ( char *string ); 		// used in function "y_uppercase_parameter()"
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Allocates a block of memory for a string
// Adds some simple checks to catch common errors.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			char *test = y_mem_alloc(999);
//			..
//			free(test);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char *y_mem_alloc(int size)
{
	char *buff;
	int mem = size * sizeof(char);
	
	if(mem <= 0)
	{
		lr_error_message("Requested non positive amounts (%d) of memory! Bailing out ..", mem);
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
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Allocates a character array and initializes all elements to zero
// Adds some simple checks to catch common errors.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char *y_array_alloc(int length, int bytesPerChar)
{
	char *buff;
	int size = bytesPerChar ; // * sizeof(char);
	int mem = length * size;

	if(mem <= 0)
	{
		lr_error_message("Requested non positive amounts (%d) of memory! Bailing out ..", mem);
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
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Convert a *single* character 0-9 to an int
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//				int i;
//		    i=y_get_int_from_char('9');
//				lr_message("i = %d", i + 1);		// result is "i = 10"
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int y_get_int_from_char(char character)
{
	char tmp[2];
	tmp[0] = character;
	tmp[1] = '\0';
	return atoi(tmp);	
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Get the content of the parameter named "paramName" and return it as a char *
//
// Todo: Make a derivative function getParameterNoZeroes() which uses lr_eval_string_ext()
// and replaces all \x00 characters in the result with something else, like y_array_get_no_zeroes() does.
// Then every user of this function should start using it.
// Alternative: Make a function that cleans parameters of embedded null bytes and tell users to use that
// before doing anything else with possible tainted values.
// I think I like that one better to be honest. For the Array_ subset it's not that bad given the better
// memory management but for this one .. muh.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//				char *test;
//				lr_save_string("test123", "TestParam");		// save the string "test123" into parameter {TestParam}
//				test=y_get_parameter("TestParam");
//				lr_message("Test: %s", test);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* y_get_parameter(const char* paramName)
{
   char *parameter;
   char *tmp = y_mem_alloc( strlen(paramName) +3 ); 
                         // parameter name + "{}" + '\0' (end of string)
   sprintf( tmp, "{%s}", paramName );   
   parameter = lr_eval_string(tmp);
   free(tmp);
   
   return parameter;
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Clean a parameter by removing any embedded \x00 (null) characters from it.
// This would only happen if you have used to web_reg_save_param() and the result contains
// a null-character. 
// Any such characters are replaced with ' '. (space)
// Since this changes existing parameters be careful what types of parameters 
// you use this on.But when no null-character is found, the result is unaltered.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			char *test;
//			web_reg_save_param("TestParam", "LB=\"name=LastName\" Value=\"", "RB=\"", LAST);
//			web_submit_data(...);					// will fail, obiously.									
//			lr_message(lr_eval_string("TestParam: {TestParam}\n"));			
//			y_cleanse_parameter("TestParam");
//			test=y_get_parameter("TestParam");
//			lr_message("\nTest: >%s<\n", test);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void y_cleanse_parameter(const char* paramName)
{
   char *tmp = y_mem_alloc( strlen(paramName) +3 ); 
                         // parameter name + "{}" + '\0' (end of string)
   char *result;
   unsigned long resultLen;
   size_t resultStrlen;

   //lr_log_message("y_cleanse_parameter(%s)", paramName );

   // Get the contents of the parameter using lr_eval_string_ext() - we can't use the
   // regular version if we expect to find \x00 in there.
   sprintf( tmp, "{%s}", paramName );
   lr_eval_string_ext(tmp, strlen(tmp), &result, &resultLen, 0, 0, -1);
   free (tmp);

   // replace NULL bytes (\x00) in the input with something else..
   for( resultStrlen = strlen(result);
		resultStrlen < resultLen;
		resultStrlen = strlen(result))
   {
	   result[resultStrlen] = ' ';
   }

   // Put the result back into the original parameter.
   lr_save_string(result, paramName);

   lr_eval_string_ext_free(&result);


   return;
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Convert the content of a parameter to UPPERCASE. Does not affect non-alphabetic characters.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			lr_save_string("aBcDeFgHiJ &*45#$@#)!({}", "Test");
//			lr_message(lr_eval_string("Original: {Test}\n"));
//			y_uppercase_parameter("Test");
//			lr_message(lr_eval_string("Altered: {Test}\n"));
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_uppercase_parameter(const char* paramName)
{
   char *result = y_get_parameter(paramName);
   //strupper(result);
   strupr(result);				// Note that in Vugen files, you need to explicitly declare C functions that do not return integers
   lr_save_string(result, paramName);
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Cut the end off a string based on a search string and store the resulting head in a parameter.
// In other words: split the string into 2 parts, using the search string. Save the left part
// into the resultParameter.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			lr_save_string("AstrixObelixIdefix", "Test");
//			lr_message(lr_eval_string("Original: {Test}\n"));	// {Test}=AstrixObelixIdefix
//			y_left( "Test", "Obelix", "Test2" );
//			lr_message(lr_eval_string("New Param: {Test2}\n"));	// {Test2}=Astrix
//
//	note: the previous name of this function was: head() and y_head()
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_left( const char *originalParameter, const char *search, const char *resultParameter )
{
   char *buffer;
   char *original = y_get_parameter(originalParameter);
   char *posPtr = (char *)strstr(original, search);
   int pos = (int)(posPtr - original);

   //lr_log_message("y_head: original=%s, search=%s, resultParam=%s", original, search, resultParam);

   if( posPtr == NULL )
   {
      lr_save_string(original, resultParameter);
      return;
   }
   //lr_log_message("pos = %d", pos);
   
   // Copy the original to a temporary buffer
   buffer = y_mem_alloc(strlen(original)+1);
   strcpy(buffer, original);
   buffer[pos] = '\0';                             // make the cut
   lr_save_string(buffer, resultParameter);        // save the result
   free(buffer);
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Cut the y_head off a string based on a search string, leaving only the tail. Store the result in a parameter.
// In other words: split the string into 2 parts, using the search string. Save the right part
// into the resultParameter.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			lr_save_string("AstrixObelixIdefix", "Test");
//			lr_message(lr_eval_string("Original: {Test}\n"));	// {Test}=AstrixObelixIdefix
//			y_right( "Test", "Obelix", "Test4" );
//			lr_message(lr_eval_string("New Param: {Test4}\n"));	//	{Test4}=Idefix
//
//	note: previous name: head() and  y_head()
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_right( const char *originalParameter, const char *search, const char *resultParameter)
{
	char *original = y_get_parameter(originalParameter);

	char *posPtr = (char *)strstr(original, search);
	int pos = (int)(posPtr - original);

	//lr_log_message("y_right: original=%s, search=%s, resultParam=%s", original, search, resultParameter);

	if( posPtr == NULL )
	{
		lr_save_string(original, resultParameter);
		return;
	}

	//lr_log_message("pos = %d", pos);

	posPtr = posPtr + strlen(search);
	lr_save_string(posPtr, resultParameter);
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Same as y_right(), but don't stop at the first match, rather use the last match instead.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			lr_save_string("WackoYackoDotWarner", "Test");
//			lr_message(lr_eval_string("Original: {Test}\n"));	// {Test}=WackoYackoDotWarner
//			y_last_right( "Test", "ot", "Test2" );
//			lr_message(lr_eval_string("New Param: {Test2}\n"));
//
//	note: previous name: tail_tail()
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_last_right( const char *originalParameter, const char *search, const char *resultParameter)
{
	char *result = y_get_parameter(originalParameter);
	char *posPtr;
	//int pos;

	if( search == NULL || strlen(search) == 0 )
	{
		lr_save_string(result, resultParameter);
		//lr_log_message("Warning: Empty search parameter passed to y_last_right()");
		return;
	}

	//lr_log_message("y_last_right: original=%s, search=%s, resultParam=%s", original, search, resultParameter);

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
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// y_split a string in two based on a search parameter.
//
// Note: Unlike the others this one does not use parameter, but raw char pointers instead.
// This mostly to accomodate the primary user - Array_Split();
// Both output buffers need to be pre-allocated! 
//
// For the parameter version use y_split().
//
// @author Floris Kraak
//
y_split_str( const char *original, const char *separator, char *left, char *right)
{
	char *buffer;
	char *posPtr = (char *)strstr(original, separator);
	int pos = (int)(posPtr - original);;

	//lr_log_message("y_split_str: original=%s, search=%s", original, search);

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
	strcpy(right, posPtr+strlen(separator));
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// y_split a string in two based on a seperating string.
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			lr_save_string("WackoYackoDotWarner", "Test");
//			lr_message(lr_eval_string("Original: {Test}\n"));	// {Test}=WackoYackoDotWarner
//			y_split("Test", "Yacko", "Left", "Right");			// Use "Yacko" as the separator
//			lr_message(lr_eval_string("Original: {Test}\n"));	// {Test}	= WackoYackoDot
//			lr_message(lr_eval_string("Left    : {Left}\n"));	// {Left}	= Wacko
//			lr_message(lr_eval_string("Right   : {Right}\n"));	// {Right}	= Warner
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_split( const char *originalParameter, const char *separator, const char *leftParameter, const char *rightParameter)
{
    char *item = y_get_parameter(originalParameter);
	int len = strlen(item);

	if( len < strlen(separator) )
	{
		// Since the separator doesn't even fit in the item, searching is pointless.
		// Store the original in the left hand parameter, the right hand is empty.
		lr_save_string(originalParameter, leftParameter);
		lr_save_string("", rightParameter);
		return;

	}
	else
	{

		// Left hand side
		// If the separator isn't found the full original string gets stored here.
		// Don't forget the 0 byte at the end though..
		char *left = y_mem_alloc(len+1);

		// Right hand side
		// If the separator gets found in position 1 the remainder of the
		// original string gets stored here (and left gets a zero-length string).
		char *right = y_mem_alloc(len-strlen(separator)+1);

		// Start off with zero-length strings. We can safely assume
		// both variables contain garbage when freshly allocated.
		left[0] = '\0';
		right[0] = '\0';

		// This is where the magic happens.
		y_split_str(item, separator, left, right);

		// Store the results in parameters.
		lr_save_string(left, leftParameter);
		free(left);
		lr_save_string(right, rightParameter);
		free(right);
	}
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Remove leading and trailing whitespace from a parameter. Does not support Unicode so use with care.
//	Whitespace can be: " "(=space)		"\r"(=carrige return)	"\n"(=line feed)	"\t"(=tab)
// @author: Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage:
//			lr_save_string("  WackoYackoDot ", "Test");
//			lr_message(lr_eval_string("Original: >{Test}<\n"));	// {Test}="  WackoYackoDot "
//			y_chop("Test");
//			lr_message(lr_eval_string("Original: >{Test}<\n"));	// {Test}	= "WackoYackoDot"
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_chop( const char* parameter )
{
	char *result;
	int i = 0;
	char character;
	
	//lr_output_message( "y_chop(%s)", parameter);	
	result = y_get_parameter(parameter);

	// y_chop leading whitespace
	character = result[i];
	while( (character == ' ') || (character == '\r') || (character == '\n') || (character == '\t') )
	{
		character = result[++i];
	}
	result += i;
	
	//lr_output_message("result after removal of leading whitespace: %s", result);
	
	// y_chop trailing whitespace
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
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Search and y_replace.
// Find 'search' in 'parameter' and replace it with 'y_replace'.
// Replaces the originally passed-in parameter with the new one.
//
// Note: This one has a built-in search/y_replace limit when
// search > y_replace. It won't do it more than 'limit' times.
//
// @author Floris Kraak
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage: 	
// 			lr_save_string("test123", "par1");
//          y_replace("par1", "1", "ing1");		// {par1} now has the value testing123
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_replace( const char *parameter, const char *search, const char *y_replace)
{
   int slen, rlen, plen;      // lengte van search, y_replace, en basis string
   int i = 0;                 // aantal replacements
   int limit = 1000;          // replacement limiet als y_replace > search

   char *c;                   // punt waar change moet beginnen
   char *cend;                // einde van de change = c+slen
   char *last;                // de \0 slotbyte van input string
   char *buffer;              // buffer voor bewerkingen
   char *string;              // originele string waar we in zoeken

   if ((search == NULL) 	|| (strlen(search) <1))     return;		// added by Ray
																// 
   if (!search || !y_replace)      return;   // ongeldige search of y_replace
   if (!strcmp(search, y_replace)) return;   // search == y_replace: geen wijziging

   slen = strlen (search);          // de lengte van search
   rlen = strlen (y_replace);         // de lengte van y_replace

   string = y_get_parameter(parameter); // <-- memory allocated by loadrunner, too small if y_replace > search
   plen = strlen(string);

   //lr_log_message("y_replace(%s, %s, %s) - slen %d, rlen %d, plen %d", parameter, search, y_replace, slen, rlen, plen);
	
   if ( rlen > slen)
   {
      // Reserve memory for -limit- replacements.
      buffer = y_mem_alloc( plen + ((rlen-slen) * limit) );
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
      if (slen != rlen)                    // zijn search en y_replace van verschillende lengte?
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
      memcpy (c, y_replace, rlen);         // voeg y_replace toe over search heen
   }
   lr_save_string(buffer, parameter);

   if( rlen > slen )
   {
      free(buffer);
   }
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Lightweight alternative to the y_replace() function.
// Remove all occorrances of 'removeMe' in the parameter named 'paramName'
// stores the result in the original parameter
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage: 	
// 			lr_save_string("test123", "par1");
//          y_remove_string_from_parameter("par1", "1");		// {par1} now has the value test23
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_remove_string_from_parameter(const char* paramName, const char* removeMe)
{
   char* parameter;
   char* tmp;
   int removePtr;

   lr_log_message("y_remove_string_from_parameter( remove:%s, parameter:%s )", removeMe, paramName);

   if ((removeMe == NULL) || (strlen(removeMe) <1))     return;		// added by Ray

   // fetch the contents of the parameter to change
   parameter = y_get_parameter(paramName);

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
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
// Usage: y_random_string_buffer("parameterNameWhichGetsTheRandomValue", minimumlength, maximumlength);
//
// ex. randomString("randomFeedback", 100, 200); will fill a parameter with between 100 and 200 characters.
// to use the random value, use the parameter name provided.
// To make it look like real sentences, this function inserts spaces at random.
// The "words" will be minimal 1 character long, and max. 8 characters.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//		example usage: 	
//			y_random_string_buffer("par1", 10,10);	// creates a string of exactly 10 characters and save it into string {par1}
//			y_random_string_buffer("par1", 5,10);	// creates a string of min 5 and max 10 char and save it into string {par1}	
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
y_random_string_buffer(const char *parameter, int minimumLength, int maximumLength)
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
   buffer = (char *)y_mem_alloc( max +1 );
   if ( !buffer )
   {
      lr_error_message("Failed to allocate buffer for random string!");
      lr_exit(LR_ABORT, LR_FAIL);
   }
   

   while ( length < max )
   {
      lettersInWord = ((rand() % 8) + 2);

      while ( lettersInWord-- && (length < (max)) )
      {
         randomNumber = (char) (rand() % charSetSize);
         buffer[length++] = characterSet[randomNumber];
      }

      if (length!=max)
	  {
		  buffer[length++] = ' ';
	  }
   }

   buffer[length++] = '\0';

   lr_save_string(buffer, parameter);
   free(buffer);

   return 0;
}
// --------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------
#endif // _STRING_C