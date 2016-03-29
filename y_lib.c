/*
 * Ylib Loadrunner function library.
 * Copyright (C) 2005-2014 Floris Kraak <randakar@gmail.com> | <fkraak@ymor.nl>
 * Copyright (C) 2009 Raymond de Jongh <ferretproof@gmail.com> | <rdjongh@ymor.nl>
 * Copyright (C) 2012-2013 André Luyer
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
 
/*! \file y_lib.c
 *  \brief Main y_lib include file.
 * Including this file will cause all common parts of y-lib to be included into your script.
 * 
 * Documentation generated from this source code can be found here: http://randakar.github.io/y-lib/
 * Main git repitory can be found at https://github.com/randakar/y-lib
 *
 * \note If compile time is a concern, you can elect to just include the parts you wish to use instead, as well.
 * \author Floris Kraak
 */
#ifndef _YLIB_C_
//! \cond include_protection
#define _YLIB_C_
//! \endcond

#include "vugen.h"
#include "y_core.c"
#include "y_string.c"
#include "y_loadrunner_utils.c"
#include "y_logging.c"
#include "y_transaction.c"
#include "y_param_array.c"
#include "y_flow_list.c" // y_profile.c got renamed, and most variables and function names in there as well.
#include "y_browseremulation.c"

#endif // _YLIB_C_
