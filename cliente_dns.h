#ifndef CLIENTE_DNS_H_INCLUDED
#define CLIENTE_DNS_H_INCLUDED

/*Define os serviços do DNS.  */
#define T_A 1 /*Ipv4 endereço */
#define T_NS 2 /*Servidor de nomes */
#define T_CNAME 5 /* nome canônico */
#define T_SOA 6 /* Inicio da zona de autoridade */
#define T_PTR 12 /* Ponteiro para nome do domínio */
#define T_MX 15 /*Servidor de email */

/*Estrutura do cabeçalho do DNS*/
struct DNS_HEADER{
    unsigned short id; /* número de identificação da mensagem */

    unsigned char rd :1; /*  indica se mensagem recursiva é desejada */
    unsigned char tc :1; /* mensagem truncada */
    unsigned char aa :1; /* resposta de autoridade */
    unsigned char opcode :4; /* tipo de mensagem */
    unsigned char qr :1; /*  flag de pergunta/resposta */

    unsigned char rcode :4; /* codigo da resposta */
    unsigned char cd :1; /* checagem desabilitada*/
    unsigned char ad :1; /* dados de autenticação */
    unsigned char z :1; /* campo reservado */
    unsigned char ra :1; /* mensagem recursiva */

    unsigned short q_count; /* Quantidade de perguntas. */
    unsigned short ans_count; /* Quantidade de respostas. */
    unsigned short auth_count; /* Quantidade de pesquisas de autoridade. */
    unsigned short add_count; /* Quantidades de pesquisas reversas. */
};

/*Campos constantes (não variáveis) da seção de pergunta (question section). */
struct QUESTION{
    unsigned short qtype;
    unsigned short qclass;
};

/*Campos de informações da estrutura da resposta do dns.*/
#pragma pack(push, 1)
struct R_DATA{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

/*Estrutura de registro de resposta do dns.*/
struct RES_RECORD{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

/*Estrutura da pergunta DNS. */
typedef struct{
    unsigned char *name;
    struct QUESTION *ques;
}QUERY;

/*Prototipos das funções utilizadas */
void pesquisaHostPeloNome(unsigned char* , int);
void converteNomeParaFormatoDns(unsigned char* dns,unsigned char* host);
unsigned char* converteNomeDoFormatoDns(unsigned char* reader,unsigned char* buffer,int* contador);
int verificaHostEmCache(const unsigned char *hostName);
void verificaTempoDeCache();
#endif /* CLIENTE_DNS_H_INCLUDED  */
