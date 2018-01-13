/* $config.c$ */
/**
 * @section LICENSE
 * ****************************************************************************
 * Copyright (c)2017 Luca Canessa (516639)                                    *
 *                                                                            *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
*/



/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include <src/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *trim (char * s)
{
    /* Initialize start, end pointers */
    char *s1 = s, *s2 = &s[strlen(s)-1];

    /* Trim and delimit right side */
    while ( isspace( *s2 ) && (s2 > s1) )
    {
        s2--;
    }
    *(s2+1) = '\0';

    /* Trim left side */
    while ( isspace( *s1 ) && ( s1 < s2 ) )
    {
        s1++;
    }

    /* Copy finished string */
    strcpy (s, s1);
    return s;
}


void pars( char *name, struct s_conf *s )
{
    FILE *f;
    if( (f=fopen( name, "r" )) == NULL )
    {
        perror("fopen()");
        fprintf( stderr, "Problem to open file %s\n", name );
    }


    char buff[1024], *str;

    while( fgets( buff, sizeof( buff ), f ) != NULL )
    {
        if(buff[0] == COMMENT || buff[0] == NEW_LINE )
        {
            continue;
        }

        char prm[1024], val[1024];

        str = strtok( buff, TOKEN );
        if( str == NULL )
        {
            continue;
        }
        else
        {
            strncpy( prm, str, sizeof( prm ) );
        }

        str = strtok( NULL, TOKEN );
        if( str == NULL )
        {
            continue;
        }
        else
        {
            strncpy( val, str, sizeof( val ) );
        }

        trim( prm );
        trim( val );


        if( strcmp( prm, "StatFileName" ) == 0 )
        {
            memcpy( s->StatFileName, val, sizeof( s->StatFileName ) );
        }
        if( strcmp( prm, "DirName" ) == 0 )
        {
            memcpy( s->DirName, val, sizeof( s->DirName ) );
        }
        if( strcmp( prm, "UnixPath" ) == 0 )
        {
            memcpy( s->UnixPath, val, sizeof( s->UnixPath ) );
        }



        if( strcmp( prm, "MaxFileSize") == 0 )
        {
            s->MaxFileSize = (int)atoi( val );
        }
        if( strcmp( prm, "MaxConnections") == 0 )
        {
            s->MaxConnections = (int)atoi( val );
        }
        if( strcmp( prm, "ThreadsInPool") == 0 )
        {
            s->ThreadsInPool = (int)atoi( val );
        }
        if( strcmp( prm, "MaxMsgSize") == 0 )
        {
            s->MaxMsgSize = (int)atoi( val );
        }
        if( strcmp( prm, "MaxHistMsgs") == 0 )
        {
            s->MaxHistMsgs = (int)atoi( val );
        }

        str = NULL;
    }
}
