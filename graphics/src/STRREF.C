/*------------------------------------------------------------------------*/
/*                                                                        */
/*  STRINGS.CPP                                                           */
/*                                                                        */
/*  Copyright Borland International 1993                                  */
/*  All Rights Reserved                                                   */
/*                                                                        */
/*  Definitions for Windows-specific member functions of string class.    */
/*                                                                        */
/*------------------------------------------------------------------------*/

#if !defined( _Windows )
#define _Windows
#endif

#if !defined( STRICT )
#define STRICT
#endif

#include <stdlib.h>
#include <windows.h>
#include <cstring.h>

extern xalloc __xalloc;

string::string( HINSTANCE instance, UINT id, int len )
    throw( xalloc, string::lengtherror )
{
    p = new TStringRef( instance, id, len );
}

TStringRef::TStringRef( HINSTANCE instance, UINT id, int len )
    throw( xalloc, string::lengtherror ) : TReference(1), flags(0)
{
    capacity = round_capacity(len);
    array = (char _FAR *)malloc( capacity + 1 );
    if( array == 0 )
        __xalloc.raise();
    nchars = LoadString( instance, id, array, len );
}

