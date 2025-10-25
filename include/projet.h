#define TAILLEMAX 20
#define NB_QUESTIONS  3

typedef struct {
    	int socket;
    	struct sockaddr_in address;
    	int addr_len;
    	char nom[TAILLEMAX];
    	int score;
} client_t;

//structure permettant d'executer la fonction 'sessionClient' du serveur avec deux arguments
typedef struct {
	client_t *client;
	DataThread *dataThr;
} arg_t;

//tableau contenant les questions
const char *questions[NB_QUESTIONS] = {
    "Quelle est la capitale du Royaume-Uni ?",
    "Quel est l'auteur du livre 'Les Miserables' ?",
    "En quelle annee la revolution francaise a-t-elle débuté ?"
};

//tableau contenant les reponses aux question
const char *reponses[NB_QUESTIONS] = {
    "Londres",
    "Victor Hugo",
    "1789"
};
