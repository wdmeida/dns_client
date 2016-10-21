
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include "cliente_dns.h"

int main( int argc , char *argv[])
{
    if(argc != 2){
        printf("Sintaxe:\n<nomePrograma> <enderecoDoSite>\n\n");
        return 0;
    }

    unsigned char hostname[100];

    strcpy(hostname, argv[1]);

    verificaTempoDeCache();

    if(verificaHostEmCache(hostname) == 0)
        //Realiza a pesquisa do host solicitado caso não esteja no cache.
        pesquisaHostPeloNome(hostname , T_A);
    return 0;
}

