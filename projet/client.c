// client.c
#include "pse.h"

#define CMD   "client"

//fonction qui prend en charge les donnees envoyees par le serveur
void *gestion_reception(void *sock) {
	int socket_client = *(int *)sock;
    	char ligne[LIGNE_MAX];
    	while (1) {
    		int lgLue = recv(socket_client, ligne, LIGNE_MAX, 0);
    		if (lgLue > 0) {
    			printf("%s", ligne);
    		}
    		else if (lgLue == 0) {
    			break;
    		}
    		else {
    			perror("recv failed");
    			break;
    		}
    		bzero(ligne, LIGNE_MAX);
    	}
    	pthread_exit(NULL);
    	return NULL;
}


int main(int argc, char *argv[]) {
	int sock, ret;
  	struct sockaddr_in *adrServ;
  	char ligne[LIGNE_MAX];
  	pthread_t idThread;
  	
  	signal(SIGPIPE, SIG_IGN);
  	
  	if (argc != 3)
    		erreur("usage: %s machine port\n", argv[0]);
    	
    	printf("%s: creating a socket\n", CMD);
  	sock = socket (AF_INET, SOCK_STREAM, 0);
  	if (sock < 0)
  		erreur_IO("socket");
  	
  	printf("%s: DNS resolving for %s, port %s\n", CMD, argv[1], argv[2]);
  	adrServ = resolv(argv[1], argv[2]);
  	if (adrServ == NULL)
  		erreur("adresse %s port %s inconnus\n", argv[1], argv[2]);
  	
  	printf("%s: adr %s, port %hu\n", CMD, stringIP(ntohl(adrServ->sin_addr.s_addr)),
	       ntohs(adrServ->sin_port));
	
	printf("%s: connecting the socket\n", CMD);
  	ret = connect(sock, (struct sockaddr *)adrServ, sizeof(struct sockaddr_in));
  	if (ret < 0)
  		erreur_IO("connect");
  	
  	printf("Veuillez entrer votre nom : ");
  	fgets(ligne, LIGNE_MAX, stdin);
  	ecrireLigne(sock, ligne);
  	
  	if (pthread_create(&idThread, NULL, gestion_reception, (void *)&sock) != 0)
  		erreur("pthread_create");
  	
  	
  	while (1) {
  		fgets(ligne, LIGNE_MAX, stdin);
  		ecrireLigne(sock, ligne);
  		if (strcmp(ligne, "fin\n") == 0) {
  			break;
  		}
  		bzero(ligne, LIGNE_MAX);
  	}
  	if (close(sock) == -1)
  		erreur_IO("fermeture socket");
  	exit(EXIT_SUCCESS);
}
  		
  		
