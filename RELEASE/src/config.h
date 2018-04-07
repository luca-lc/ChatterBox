/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 * 
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

/******************************************************************************
                                    HEADER
******************************************************************************/

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH                  32    //users name


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* aggiungere altre define qui */

#define COMMENT     '#'             ///< define the symbol that represents the beginning of the comments
#define TOKEN       "="             ///< defines the symbol that divide the config variable from its value
#define NEW_LINE    '\n'            ///< defines the symbol that represent a new line in the file



/**
 * @struct                  struct s_conf
 * @brief                   Defines the server configuration variables
 * @var     StatFileName    Variable where the path of the server log file was saved (max 255 charater)
 * @var     DirName         Variable where the path of the folder is saved to save the files that the clients send (max 255 characters)
 * @var     UnixPath        Variable where the path to socket is saved (max 255 characters)
 * @var     MaxFileSize     Variable to define the max file size (in kilobyte)
 * @var     MaxConnections  Variable to define the max connections that the server can handle
 * @var     ThreadsInPool   Variable to define the number of threads in the pool
 * @var     MaxMsgSize      Variable to define the max length of text message that can be sent
 * @var     MaxHistMsgs     Variable to define the number of messages that the server can save (messages history)
 */
struct s_conf
{
    char    StatFileName[256];
    char    DirName[256];
    char    UnixPath[256];
    int     MaxFileSize;
    int     MaxConnections;
    int     ThreadsInPool;
    int     MaxMsgSize;
    int     MaxHistMsgs;
};


/******************************************************************************
                                    FUNCTIONS
******************************************************************************/
/**
 * @function              pars
 * @brief           Prende il file di configurazione e lo legge. Per ogni 
 *                  opzione conosciuta, selezionare il suo valore e lo salva 
 *                  nella variabile del server appropriata.
 * @param   name    Pointer to path to the config file
 * @param   s       Pointer to 's_conf' struct (see 's_conf' description for 
 *                  details)
 */
void pars( char *name, struct s_conf *s );


// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
