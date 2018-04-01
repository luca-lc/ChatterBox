/** $config_c$ **/

/**
 * \section LICENSE
 * ****************************************************************************
 * \author Luca Canessa (516639)                                              *
 *                                                                            *
 * \copyright \n                                                              *
 * Declares that all contents of this file are author's original operas       *
 *                                                                            *
 ******************************************************************************
 *
 * \section DESCRIPTION
 * Looking overview
 * 
 * trim()   :   Takes a string (of course of type char *) and parse its 
 *              contents. Removes each's space found in it, so at the end 
 *              it can returns only word inside it without other characters
 *              To find the spaces uses 'isspace()' function declred in 
 *              'ctype.h'.
 *              Requires the pointer to the string to trim.
 *              Returns the trimmed word.
 * 
 * pars()   :   Opens the file passed in read-only mode. Read each line and 
 *              check that it is not a comment or a blank line (new line '\n').
 *              Then it divides the configuration option from its value. For 
 *              each known option, save the value in the appropriate server 
 *              configuration variable.
 *              Requires the pointer to the configuration file path and pointer
 *              to the 'struct s_conf' structure used to save the configuration 
 *              variables (see 'struct s_conf' for details)
*/



/******************************************************************************
 	 	 	 	 	 	 	 	 	 HEADER
******************************************************************************/
#include <src/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



/******************************************************************************
                                    FUNCTIONS
******************************************************************************/
/**
 * \fn          trim
 * \brief       Removes space from a string
 * \param   s   Pointer to string (of type 'char *') to trim
 * \return  s   Pointer to the trimmed string
 */
char *trim ( char *s )
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
    char st[strlen(s)];
    memset( st+0, 0, strlen(s) );
    strcpy(st, s1);
    memset( s+0, 0, strlen( s ) );
    strcpy(s, st);
    return s;
}



/**
 * \fn              pars
 * \brief           Opens the file passed in read-only mode. Read each line and 
 *                  check that it is not a comment or a blank line (new line '\n').
 *                  Then it divides the configuration option from its value. For 
 *                  each known option, save the value in the appropriate server 
 *                  configuration variable.
 * \param   name    Pointer to configuration file path
 * \param   s       Pointer to 's_conf' structure
 */
void pars( char *name, struct s_conf *s )
{
    FILE *f;
    if( (f=fopen( name, "r" )) == NULL )
		 {
			  perror( "fopen()" );
			  fprintf( stderr, "Problem to open file %s\n", name );
			  exit( EXIT_FAILURE );
		 }


    char buff[1024], *str;

    while( fgets( buff, sizeof( buff ), f ) != NULL )
    {
        if( buff[0] == COMMENT || buff[0] == NEW_LINE )
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

    fclose( f );
}
