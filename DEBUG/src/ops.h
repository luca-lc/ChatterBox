/*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

#if !defined(OPS_H_)
#define OPS_H_

/**
 * @file  ops.h
 * @brief Contiene i codici delle operazioni di richiesta e risposta
 */


typedef enum {

    /* ------------------------------------------ */
    /*      operazioni che il server deve gestire */
    /* ------------------------------------------ */
    REGISTER_OP      = 0,   /// richiesta di registrazione di un ninckname
    CONNECT_OP       = 1,   /// richiesta di connessione di un client
   
    POSTTXT_OP       = 2,   /// richiesta di invio di un messaggio testuale ad un nickname o groupname
    POSTTXTALL_OP    = 3,   /// richiesta di invio di un messaggio testuale a tutti gli utenti
    POSTFILE_OP      = 4,   /// richiesta di invio di un file ad un nickname o groupname
    
	GETFILE_OP       = 5,   /// richiesta di recupero di un file
    GETPREVMSGS_OP   = 6,   /// richiesta di recupero della history dei messaggi
    
	USRLIST_OP       = 7,   /// richiesta di avere la lista di tutti gli utenti attualmente connessi
    
	UNREGISTER_OP    = 8,   /// richiesta di deregistrazione di un nickname o groupname
    DISCONNECT_OP    = 9,   /// richiesta di disconnessione

    CREATEGROUP_OP   = 10,  /// richiesta di creazione di un gruppo
    ADDGROUP_OP      = 11,  /// richiesta di aggiunta ad un gruppo
    DELGROUP_OP      = 12,  /// richiesta di rimozione da un gruppo

    /* NOTA: la richiesta di cancellazione di un gruppo e' lasciata come task opzionale */

    /* 
     * aggiungere qui eltre operazioni che si vogliono implementare 
     */

    /* ------------------------------------------ */
    /*    messaggi inviati dal server             */
    /* ------------------------------------------ */

   OP_OK           = 20,  /// operazione eseguita con successo
   
   TXT_MESSAGE     = 21,  /// notifica di messaggio testuale
   FILE_MESSAGE    = 22,  /// notifica di messaggio "file disponibile"
   
   OP_FAIL         = 25, /// generico messaggio di fallimento
  
   OP_NICK_ALREADY = 26, /// nickname o groupname gia' registrato
   OP_NICK_UNKNOWN = 27, /// nickname non riconosciuto
   OP_MSG_TOOLONG  = 28,  	/// messaggio con size troppo lunga
   OP_NO_SUCH_FILE = 29,  	/// il file richiesto non esiste
    

	/*
	* aggiungere qui altri messaggi di ritorno che possono servire
	*/

	OP_ALREADY_ONLINE 	= 30,		///user with nickname already online
	OP_NICK_AVAILABLE 	= 31,		///nickname already available in that group
	OP_GROUP_UNKNOWN 		= 32, 		///group not available

	OP_TOO_MANY_CONN 		= 33, 		///too many connection in server
	OP_UNKNOWN 				= 34, 				///unrecognized operation
	OP_RM_USR_G 			= 35,				///remove user from group

	GETPREVGROUPMSGS_OP 	= 36,	/// richiesta di recupero della history dei messaggi di un gruppo

	MSG_NDELIV_SOMEUSERS = 37,

	OP_END          		= 100 /// limite superiore agli id usati per le operazioni

} op_t;
    

#endif /* OPS_H_ */
