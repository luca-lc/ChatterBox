/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH                  32    //users name

/* aggiungere altre define qui */

#define COMMENT     '#'
#define TOKEN       "="
#define NEW_LINE    '\n'


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


void pars( char *name, struct s_conf *s );


// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
