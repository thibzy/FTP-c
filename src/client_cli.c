#include "client.h"

// Variables globales
int stop = 0; // Variable pour la boucle principale
struct sockaddr_in serv_addr; // Adresse du serveur
int sockfd; // Socket du serveur

// Vider le tampon et demander une commande
void clear_and_prompt()
{
	printf("%s","client>");
	fflush(stdout);
}

// Ouverture d'une connexion data
int open_data_socket(struct sockaddr_in serv_addr, int dataport, int asclient) {
	struct sockaddr_in to;
	int sd, tolen;

	to.sin_family = AF_INET;
	to.sin_port = htons(dataport);
	to.sin_addr.s_addr = asclient == 1 ? serv_addr.sin_addr.s_addr : INADDR_ANY;

	tolen = sizeof(to);

	if( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Erreur socket data\n");
		return -1;
	}

	if(asclient == 1)
	{
		if(0 > connect(sd, (struct sockaddr*)&to, tolen))
		{
			printf("Erreur connect data socket\n");
			return -1;
		}
	}
	else
	{
		if (bind(sd, (struct sockaddr *) &to, tolen) < 0)
		{
			printf("Erreur bind data socket\n");
			return -1;
		}

		// Ecoute du serveur
		if(listen(sd, 1) < 0)
		{
			printf("Erreur listen data socket\n");
			return -1;
		}
	}

	return sd;
}

void cmd_retr(char* filename) {
	// On écoute une connexion nouvelle connexion sur le port 2000 (défaut) pour la réception du fichier
    int server_datasocket = 0;
    struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);

	int datasocket = open_data_socket(serv_addr, data_port, 0); // client devient serveur
	if(datasocket > 0) {
		server_datasocket = accept(datasocket, (struct sockaddr *) &from, &fromlen);
	} else {
		printf("%s\n", strerror(errno));
	}

	int fd, recv = 1, writesize = 1;
	char bufferfile[BUFFER_LENGTH];

	// Enregistrement du fichier
	if(0 > (fd = open(filename, O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)))
	{
		printf("%s\n", strerror(errno));
	}

	int size_received = 0;
	while( writesize != 0 )
	{
		recv = read(server_datasocket, bufferfile, sizeof(bufferfile));
		writesize = write(fd, bufferfile, recv);
		size_received += recv;
	}

	close(datasocket);
	close(server_datasocket);
	server_datasocket = datasocket = 0;
}

void cmd_stor(char* filename) {
    int file, size_read;
    char bufferfile[BUFFER_LENGTH];
    char bufferresponse[BUFFER_LENGTH];
    memset(bufferresponse, '\0', BUFFER_LENGTH);

    // Attente d'une réponse du serveur pour se connecter en socket data
	read(sockfd,bufferresponse,BUFFER_LENGTH); // Lecture du message
    printf("%s\n",bufferresponse);

	int datasocket = open_data_socket(serv_addr, data_port, 1);

	// Enregistrement du fichier
	if(datasocket > 0)
	{
		// fopen du fichier demandé en paramêtre
		file = open(filename, O_RDONLY);
		if(file >= 0)
		{
			int size_sent = 0;
			while( (size_read = read(file, bufferfile, BUFFER_LENGTH)) > 0 )
			{
				// Envoi des données
				size_sent += write(datasocket, bufferfile, size_read);
			}
		}
		else
		{
			printf("%s\n", strerror(errno));
		}

		close(datasocket);
		datasocket = 0;
	}
	else
	{
		printf("%s\n", strerror(errno));
	}
}

// Execute une commande /!\ après l'envoi de cette commande au serveur
void exec_cmd(char* cmd, char* param)
{
	// Commandes
	if(strcmp(cmd, "QUIT") == 0)
	{
		stop = 1;
	}
	else if(strcmp(cmd, "STOR") == 0 && param)
	{
		cmd_stor(param);
	}
	else if(strcmp(cmd, "RETR") == 0 && param)
	{
		cmd_retr(param);
	}
	else if(strcmp(cmd, "PORT") == 0 && param)
	{
		data_port = atoi(param);
	}
}

// Lecture d'une commande
void read_cmd(char* commande)
{
	char* param;
	int sizelen = strlen(commande);

    // On copie la commande (altération dans read_cmd pour séparation des params)
    char commandecpy[BUFFER_LENGTH];
    strcpy(commandecpy, commande);

	// Suppression des sauts de lignes
	if(commandecpy[sizelen-1] == '\n')
	{
		commandecpy[sizelen-1] = 0;
	}

	param = strchr(commandecpy,' ');
	if (param)
	{
		*param = 0;
		param++;
  	}

	exec_cmd(commandecpy, param);
}

// Main
int main(int argc, char *argv[])
{
	// Initialisation des variables
    int portno;
    struct hostent *server;
    char buffer[BUFFER_LENGTH];
    memset(buffer, '\0', BUFFER_LENGTH);

    // Arguments
    if (argc < 3)
    {
       fprintf(stderr,"Usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);

    // Initialisation
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPV4, intégrité+flux binaire
    if (sockfd < 0)
        display_error("ERREUR ouverture socket");

    // IP ou hostname en paramêtre ?
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    server = gethostbyname(argv[1]); // On récupère l'host par son nom
    if (server != NULL)
    {
        bcopy((char *)server->h_addr,
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
    }
    else
    {
    	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    	if (serv_addr.sin_addr.s_addr == INADDR_NONE)
    	{
            fprintf(stderr,"Socket Error # 11001, Host not found:\n");
			exit(0);
        }
    }

    serv_addr.sin_family = AF_INET; // IPV4
    serv_addr.sin_port = htons(portno); // Conversion et assignation du port
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) // Connexion au serveur
    {
    	display_error("Socket Error # 10061, Connection refused");
    	exit(1);
    }

    memset(buffer, 0, BUFFER_LENGTH);
	read(sockfd,buffer,BUFFER_LENGTH); // Lecture du message de bienvenu
    printf("%s\n",buffer);

	// Boucle principale
	while(!stop) {
	    clear_and_prompt();
	    memset(buffer, 0, BUFFER_LENGTH);
	    fgets(buffer,BUFFER_LENGTH,stdin);

	    // Envoi au serveur
	    write(sockfd,buffer,strlen(buffer));

	    // Lecture de la commande (si traitement côté client nécessaire)
	    read_cmd(buffer);

	    // Réponse du serveur
	    memset(buffer, 0, BUFFER_LENGTH);
		read(sockfd,buffer,BUFFER_LENGTH);
	    printf("%s\n",buffer);
	}

    close(sockfd); // Fermeture du socket

    return 0;
}
