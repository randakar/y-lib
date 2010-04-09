/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2009-2010 Raymond de Jongh <ferretproof@gmail.com> | <rdjongh@ymor.nl>
 * Copyright (C) 2010 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
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

/*****************************************************************************************************
  - VTS functies -
datum   :   2008-11-13
            2009-09-03

benodigd:   VTS-server. Deze is normaal gesproken te vinden op een pc met loadrunner.
            Bij mij staat deze op C:\Program Files\HP\LoadRunner\bin\vtconsole.exe
            Het ip-adres en poortnummer kan je instellen via de parameters VTSServer en VTSPort
            Deze instellingen moeten overeenkomen met die van de VTSServer.

VTS installeren:
1) Obtain VTS2 from Mercury Support Representatives. (Downloadable Binaries)
2) Unzip the file into the main LoadRunner directory (using the directory structure provided).
   (eg. "..\Mercury Interactive\LoadRunner")
   
3) Register "vtsctls.ocx" in the "..\LoadRunner\bin" directory.
  (eg. regsvr32 "c:\Program Files\Mercury Interactive\LoadRunner\bin\vtsctls.ocx")
  
VTS starten:
    C:\Program Files\HP\LoadRunner\bin\vtconsole.exe
*****************************************************************************************************/

#ifndef _VTS_FUNC_C
#define _VTS_FUNC_C

#include "vts2.h"
#include "y_string.c"
#include "y_loadrunner_utils.c"

// Global variables
int _vts_setup_completed = 0;


// Standardised error reporting for all VTS functions.
// @author Floris Kraak
void VTS_report_error(char* errortext)
{
    const char* errorheader = "****** VTS ERROR: ";
    char *buffer = y_mem_alloc( strlen(errorheader) + strlen(errortext) +1 );
    sprintf(buffer, "%s%s", errorheader, errortext);
    lr_error_message(buffer);
    lr_save_string(buffer, "VTS_ERROR_MESSAGE");
    free(buffer);
}

// Initialisation of the VTS client library etc.
// @author Floris Kraak
void VTS_setup()
{
    int result;
    if( _vts_setup_completed )
    {
        return;
    }
    
    //***************************
    //* Load the client VTS DLL *
    //***************************
    if( (result = lr_load_dll("vtclient.dll")) != 0 )
    {
        VTS_report_error("Unable to load Virtual Table Server client dll. Please check your VTS installation.");
        lr_exit(LR_EXIT_VUSER, LR_FAIL);
    }

    _vts_setup_completed = 1;
}

//
// Translation facility for VTS return codes. Also reports any errors.
// 
// @author Floris Kraak
int VTS_process_returncode(int returncode)
{
    char* errortext;
    
    /** 
     * This is what the header file vts2.h tells us about error codes:
     *
     * //VTS Error Codes
     * #define  VTCERR_OK                            0
     * #define  VTCERR_INVALID_CONNECTION_INFO  -10000
     * #define  VTCERR_FAILED_TO_RESOLVE_ADDR   -10001
     * #define  VTCERR_FAILED_TO_CREATE_SOCKET  -10002
     * #define  VTCERR_FAILED_TO_CONNECT        -10003
     *
     * #define  VTCERR_INCOMPLETE_REQUEST       -10100
     * #define  VTCERR_FAILED_TO_RECV_RESPONSE  -10101
     * #define  VTCERR_INCOMPLETE_RESPONSE      -10102
     * #define  VTCERR_RESPONSE_ARGS_UNMATCH    -10103
     *
     * // --- Operation Error Base
     * #define  VTCERR_OPERATION_ERROR_BASE     -11000
     * #define  VTCERR_SERVER_IS_BUSY           (VTCERR_OPERATION_ERROR_BASE - 0xFF)
     * #define  VTCERR_CLIENT_REQUEST_ERROR     (VTCERR_OPERATION_ERROR_BASE - 0xFE)
     **/

    // Encode the above information in a simple lookup table.
    switch(returncode)
    {
        case VTCERR_OK:
            errortext = "INFO: VTS command succeeded.";
            lr_message(errortext);
            return returncode;
            break; // <-- never reached, but kept for readability.

        case VTCERR_INVALID_CONNECTION_INFO:
            errortext = "Invalid connection info.";
            break;

        case VTCERR_FAILED_TO_RESOLVE_ADDR:
            errortext = "Failed to resolve address.";
            break;

        case VTCERR_FAILED_TO_CREATE_SOCKET:
            errortext = "Failed to create socket.";
            break;

        case VTCERR_FAILED_TO_CONNECT:
            errortext = "Failed to connect.";
            break;

        case VTCERR_INCOMPLETE_REQUEST:
            errortext = "Incomplete request.";
            break;

        case VTCERR_FAILED_TO_RECV_RESPONSE:
            errortext = "Failed to receive response.";
            break;

        case VTCERR_INCOMPLETE_RESPONSE:
            errortext = "Incomplete response.";
            break;

        case VTCERR_RESPONSE_ARGS_UNMATCH:
            errortext = "Received error code VTCERR_RESPONSE_ARGS_UNMATCH.";
            break;

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
    VTS_report_error(errortext);
    // At this point it might be an idea to call lr_abort() or lr_exit(LR_EXIT_VUSER, LR_FAIL)
    
    return returncode;
}


// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************

// maak een verbinding met de VTS-server
// nodig: parameter VTSServer, bevat de URL of ip-adres van de VTS-server
//        parameter VTSPort, bevat de Poortnummer van de VTS-server
int VTS_connect()
{
    PVCI ppp;
    VTS_setup();

    // Connect to the Virtual Table Server and grab the Handle, and print it.
    //ppp = vtc_connect(lr_eval_string("{VTSServer}"), atoi(lr_eval_string("{VTSPort}")), VTOPT_KEEP_ALIVE);
    ppp = lrvtc_connect(lr_eval_string("{VTSServer}"), atoi(lr_eval_string("{VTSPort}")), VTOPT_KEEP_ALIVE);
    
    if( VTS_process_returncode(vtc_get_last_error(ppp)) != VTCERR_OK )
    {
        ppp = -1;
    }

    // lr_output_message(">> The VTS Handle is : %d", ppp);
    lr_save_int(ppp, "VTS_ppp");
    return ppp;
}

// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************

// verbinding met VTS verbreken
int VTS_disconnect()
{
    lrvtc_disconnect();
    return 0;
}



// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************

// Voeg een waarde toe aan de onderkant van de tabel. 
// Als de unique vlag groter dan 0 is gebeurt dat alleen onder voorwaarde dat de waarde niet al 
// bestaat in de tabel.
//
// Todo: convert these functions to return a VTCERR rather than a simple int.
//
// @author Floris Kraak
int VTS_pushlast_with_flag(char* columnname, char* value, int unique)
{
    int errorcode = 0;
    char* errortext = "Write to VTS: OK.";
    unsigned short status;
    PVCI ppp = VTS_connect();

    if( ppp == -1 )
    {
        // VTS_connect() should have set the error message already.
        return -1;
    }

    if( unique > 0 )
    {
        errorcode = vtc_send_if_unique(ppp, columnname, value, &status);
    }
    else
    {
        errorcode = vtc_send_message(ppp, columnname, value, &status);
    }
    vtc_free(value);
    VTS_disconnect();

    //lr_log_message("result: %d .... send message status: %d", errorcode, status);
    if(VTS_process_returncode(errorcode) == VTCERR_OK)
    {
        if(status == 1)
        {
            // Success!
            lr_message("INFO: Value pushed onto column successfully.");
        }
        else
        {
            errorcode = -2;
            errortext = "Can not write to VTS: value (most likely) already exists in VTS.";
            VTS_report_error(errortext);
        }
    }

    return errorcode;
}


// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************

// Voeg een waarde toe aan de onderkant van de tabel, onder voorwaarde dat deze waarde niet al bestaat!
int VTS_pushlast_unique(char* columnname, char* value)
{
    return VTS_pushlast_with_flag(columnname, value, 1);
}

// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************

// Voeg een waarde toe aan de onderkant van de tabel.
int VTS_pushlast(char* columnname, char* value)
{
    return VTS_pushlast_with_flag(columnname, value, 0);
}



// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *the end* * *
// ***************************************************************************************************


// maak de hele kolom (met name "columnname") leeg
int VTS_clearColumn(char* columnname)
{
    PVCI ppp = VTS_connect();
    int errorcode = 0;
    unsigned short status;

    if( ppp == -1 )
    {
        // VTS_connect() should have set the error message already.
        return -1;
    }

    errorcode = vtc_clear_column(ppp, columnname, &status);
    VTS_disconnect();

    if(VTS_process_returncode(errorcode) == VTCERR_OK)
    {
        if(status == 1)
        {
            // Success!
            lr_message("INFO: Content of the column is deleted.");
        }
        else
        {
            errorcode = -2;
            VTS_report_error("Failed to clear column.");
        }
    }

    return errorcode;
}


// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *the end* * *
// ***************************************************************************************************

// lees een willekeurige cel uit de tabel columnname
int VTS_readRandom(char* columnname, char* ParameterName)
{
    PVCI ppp = VTS_connect();
    int rc = 0;
    unsigned short status;
    int errorcode = 0;
    char* errortext;
    int tablesize;
    int rand_row;
    char* value = NULL;

    if( ppp == -1 )
    {
        // VTS_connect() should have set the error message already.
        return -1;
    }

    if((rc = vtc_column_size(ppp, columnname, &tablesize)) != 0)
    {
        errorcode = rc;
        errortext = "Can not determine column size. Error code %d.";
    }

    //lr_log_message("tablesize: %d\n", tablesize);
    rand_row = y_rand() % tablesize + 1;
    if((rc = vtc_query_column(ppp, columnname, rand_row, &value)) != 0)
    {
        errortext = "******************** VTS Error - Query Return Code = %d";
        errorcode = rc;
    }
    else
    {
        errortext = "INFO: VTS random read succeeded.";
        lr_save_string(value,ParameterName);        
    }
    vtc_free(value);    
    VTS_disconnect();   

    if(errorcode != 0)
    {
        lr_error_message(errortext, errorcode);
    }
    lr_param_sprintf("VTS_ERROR_MESSAGE", errortext, errorcode);

    return errorcode;
}



// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************
// input: een waarde waarin de kolomnamen staan die uitgelezen moeten worden 
// output: parameter die de naam heeft van de kolomnamen. 
// verwerking: de waarde van de opgegeven kolom worden in een parameter gezet en vervolgens wordt deze waarden 
//             uit de database verwijderd. Zodoende kan een deze waarde nooit 2x gebruikt worden.
int VTS_popfirst(char *columnname)
{
    PVCI           ppp = VTS_connect();
    int            rc = 0;
    int            size;
    unsigned short status;
    int            errorcode = 0;
    
    if( ppp == -1 )
    {
        // VTS_connect() should have set the error message already.
        return -1;
    }
    
    if( (rc = lrvtc_retrieve_message(columnname)) != 0)
    {
        lr_error_message("******************** VTS Error - Query Return Code = %d", rc);
    }
    else
    {
        // dit werkt niet helemaal goed. Dit geeft de tekst "columname" terug, ipv de inhoud van {columnname}
        lr_output_message ("Retrieved value is : %s", y_get_parameter(columnname));
    }
    VTS_disconnect();

    return errorcode;
}


// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************
// VTS_push_multiple_columns()
//
//     Voeg data toe aan meerdere kolommen tegelijk, aan de onderkant van de database.
//     De kolomnamen staan in 1 string, gescheiden door een punt-komma.
//     De data staan in 1 string, gescheiden door een punt-komma.
// 
// Voorbeeld:
//     VTS_push_multiple_columns("VOORNAAM,ACHTERNAAM,ADRES", "Pietje;Puk;Wegiswegweg 3");
int VTS_push_multiple_columns_unique(char *columnnames, char *data)
{
    PVCI           ppp = VTS_connect();
    int            rc = 0;
    unsigned short status;
    int            errorcode = 0;

    if( ppp == -1 )
    {
        // VTS_connect() should have set the error message already.
        return -1;
    }

    rc=lrvtc_send_row1(columnnames, data, ";", VTSEND_STACKED_UNIQUE);
    if(rc != 0)
    {
        lr_save_string("Can not write to columns: ", "VTS_ERROR_MESSAGE");
        lr_error_message(lr_eval_string("{VTS_ERROR_MESSAGE}"));
        errorcode = -1;
    }

    VTS_disconnect();
    return errorcode;
}


// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************
// VTS_popfirstMultipleColumns()
// 
// Haal uit VTS de bovenste rij uit de opgegeven kolommen.
// deze kolommen staan in 1 string, gescheiden door een punt-komma.
// 
// Voorbeeld:
//     VTS_popfirstMultipleColumns("Voornaam;Achternaam;Adres");
//        Het resultaat wordt dan in {Voornaam}, {Acternaam} en {Adres} geplaatst.
// 
//    snelheid:
//     wanneer in VTS 1 miljoen records staan, en 1000x het volgende wordt uitgevoerd:
//           VTS_push_multiple_columns("CRDNUM;UTN;EMBNM1", "123123123;123123123;JANSEN");
//            VTS_popfirstMultipleColumns2("CRDNUM;UTN;EMBNM1");
//     dan duurt dat ca. 12,5 sec. Dat is gemiddeld dus per push en pop: 12,5 msec. 
int VTS_popfirstMultipleColumns(char *gewenste_databasevelden)
{
    PVCI           ppp = VTS_connect();
    int            rc = 0;
    unsigned short status;
    int            errorcode = 0;

    if( ppp == -1 )
    {
        // VTS_connect() should have set the error message already.
        return -1;
    }

    if( (rc = lrvtc_retrieve_messages1(gewenste_databasevelden, ";")) != 0)
    {
        lr_error_message("******************** VTS Error - Query Return Code = %d", rc);
    }
    else
    {
        // lr_output_message("******************** Query Column 1 Result = %s", value);
        // lr_save_string(value,lr_eval_string("{databaseveld}"));
    }
    VTS_disconnect();

    return errorcode;
}



// ***************************************************************************************************
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ***************************************************************************************************
// VTS_push_multiple_columns()
//
//     Voeg data toe aan meerdere kolommen tegelijk, aan de onderkant van de database.
//     De kolomnamen staan in 1 string, gescheiden door een punt-komma.
//     De data staan in 1 string, gescheiden door een punt-komma.
// 
// Voorbeeld:
//     VTS_push_multiple_columns("VOORNAAM,ACHTERNAAM,ADRES", "Pietje;Puk;Wegiswegweg 3");
int VTS_push_multiple_columns(char* columnnames, char* data)
{
    int            rc = 0;
    unsigned short status;
    int            errorcode = 0;
    PVCI ppp = VTS_connect();

    if( ppp == -1 )
    {
        // VTS_connect() should have set the error message already.
        return -1;
    }

    errorcode=vtc_send_row1(ppp, columnnames, data, ";", VTSEND_SAME_ROW, &status);
    //lr_message("status: %d", status);

    VTS_disconnect();

    if(VTS_process_returncode(errorcode) == VTCERR_OK)
    {
        if(status > 0)
        {
            // Success!
            lr_message("INFO: Data written to multiple columns.");
        }
        else
        {
            errorcode = -2;
            VTS_report_error("Failed to write to multiple columns.");
        }
    }

    return errorcode;
}

// --------------------------------------------------------------------------------------------------
#endif // _VTS_FUNC_C