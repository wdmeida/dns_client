#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

#include<sys/socket.h>   /*Funções e constantes relativas a sockets.*/
#include<arpa/inet.h> /*Definições para funções destinadas a operações com sockets.*/
#include<netinet/in.h>  /*Utilizada para manipulação de estruturas, dados e endereços da família internet.*/
#include<unistd.h>    /*Recuperar informações geradas pelo sistema (getpid())*/

#include "cliente_dns.h"

/*
 Realiza uma pesquisa em um servidor dns pelo host inserido pelo usuário. Antes de pesquisar na
  internet verifica no cache, caso encontre, retorna os endereços. Caso não encontre no cache, faz
  a pesquisa a um servidor dns na internet.
 */
void pesquisaHostPeloNome(unsigned char *host , int query_type)
{
    /*Cria as variáveis de controle que serão utilizadas pelo programa.
      buf = buffer de entrada e saída de dados.
      *qname = aponta para a qname na seção de pergunta (question section).
      *answerDNS = aponta para a seção de resposta do dns (answer section).
    */
    unsigned char buf[65536],*qname, *answerDNS;
    char dns_servers[15];
    int i , j , stop , s;

    /*Estruturas utilizadas pelo socket para gerenciar chamadas de sistema e de funções que lidam com endereços de Internet
      do protocolo IPv4.
    */
    struct sockaddr_in a;
    struct sockaddr_in dest;

    /*Estruturas que receberão as respostas do servidor dns.*/
    struct RES_RECORD answers[20],auth[20],addit[20];

    /*Estruturas de cabeçalho e de pergunta do dns.*/
    struct DNS_HEADER *headerDNS = NULL;
    struct QUESTION *questionDNS = NULL;

    printf("Pesquisando %s" , host);
    //Define o Ip do DNS que será utilizado para pesquisa.
    strcpy(dns_servers, "8.8.8.8");

    /*Cria um socket UDP para fazer a pesquisa DNS.*/
    s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);

    /*Configura uma das estruturas utilizadas pelo socket com as informações para pesquisa.*/
    dest.sin_family = AF_INET; /* Configura o tipo de conexão.*/
    dest.sin_port = htons(53); /* Configura a porta.*/
    dest.sin_addr.s_addr = inet_addr(dns_servers); /*Configura o endereço do dns.*/

    /*Inicializa o ponteiro para cabeçalho DNS com o endereço do buffer. (Utilizando o casting, pois buf é um unsigned char).*/
    headerDNS = (struct DNS_HEADER *)&buf;

    /*Define o valor padrão dos campos do cabeçalho de uma requisição DNS.*/
    headerDNS->id = (unsigned short) htons(getpid());
    headerDNS->qr = 0; /*Operacao (Requisição ou resposta). */
    headerDNS->opcode = 0; /*Tipo de requisição*/
    headerDNS->aa = 0; /*Resposta de autoridade*/
    headerDNS->tc = 0; /*Mensagem truncada*/
    headerDNS->rd = 1; /*Recursão desejada*/
    headerDNS->ra = 0; /*Informa se a recursão não for possível.*/
    headerDNS->z = 0;  /*Valor reservado,sempre nulo*/
    headerDNS->ad = 0; /*Valor reservado,sempre nulo*/
    headerDNS->cd = 0; /*Valor reservado,sempre nulo*/
    headerDNS->rcode = 0; /*Código da resposta.*/
    headerDNS->q_count = htons(1); /*Quantidade de perguntas enviadas.*/
    headerDNS->ans_count = 0; /*Quantidade de respostas recebidas.*/
    headerDNS->auth_count = 0; /*Quantidade de respostas de autoridade recebidas.*/
    headerDNS->add_count = 0; /*Quantidade de informações adicionais recebidas.*/

    /*Faz qname referenciar a parte da consulta.*/
    qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];

    converteNomeParaFormatoDns(qname , host);

    /*Faz questionDNS referenciar os campos de tamanho estático da seção de pergunta.*/
    questionDNS =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)];

    /*A função htons, converte um valor especificado converter um número expresso em Host Byte Order
    (no qual o primeiro byte é o mais significativo) para Network Byte Order (primeiro byte menos significativo).
    Isto é necessário por questões de compatibilidade.*/
    questionDNS->qtype = htons(query_type); /*Tipo de pergunta a ser realizada, A , MX , CNAME , NS, etc. */
    questionDNS->qclass = htons(1); /* Classe: IN, CH, HS, etc. 1 = Internet (IN)*/

    printf("\nEnviando a pergunta...");

    /*A função sendto é a responsavel por fazer a requisição de envio de msg para o servidor DNS. Ela recebe os
      seguintes parâmetros.
      1 - O socket para envio dos dados.
      2 - Um ponteiro para o buffer onde estão os dados que serão enviados.
      3 - O tamanho dos dados a serem enviados em bytes.
      4 - Flag de envio - (segundo documentação o valor default é 0).
      5 - Um ponteiro para uma estrutura socketaddr que será preenchida com as informações da maquina que envia os dados.
      6 - Tamanho em bytes da estrutura socketaddr que será preenchida.
     Se a operação for bem sucedida a função retorna o número de caracteres enviados, e um valor negativo, se ocorrer algum erro.
    */
    if(sendto(s, (char*)buf, sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION), 0, (struct sockaddr*)&dest,sizeof(dest)) < 0)
    {
        perror("Falha no envio");
    }
    printf("Enviado");

    /*Inicia o contador "i" com o tamanho da estrutura de destino, onde os dados do buffer foram copiados.*/
    i = sizeof dest;

    printf("\nRecebendo resposta...");

    /*A função recvfrom é responsável por receber a resposta do servidor. Ela recebe os sequintes parâmetros.
      1 - O socket para recebimento dos dados.
      2 - Um ponteiro para o buffer onde os dados recebidos serão armazenados.
      3 - O tamanho máximo dos dados que podem ser recebidos.
      4 - Um flag - (segundo documentação o valor default é 0).
      5 - Um ponteiro para uma estrutura socketaddr a partir do qual os dados são recebidos.
      6 - Tamanho em bytes da estrutura socketaddr que recebe os dados.
       Se a operação for bem sucedida a função retorna o tamanho em bytes do datagrama. Se é recebido um sinal de fim de arquivo
       ou a conexão é interrompida retorna 0. Se ocorrer algum erro, a função retorna um valor negativo.
    */
    if(recvfrom(s, (char*)buf, 65536, 0, (struct sockaddr*)&dest, (socklen_t*)&i ) < 0)
    {
        perror("Falha na resposta");
    }
    printf("Recebido");

    /*Inicializa o ponteiro para cabeçalho DNS (agora com um cabeçalho de resposta) com o endereço do buffer.*/
    headerDNS = (struct DNS_HEADER*) buf;

     /*Faz answerDNS referenciar a seção de respostas.*/
    answerDNS = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];

    printf("\nCampos contidos na resposta: ");
    printf("\n %d Perguntas.",ntohs(headerDNS->q_count));
    printf("\n %d Respostas.",ntohs(headerDNS->ans_count));
    printf("\n %d Servidores de Autoridade.",ntohs(headerDNS->auth_count));
    printf("\n %d Respostas Adicionais.\n\n",ntohs(headerDNS->add_count));

    /*O código de resposta diferente de zero significa um erro na requisição DNS.*/
    if(headerDNS->rcode != 0){
        printf("\nErro na requisicao: ");
        switch(headerDNS->rcode){
            case 1:printf("Erro de formato, servidor incapaz de interpretar a requisicao.");break;
            case 2:printf("Falha do servidor.");break;
            case 3:printf("Dominio inexistente");break;
            case 4:printf("Servidor DNS não oferece suporte a este tipo de requisicao.");break;
            case 5:printf("Requisicao recusada pelo servidor.");break;
        }/*switch*/
        printf("\n\n");
    }/*if*/

    /*Começa a ler as respostas*/
    stop = 0;

    /*Verifica quantas respostas foram obtidas na pesquisa, para a interação no loop. Cada resposta
      será indexada de acordo com a quantidade da mesma, por exemplo resposta 1 na posição 0 e caso
      hajam mais respostas, serão indexadas nas posições seguintes.*/
    for(i = 0; i < ntohs(headerDNS->ans_count); i++)
    {
        /*Para converter os dados recebidos, é utilizado a função converteNomeDoFormatoDns. A função
          recebe uma struct com o tamanho pré fixado, o buffer com os dados recebidos na consulta e um
          flag para contar a quantidade de vezes que os dados precisarão ser divididos dentro da função
          e controlar sinalizar onde termina um campo e começa o pŕoximo (inserido depois o caracter '\0').
          Como a maioria dos campos do dns tem tamanho pré fixado, a cada quebra da mensagem o campo com
          tamanho pré fixado é incrementado ao tamanho do proximo salto. Os campos com tamanho variaveis
          possuem um campo com o tamanho que será usado para separar a mensagem.*/
        answers[i].name = converteNomeDoFormatoDns(answerDNS,buf,&stop);
        answerDNS = answerDNS + stop;

        answers[i].resource = (struct R_DATA*)(answerDNS);
        answerDNS = answerDNS + sizeof(struct R_DATA);

        /* Verifica se é um endereço IPv4.*/
        if(ntohs(answers[i].resource->type) == T_A)
        {
            //Aloca espaço com o tamanho necessário para os campos de informações de resposta do dns.
            answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));

            //Copia os os dados para as posições.
            for(j = 0 ; j<ntohs(answers[i].resource->data_len) ; j++)
                answers[i].rdata[j] = answerDNS[j];

            //Insere um caracter nulo na ultima posição para indicar termino.
            answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

            //Calcula o valor do próximo salto para o proximo campo.
            answerDNS = answerDNS + ntohs(answers[i].resource->data_len);
        }
        else
        {
            //Caso não seja um endereço valido (internet), apenas preenche os campos da mensagem.
            answers[i].rdata = converteNomeDoFormatoDns(answerDNS,buf,&stop);
            answerDNS = answerDNS + stop;
        }
    }

    /*Lê e converte as respostas de autoridade. */
    for(i = 0; i < ntohs(headerDNS->auth_count); i++)
    {
        auth[i].name = converteNomeDoFormatoDns(answerDNS,buf,&stop);
        answerDNS += stop;

        auth[i].resource = (struct R_DATA*)(answerDNS);
        answerDNS += sizeof(struct R_DATA);

        auth[i].rdata = converteNomeDoFormatoDns(answerDNS,buf,&stop);
        answerDNS += stop;
    }

    /*Lê e converte as respostas adicionais.*/
    for(i = 0; i < ntohs(headerDNS->add_count); i++)
    {
        addit[i].name = converteNomeDoFormatoDns(answerDNS,buf,&stop);
        answerDNS += stop;

        addit[i].resource = (struct R_DATA*)(answerDNS);
        answerDNS += sizeof(struct R_DATA);

        /* Verifica se é um endereço IPv4.*/
        if(ntohs(addit[i].resource->type) == T_A)
        {
            addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len));
            for(j = 0; j < ntohs(addit[i].resource->data_len); j++)
            addit[i].rdata[j] = answerDNS[j];

            addit[i].rdata[ntohs(addit[i].resource->data_len)] = '\0';
            answerDNS += ntohs(addit[i].resource->data_len);
        }
        else
        {
            addit[i].rdata = converteNomeDoFormatoDns(answerDNS,buf,&stop);
            answerDNS += stop;
        }
    }

    /*Cria um ponteiro para arquivo e salva o conteúdo em disco.*/
    FILE *arqSaida;
    arqSaida = fopen("DnsResultados.txt","a+");
    fprintf(arqSaida,"%s\n",host);

    /*Imprime as respostas na tela e no arquivo de saída.*/
    printf("\nRespostas : %d" , ntohs(headerDNS->ans_count));
    fprintf(arqSaida,"\n%d\n",ntohs(headerDNS->ans_count));

    /* Inicializa um contador de de 0 até a quantidade de respostas recebidas.*/
    for(i=0 ; i < ntohs(headerDNS->ans_count) ; i++)
    {
        printf("\nNome : %s ",answers[i].name);
        fprintf(arqSaida,"URL: %s ",answers[i].name);

        /*Verifica se é um endereço IPv4.*/
        if( ntohs(answers[i].resource->type) == T_A)
        {
            long *p;
            //Obtem o endereço IP.
            p = (long*)answers[i].rdata;
            a.sin_addr.s_addr = (*p);
            printf("Endereço IPv4 : %s",inet_ntoa(a.sin_addr));
            fprintf(arqSaida," IP: %s\n",inet_ntoa(a.sin_addr));
        }

        if(ntohs(answers[i].resource->type) == T_CNAME) /*Verifica se é um nome canônico*/
        {
            /*Nome canônico para apelido.*/
            printf("Nome canônico : %s",answers[i].rdata);
            fprintf(arqSaida,"%s\n",answers[i].rdata);
        }
    }

    /*Imprime as respostas de autoridade.*/
    printf("\nRespostas de Autoridade : %d\n" , ntohs(headerDNS->auth_count) );
    fprintf(arqSaida,"%d\n",ntohs(headerDNS->auth_count));
    for(i = 0 ; i < ntohs(headerDNS->auth_count); i++)
    {

        printf("Nome : %s ",auth[i].name);
        fprintf(arqSaida,"URL: %s ",auth[i].name);
        if(ntohs(auth[i].resource->type) == 2)
        {
            printf("Nome do Servidor : %s",auth[i].rdata);
            fprintf(arqSaida,"Nome do Servidor: %s\n",auth[i].name);
        }
        printf("\n");
    }

    /*Imprime as respostas adicionais */
    printf("\nRespostas Adicionais : %d \n", ntohs(headerDNS->add_count) );
    fprintf(arqSaida,"%d\n",ntohs(headerDNS->auth_count));
    for(i = 0; i < ntohs(headerDNS->add_count); i++)
    {
        printf("Nome : %s ",addit[i].name);
        fprintf(arqSaida,"URL: %s ",addit[i].name);
        if(ntohs(addit[i].resource->type) == 1)
        {
            long *p;
            p = (long*)addit[i].rdata;
            a.sin_addr.s_addr = (*p);
            printf("Endereço IPv4: %s",inet_ntoa(a.sin_addr));
            fprintf(arqSaida,"IP: %s \n",inet_ntoa(a.sin_addr));
        }
        printf("\n");
    }
    fprintf(arqSaida,"\n\n---\n");
    fclose(arqSaida);
    return;
}/*pesquisaHostPeloNome*/

/*
  Converte o nome recebido pela pesquisa. O nome será recebido em em binário, convertido para
  o formato do dns (com quantidade dos caracteres subsequentes no lugar dos pontos) e depois
  para o formato de normal de dominio (com os pontos no local).
  Por exemplo:

        3www6google3com para www.google.com.

 Os parâmetros da função são:
  reader = uma struct com o tamanho pré fixado
  buffer = variável com os dados recebidos na consulta
  contador = flag para contar a quantidade de vezes que os dados precisarão ser divididos dentro da função,
             controlar sinalizar onde termina um campo e começa o pŕoximo (inserido depois o caracter '\0').

  Como a maioria dos campos do dns tem tamanho pré fixado, a cada quebra da mensagem o campo com
  tamanho pré fixado é incrementado ao tamanho do proximo salto. Os campos com tamanho variaveis
  possuem um campo com o tamanho que será usado para separar a mensagem.
*/
u_char* converteNomeDoFormatoDns(unsigned char* reader,unsigned char* buffer,int* contador)
{
    unsigned char *nome;
    unsigned int p = 0, jumped = 0, saida;
    int i, j;

    *contador = 1;
    /*Aloca espaço em memoria para armazenar o nome (maximo do campo é 256 caracteres).*/
    nome = (unsigned char*)malloc(256);

    nome[0] = '\0';

    /*Verifica se o campo recebido não esta vazio.*/
    while(*reader != 0)
    {   /*Verifica se o valor do campo é maior do que 192 (o que indica que ainda precisa ser convertido).*/
        if(*reader >= 192)
        {
            /*Realiza a conversão do valor para seu um valor padrão da tabela ASCII.*/
            saida = (*reader)*256 + *(reader+1) - 49152; /*49152 = 11000000 00000000*/
            reader = buffer + saida - 1;
            jumped = 1; /*inicializa o flag para saber que houve uma quebra.*/
        }
        else
        {
            nome[p++]= *reader; /*Se o valor for menor, inicia a posição do nome com o respectivo valor (em ASCII).*/
        }

        reader = reader + 1;

        if(jumped == 0)
        {
            *contador = *contador + 1; /*Verifica se o valor não precisou ser convertido, caso não tenha incrementa o flag.*/
        }
    }

    nome[p] = '\0'; /*string completa.*/
    if(jumped == 1)
    {
        *contador = *contador + 1; /*Aumenta o valor da váriavel para informar que houve quebra de pacote.*/
    }
    /*Converte o nome do formato dns para formato normal (com pontos no lugar dos números).*/
    for(i = 0; i < (int)strlen((const char*)nome); i++)
    {
        p = nome[i];
        for(j = 0; j < (int)p; j++)
        {
            nome[i] = nome[i+1];
            i = i+1;
        }
        nome[i] = '.';
    }
    nome[i-1] = '\0'; /*Remove o ponto final da string.*/
    return nome;
}/*converteNomeDoFormatoDns()*/

/*Converte o host a ser pesquisado para o formato utilizado para pesquisa do dns.
  Substitui todos os '.' do dominio pelo valor correspondentes de caracteres que
  possuem antes do ponto.
  Por exemplo:

        www.google.com para 3www6google3com.
*/
void converteNomeParaFormatoDns(unsigned char* dns,unsigned char* host)
{
    unsigned int lock = 0,i;

    /*Concatena o host a ser pesquisado com um ponto. */
    strcat((char*)host,".");

    /*Percorre a string. */
    for(i = 0 ; i < strlen((char*)host) ; i++)
    {
        if(host[i]=='.')
        {
            /*Verifica quantos caracteres foram deslocados antes do ponto e atribui esta quantidade
              no lugar do ponto.*/
            *dns++ = i-lock;
            for(;lock<i;lock++)
            {
                /*Copia os caracteres anteriores para a string auxiliar.*/
                *dns++ = host[lock];
            }
            lock++;
        }
    }
    *dns++='\0'; /*insere um caracter nulo para informar que terminou a cópia.*/
}/*converteNomeParaFormatoDns()*/


/* Verfica se o host fornecido pelo usuario se encontra em cache.
   Recebe o endereço fornecido pelo usuário.
   Retorna 1 e exibe na tela informações em cache se encontrar as informações, 0 se não encontrar.
*/
int verificaHostEmCache(const unsigned char *hostName){
    FILE *arqSaida;/*Arquivo Texto*/
    arqSaida = fopen("DnsResultados.txt","r");/*Abre o arquivo texto.*/
    /*Verfica se o arquivo foi aberto com sucesso.*/
    if(arqSaida == NULL) return 0;
    /*variaveis para receber os dados do arquivo.*/
    char host[100],URL[100],nome[100],MIP[100],ip[100];
    /*le do arquivo a URL.*/
    fscanf(arqSaida,"%s",host);
    /*Obtem o tamanho da URL.*/
    int tamanho = (int)strlen(host);

    int numerosDeIp,indice;
     char NovoHost[tamanho];
    strcpy(NovoHost,hostName);
    /*Adiciona ponto no final da URL pasada por parametro para comparaçao.*/
    strcat(NovoHost,".");
        /*feof verifica se chegou ao final do arquivo.*/
        while(!feof(arqSaida)){
            /*Compara se o host e igual ao passado por parametro, caso for recupera
            os dados do arquivo e exibe no console.*/
            if(!strcmp(NovoHost,host)){
                /*Obtem o numero de Ips no arquivo.*/
                fscanf(arqSaida,"%d",&numerosDeIp);
                /*Exibe a url.*/
                if(numerosDeIp == 0)return 0;
                printf("\n%s\n\n",hostName);
                /*Loop para obter os Ips da URL no arquivo*/
                for(indice = 0; indice < numerosDeIp;indice++){
                    /*Le do arquivo.*/
                    fscanf(arqSaida,"%s %s %s %s",URL,nome,MIP,ip);
                    /*Exibe os dados lidos do arquivo.*/
                    printf("%s %s %s %s\n",URL,nome,MIP,ip);
                }
                printf("\n\n");
                return 1;
            }else{
                fscanf(arqSaida,"%d",&numerosDeIp);
                for(indice = 0; indice < numerosDeIp;indice++)
                    fscanf(arqSaida,"%s %s %s %s",URL,nome,MIP,ip);
            }
            fscanf(arqSaida,"%d",&numerosDeIp);
            fscanf(arqSaida,"%d",&numerosDeIp);
            /*Le o proxima URL*/
            fscanf(arqSaida,"%s",host);
            /* caso fo igual ler novamente o proximo campo do arquivo.*/
            if(!strcmp(host,"---"))
                fscanf(arqSaida,"%s",host);
        }
    /*fecha arquivo.*/
    fclose(arqSaida);
    printf("\n\n");
    return 0;
}/*verificaHostEmCache()*/

/*Verifica se o tempo do cache expirou, caso expirou ele apaga o conteudo do cache
  e atualiza o arquivo de configuraçao com o horario da ultima verificaçao.
*/
void verificaTempoDeCache(){
    //Cria uma variavel para acessar o arquivo de configuraçao.
    FILE *arqConfiguracao;

    //Abre o arquivo de configuraçao em modo so leitura.
    arqConfiguracao = fopen("configuracaoDNS.config","r");

    //verfica se o arquivo foi aberto corretamente caso nao foi ele termina a execuçao da funçao.
    if(arqConfiguracao == NULL) return;

    int tempo, horaArquivo,minArquivo, secArquivo;

    //Le os dados do arquivo de configuraçao.
    fscanf(arqConfiguracao,"%d %d %d %d",&tempo,&horaArquivo,&minArquivo,&secArquivo);

    //fecha o arquivo de configuraçao.
    fclose(arqConfiguracao);


    int segundosArquivo, segundosSistema;

    //Define a variavel do tipo tm para pegar a data do sistema.
    struct tm *novoTempo;

    //Variavel do tipo time_t para obter a hora do sistema.
    time_t t;

    //Obtem a hora do sistema.
    t = time(NULL);

    //atribui a data do sistema para a variavel novoTempo.
    novoTempo = localtime(&t);

    //Transforma o tempo obtido do arquivo em segundos.
    segundosArquivo = (horaArquivo*3600) + (minArquivo*60) + secArquivo;

    //Transforma o tempo obtido do sistema em segundos.
    segundosSistema = (novoTempo->tm_hour*3600)+(novoTempo->tm_min*60)+novoTempo->tm_sec;

    //calcula a diferenca do tempo do sistema com o do arquivo.
    int diferenca = segundosSistema - segundosArquivo;

    //Verifica se a dirença e maior que o tempo obtido do arquivo.
    if(diferenca > tempo || diferenca < (tempo*-1)){
        //Abre o arquivo de configuraçao para atualizar o arquivo.
        arqConfiguracao = fopen("configuracaoDNS.config","rw+");

        //Salva a hora obtida do sistema no arquivo e o fecha em seguida.
        fprintf(arqConfiguracao,"%d\n %d %d %d",tempo,novoTempo->tm_hour,novoTempo->tm_min,novoTempo->tm_sec);
        fclose(arqConfiguracao);

        //Cria um variavel para acessar o arquivo de cache do dns.
        FILE *arqSaida;
        //Abre o arquivo de cache para apagar o conteudo nele.
        arqSaida = fopen("DnsResultados.txt","w+");//Abre o arquivo texto.
        //Verifica se o arquivo foi aberto com sucesso.
        if(arqSaida==NULL)return;
        //Sobrescreve o conteudo atual do arquivo com um " ".
        fscanf(arqSaida," ");
        //Fecha o arquivo.
        fclose(arqSaida);
    }
}/*verificaTempoEmCache()*/
