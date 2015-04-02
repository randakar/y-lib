/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2009-2010 Raymond de Jongh <ferretproof@gmail.com> | <rdjongh@ymor.nl>
 * Copyright (C) 2010,2014 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

/*!
\file y_vts.c
\brief VTS functions for LR 11.5 and above.

Library wrapperrs for talking to a VTS-server. 
This is part of LR 11.5 and needs to be installed seperately.
For LR versions older than 11.5, see the old y_vts.c.

\warning This y_vts.c is NOT compatible with the vts version that was shipped before LR 11.5, and the older y_vts.c is NOT compatible with the version in LR 11.5 either.

*/

#ifndef _Y_VTS_FUNC_C_
//! \cond include_protection
#define _Y_VTS_FUNC_C_
//! \endcond

/* \brief Standardised error reporting for all VTS functions.

\author Floris Kraak
*/
void y_vts_report_error(char* message)
{
    lr_error_message("****** VTS ERROR: %s", message);
}

/*!
\brief Error translation and reporting facility for VTS return codes.
This will emit VTS errors using lr_error_message() in human readable form, if the code represents a VTS error.

\param returncode An error code as returned by VTS
\returns the returncode

This is what the header file vts2.h tells us about error codes:

\code
//VTS Error Codes
#define  VTCERR_OK                            0
#define  VTCERR_INVALID_CONNECTION_INFO  -10000
#define  VTCERR_FAILED_TO_RESOLVE_ADDR   -10001
#define  VTCERR_FAILED_TO_CREATE_SOCKET  -10002
#define  VTCERR_FAILED_TO_CONNECT        -10003

#define  VTCERR_INCOMPLETE_REQUEST       -10100
#define  VTCERR_FAILED_TO_RECV_RESPONSE  -10101
#define  VTCERR_INCOMPLETE_RESPONSE      -10102
#define  VTCERR_RESPONSE_ARGS_UNMATCH    -10103

// --- Operation Error Base
#define  VTCERR_OPERATION_ERROR_BASE     -11000
#define  VTCERR_SERVER_IS_BUSY           (VTCERR_OPERATION_ERROR_BASE - 0xFF)
#define  VTCERR_CLIENT_REQUEST_ERROR     (VTCERR_OPERATION_ERROR_BASE - 0xFE)
\endcode

Loadrunner 11.52 expanded that a little, to the following definition in the help files:

\code
VTCERR_OK 0 Success 
VTCERR_INVALID_CONNECTION_INFO -10000 The corresponding handle does not exist or the connection information is corrupted. Disconnect and reconnect. 
VTCERR_FAILED_TO_RESOLVE_ADDR -10001 Failed to find server. 
VTCERR_FAILED_TO_CREATE_SOCKET -10002 Failed to create socket. 
VTCERR_FAILED_TO_CONNECT -10003 Failed to connect. Check the server name, port number, network connectivity, and whether server is on line. 
VTCERR_INVALID_API_CALL -10004 Failed to get the API entry.  <-- NEW

VTCERR_INCOMPLETE_REQUEST -10100 Communications packet from client is invalid. 
VTCERR_FAILED_TO_RECV_RESPONSE -10101 No response received from server. 
VTCERR_INCOMPLETE_RESPONSE -10102 Communications packet from server is invalid. 
VTCERR_RESPONSE_ARGS_UNMATCH -10103 Unexpected count of arguments in server response. 
VTCERR_INVALID_ARGUMENT -10104 Invalid argument.  <-- NEW
VTCERR_HANDLE_NOT_EXIST -10105 Connection handle does not exist. <-- NEW
VTCERR_INNER_JSON_CONVERT -10106 Cannot parse server response. <-- NEW
VTCERR_INNER_UTF8_CONVERT -10107 Cannot convert between UTF8 and Locale.  <-- NEW
VTCERR_COL_FORMAT_ERROR -10108 Invalid or empty column name.  <-- NEW
VTCERR_COL_VALUE_NO_MATCH -10109 Column names list and messages list do not have the same number of values. Check delimiters.  <-- NEW
VTCERR_EVAL_STRING -10110 Error evaluating parameter value.  <-- NEW
VTCERR_DATA_NOT_EXIST -10111 There is no data at the specified column and row.  <-- NEW
\endcode

.. note however that the "Operation Error Base" codes are gone.

\author Floris Kraak
*/
int y_vts_process_returncode(int returncode)
{
    char* errortext;
    
    // Encode the above information in a simple lookup table.
    switch(returncode)
    {
        case VTCERR_OK:
            errortext = "INFO: VTS command succeeded.";
            lr_message(errortext);
            return returncode;
            break; // <-- never reached, but kept for readability.

        case VTCERR_INVALID_CONNECTION_INFO:
            errortext = "The corresponding handle does not exist or the connection information is corrupted. Disconnect and reconnect.";
            break;

        case VTCERR_FAILED_TO_RESOLVE_ADDR:
            errortext = "Failed to resolve server address.";
            break;

        case VTCERR_FAILED_TO_CREATE_SOCKET:
            errortext = "Failed to create socket.";
            break;

        case VTCERR_FAILED_TO_CONNECT:
            errortext = "Failed to connect. Check the server name, port number, network connectivity, and whether server is on line."; // LR 11.52 simply calls lr_abort() if this happens.
            break;
            
        case VTCERR_INVALID_API_CALL: 
            errortext = "Failed to get the API entry.";
            break;

        case VTCERR_INCOMPLETE_REQUEST:
            errortext = "Communications packet from client is invalid.";
            break;

        case VTCERR_FAILED_TO_RECV_RESPONSE:
            errortext = "No response received from server.";
            break;

        case VTCERR_INCOMPLETE_RESPONSE:
            errortext = "Response from server is incomplete.";
            break;

        case VTCERR_RESPONSE_ARGS_UNMATCH:
            errortext = "Unexpected count of arguments in server response.";
            break;

        case VTCERR_INVALID_ARGUMENT:
            errortext = "Invalid argument.";
            break;

        case VTCERR_HANDLE_NOT_EXIST:
            errortext = "Connection handle does not exist.";
            break;

        case VTCERR_INNER_JSON_CONVERT:
            errortext = "Cannot parse server (JSON) response.";
            break;

        case VTCERR_INNER_UTF8_CONVERT:
            errortext = "Cannot convert between UTF8 and Locale.";
            break;

        case VTCERR_COL_FORMAT_ERROR:
            errortext = "Invalid or empty column name.";
            break;

        case VTCERR_COL_VALUE_NO_MATCH:
            errortext = "Column names list and messages list do not have the same number of values. Check delimiters.";
            break;

        case VTCERR_EVAL_STRING:
            errortext = "Error evaluating parameter value.";
            break;

        case VTCERR_DATA_NOT_EXIST:
            errortext = "There is no data at the specified column and row.";
            break;

        // These operation base errors may not exist in LR 11.52. The documentation no longer mentions them.            
        case VTCERR_OPERATION_ERROR_BASE:
            errortext = "Received error code VTCERR_OPERATION_ERROR_BASE.";
            break;

        case VTCERR_SERVER_IS_BUSY:
            errortext = "Server is busy. Go away.";
            break;

        case VTCERR_CLIENT_REQUEST_ERROR:
            errortext = "Client request error.";
            break;

        default:
            errortext = "Unknown VTC error code.";
            break;
    }

    // Report the error to the user. (The "All OK" case jumped out of this function earlier ..)
    y_vts_report_error(errortext);
    // At this point it might be an idea to call lr_abort() or lr_exit(LR_EXIT_VUSER, LR_FAIL)
    
    return returncode;
}

// --------------------------------------------------------------------------------------------------
#endif // _Y_VTS_FUNC_C_
