// serveur.c
#include "pse.h"

#define CMD           "serveur"
#define NOM_JOURNAL   "journal.log"
#define MAX_CLIENTS   5


int fdJournal;
int i_question = 0;
client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//fonction qui permet de liberer les maillons de la liste chainee contenant les threads en cours de travail
void supprimerDataThread(DataThread *dataThreadSuppr) {
  	DataThread *maillon;
  	DataThread *prec;

  	maillon = listeDataThread;
  	prec = NULL;

  	while (maillon != NULL && maillon != dataThreadSuppr) {
    		prec = maillon;
    		maillon = maillon->next;
  	}

  	if (maillon == NULL)
    		printf("maillon non trouve\n");
  	else {
    		if (prec == NULL)
      			listeDataThread = maillon->next;
    		else
      			prec->next = maillon->next;
    		free(maillon);
  	}
}

//fonction qui prend un message et un socket en argument et envoie ce meme message aux clients
void envoyer_message(char *message, int sock) {
    	pthread_mutex_lock(&clients_mutex);
    	for (int i = 0; i < MAX_CLIENTS; ++i) {
        	if (clients[i]) {
            		if (clients[i]->socket != sock) {
                		if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                    			perror("send failed");
                    			continue;
                		}
            		}
        	}
    	}
    	pthread_mutex_unlock(&clients_mutex);
}

//fonction qui envoie une question aux clients a l'aide de l'indice de la question en argument
void diffuser_question(int indice_question) {
    	char message[LIGNE_MAX];
    	snprintf(message, sizeof(message), "Question %d: %s\n", indice_question + 1, questions[indice_question]);
    	envoyer_message(message, -1);
}

//fonction qui supprime tous les clients
void suppr_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->socket == sock) {
                clients[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//fonction qui ajoute un nouveau client a la liste des clients
void ajouter_client(client_t *cl) {
    	pthread_mutex_lock(&clients_mutex);
    	for (int i = 0; i < MAX_CLIENTS; ++i) {
        	if (!clients[i]) {
            		clients[i] = cl;
            		break;
        	}
    	}
    	pthread_mutex_unlock(&clients_mutex);
}

void remiseAZeroJournal(void) {
  if (close(fdJournal) == -1)
    erreur_IO ("fermeture jornal pour remise a zero");

  fdJournal = open(NOM_JOURNAL, O_TRUNC|O_WRONLY|O_APPEND, 0600);
  if (fdJournal == -1)
    erreur_IO ("ouverture journal pour remise a zero");
}

//fonction qui simule une partie pour chaque client
void sessionClient(arg_t *ar) {
  	int fin = FAUX;
  	char ligne[LIGNE_MAX];
  	int cnl = ar->dataThr->spec.canal;
  	client_t *clnt = ar->client;
  	
  	// Reception du nom du client
    	if (lireLigne(cnl, ligne) <= 0) {
        	printf("Veuillez entrer correctement votre nom.\n");
        	fin = VRAI;
    	}
    	else {
        	strncpy(clnt->nom, ligne, sizeof(clnt->nom));
        	clnt->nom[strcspn(clnt->nom, "\n")] = '\0';
        	printf("%s a rejoint la partie.\n", clnt->nom);
        	snprintf(ligne, sizeof(ligne), "%s a rejoint la partie.\n", clnt->nom);
        	envoyer_message(ligne, clnt->socket);
    	}
    	
    	bzero(ligne, LIGNE_MAX);
    	
    	
	
  	while (!fin && i_question < NB_QUESTIONS) {
    		diffuser_question(i_question);
    		int lgLue = lireLigne(cnl, ligne);
    		if (lgLue > 0) {
    			ligne[strcspn(ligne, "\n")] = '\0';
    			
    			if (strcasecmp(ligne, reponses[i_question]) == 0) { //reponse correcte
    				clnt->score++;
    				snprintf(ligne, sizeof(ligne), "%s a repondu correctement !\n", clnt->nom);
    				pthread_mutex_lock(&clients_mutex);
    				i_question ++;
    				pthread_mutex_unlock(&clients_mutex);
    				
    				
            		}
            		
    			else { //reponse incorrecte
                		snprintf(ligne, sizeof(ligne), "%s a repondu incorrectement.\n", clnt->nom);
            		}
            		
            		envoyer_message(ligne, clnt->socket);
            		
            	}
            	else if (lgLue == 0 || strcmp(ligne, "fin") == 0) {
            		printf("%s a quitte la partie.\n", clnt->nom);
            		fin = VRAI;
            	}
            	else if (strcmp(ligne, "init") == 0) {
      			printf("serveur: remise a zero du journal\n");
      			remiseAZeroJournal();
    		}
            	else {
            		printf("Error.\n");
            		fin = VRAI;
            	}
            	bzero(ligne, LIGNE_MAX);
       }
       
       snprintf(ligne, sizeof(ligne), "\nPartie terminée !\n");
       envoyer_message(ligne, -1);
       
       //affichage des scores
       for (int i=0; i < MAX_CLIENTS; i++) {
       		if (clients[i]) {
       			snprintf(ligne, sizeof(ligne), "%s : score final : %d\n", clients[i]->nom, clients[i]->score);
      
       			envoyer_message(ligne, -1);
       		}
       }
       
       if (close(cnl) == -1)
       		erreur_IO("fermeture canal");
       suppr_client(clnt->socket);
       
       
}

//fonction executee dans le thread du main, execute sessionClient
void *threadSessionClient(void *arg) {
  	
  	arg_t *argument = (arg_t *)arg;
  	sessionClient(argument);
  	supprimerDataThread(argument->dataThr);

  	pthread_exit(NULL);
}







int main(int argc, char *argv[]) {
  	short port;
  	int ecoute, canal, ret;
  	struct sockaddr_in adrEcoute, adrClient;
  	unsigned int lgAdrClient;
  	DataThread *dataThread;
  	
  	if (argc != 2)
    		erreur("usage: %s port\n", argv[0]);
    	
    	fdJournal = open(NOM_JOURNAL, O_CREAT|O_WRONLY|O_APPEND, 0600);
  	if (fdJournal == -1)
    		erreur_IO("ouverture journal");
    	
    	port = (short)atoi(argv[1]);
    	
    	initDataThread();
    	
    	printf("%s: creating a socket\n", CMD);
  	ecoute = socket (AF_INET, SOCK_STREAM, 0);
  	if (ecoute < 0)
    		erreur_IO("socket");
    	
    	adrEcoute.sin_family = AF_INET;
  	adrEcoute.sin_addr.s_addr = INADDR_ANY;
  	adrEcoute.sin_port = htons(port);
  	printf("%s: binding to INADDR_ANY address on port %d\n", CMD, port);
  	ret = bind (ecoute,  (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
  	if (ret < 0)
    		erreur_IO("bind");
    	
    	printf("%s: listening to socket\n", CMD);
  	ret = listen (ecoute, 5);
  	if (ret < 0)
    		erreur_IO("listen");
    	
    	while (VRAI) {
    		printf("%s: accepting a connection\n", CMD);
    		lgAdrClient = sizeof(adrClient);
    		canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
    		if (canal < 0)
    			erreur_IO("accept");
    		
    		printf("%s: adr %s, port %hu\n", CMD,
    		stringIP(ntohl(adrClient.sin_addr.s_addr)), ntohs(adrClient.sin_port));
    		
    		dataThread = ajouterDataThread();
    		dataThread->spec.canal = canal;
    		
    		client_t *cli = (client_t *)malloc(sizeof(client_t));
    		cli->address = adrClient;
    		cli->socket = canal;
    		cli->addr_len = lgAdrClient;
    		cli->score = 0;
    		ajouter_client(cli);
    		
    		arg_t *argmt = (arg_t *)malloc(sizeof(arg_t));
    		argmt->dataThr = dataThread;
    		argmt->client = cli;
    		
    		ret = pthread_create(&dataThread->spec.id, NULL, threadSessionClient, argmt);
    		if (ret != 0)
      			erreur_IO("creation thread");
      		
      	}
      	
      	if (close(ecoute) == -1)
    		erreur_IO("fermeture socket ecoute");  

  	if (close(fdJournal) == -1)
    		erreur_IO("fermeture journal");  

  	exit(EXIT_SUCCESS);
}


