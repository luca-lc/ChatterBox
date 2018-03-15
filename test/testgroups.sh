#!/bin/bash

# registro un po' di nickname
./client -l $1 -c pippo &
./client -l $1 -c pluto &
./client -l $1 -c minni &
./client -l $1 -c topolino &
./client -l $1 -c paperino &
./client -l $1 -c qui &
wait

echo ""
echo ""
echo "1"
# creo il gruppo1
./client -l $1 -k pippo -g gruppo1
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "2"
# creo il gruppo2
./client -l $1 -k pluto -g gruppo2
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "3"
# aggiungo minni al gruppo1
./client -l $1 -k minni -a gruppo1
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "4"
# aggiungo topolino al gruppo1
./client -l $1 -k topolino -a gruppo1
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "5"
# aggiungo paperino al gruppo2
./client -l $1 -k paperino -a gruppo2
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "6"
# aggiungo minni al gruppo2
./client -l $1 -k minni -a gruppo2
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "7"
# pippo manda un messaggio a tutti gli iscritti al gruppo1 (pippo, minni e topolino)
./client -l $1  -k pippo -S "Ciao a tutti da pippo":gruppo1 -R 1
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "8"
# pluto manda un messaggio a tutti gli iscritti al gruppo2 (pluto, paperino e minni)
./client -l $1  -k pluto -S "Ciao a tutti da pluto":gruppo2 -R 1
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "9"
# messaggio di errore che mi aspetto dal prossimo comando
# l'utente qui non puo' mandare il messaggio perche' non e' inscritto al gruppo1
OP_NICK_UNKNOWN=27
./client -l $1  -k qui -S "Ciao sono qui":gruppo1
e=$?
if [[ $((256-e)) != $OP_NICK_UNKNOWN ]]; then
    echo "Errore non corrispondente $e"
    exit 1
fi

echo ""
echo ""
echo "10"
# minni manda un file a tutti gli utenti dei gruppi a cui appartiene
./client -l $1  -k minni -S "Vi mando un file":gruppo1 -s ../lib/libchatty.a:gruppo1 -s ../lib/libchatty.a:gruppo2 -S "Vi ho mandato un file":gruppo2
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "11"
# ricevo i messaggi che mi sono perso
./client -l $1 -k pippo -p
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "12"
./client -l $1 -k pluto -p
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "13"
./client -l $1 -k minni -p
if [[ $? != 0 ]]; then
    exit 1
fi
# sleep 5
echo ""
echo ""
echo "14"
./client -l $1 -k topolino -p
if [[ $? != 0 ]]; then
    exit 1
fi

echo ""
echo ""
echo "15"
./client -l $1 -k topolino -d gruppo1
if [[ $? != 0 ]]; then
    exit 1
fi

echo "16"
./client -l $1 -k minni -d gruppo2
if [[ $? != 0 ]]; then
    exit 1
fi


echo "Test OK!"
exit 0
