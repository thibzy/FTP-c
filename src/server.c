#include "server.h"

client clients[MAX_USERS];
char buffer[BUFFER_LENGTH];
int number_file_descriptor_set = 0;

// Vider le tampon et demander une commande
void clear_and_prompt()
{
	printf("%s","server>");
	fflush(stdout);
}

// Gestion des nouveaux clients
void handle_clients(int socket_server, struct sockaddr_in cli_addr)
{
	int socket_newclient;
	socklen_t clilen = sizeof(cli_addr); // Taille de l'adresse du client

	socket_newclient = accept(socket_server, (struct sockaddr *) &cli_addr, &clilen); // On attend les connexions
	if (socket_newclient < 0)
		display_error("Accept fail");

	if(nb_users == MAX_USERS)
	{
		socket_send_with_code(socket_newclient, "Service not available. Too many users.", 421);
	}
	else
	{
		// Nouveau client
		clients[nb_users]->addrip.s_addr = cli_addr.sin_addr.s_addr; // IP client
		clients[nb_users]->sock = socket_newclient; // Socket client
		clients[nb_users]->dataport = 0; // Port du client
		clients[nb_users]->pid = 0; // PID sous processus gérant le client
		strcpy(clients[nb_users]->curdir, "/home/thibz/ftp"); // Répertoire par défaut du client
		nb_users++;

		socket_send_with_code(socket_newclient, "Welcome on ESGI FTP server.\n Current dir is /home/thibz/ftp", 220);
	}
}

// Suppression d'un client
void remove_handle_client(client client)
{
	close(client->sock);
	client->pid = 0;
	client->sock = 0;
	client->dataport = 0;
	nb_users--;
}

// Lecture d'une commande
void read_cmd(client client, char* commande)
{
	char* param;
	int sizelen = strlen(commande);

	// Suppression des sauts de lignes
	if(commande[sizelen-1] == '\n')
	{
		commande[sizelen-1] = 0;
	}

	param = strchr(commande,' ');
	if (param)
	{
		*param = 0;
		param++;
  	}

	exec_cmd(client, commande, param);
}

// Execute une commande
// Une seule utilisation de socket_send possible (le client attend une réponse après la commande)
void exec_cmd(client client, char* cmd, char* param)
{
	int i = 0, cmd_is_valid = 0;

	printf("Command = %s[FIN]\n", cmd);
	printf("Param   = %s[FIN]\n", param);

	// Vérification commande valide
	for (i = 0; i < nb_commandes; i++)
	{
		if (strcmp(cmd,commandes[i]) == 0)
		{
			cmd_is_valid = 1;
		}
	}

	if(!cmd_is_valid)
	{
		socket_send(client->sock, "Invalid command.");
		return;
	}

	// Traitement de la commande
	if(strcmp(cmd, "USER") == 0)
	{
		// TODO : Gérer un fichier de configuration avec des utilisateurs
		socket_send_with_code(client->sock, "Ok", 230);
	}
	else if(strcmp(cmd, "PASS") == 0)
	{
		// TODO : Gérer un fichier de configuration avec des utilisateurs
		socket_send_with_code(client->sock, "Ok", 230);
	}
	else if(strcmp(cmd, "LIST") == 0)
	{
		struct dirent *dirp;
		DIR *dirfd;
		dirfd = opendir(client->curdir);

		if(!dirfd)
		{
			socket_send(client->sock, "Problem while opening directory");
		}
		else
		{
			while( (dirp = readdir(dirfd)) != NULL) {
				// On affiche les dossiers/fichier à la suite avec un espace
				char d_name[strlen(dirp->d_name)+1];
				strcpy(d_name, dirp->d_name);
				strcat(d_name, " ");
				socket_send(client->sock, d_name);
			}
		}
	}
	else if(strcmp(cmd, "PORT") == 0)
	{
		client->dataport = atoi(param);
		socket_send_with_code(client->sock, "Ok", 230);
	}
	else if(strcmp(cmd, "QUIT") == 0)
	{
		socket_send_with_code(client->sock, "Goodbye", 230);
		remove_handle_client(client);
	}
	else if(strcmp(cmd, "CWD") == 0)
	{
		strcpy(client->curdir, param);

		char response[BUFFER_LENGTH];
		strcpy(response, "Current dir is now ");
		strcat(response, client->curdir);

		socket_send_with_code(client->sock, response, 230);
	}
	else if(strcmp(cmd, "DELE") == 0 && param)
	{
		char filename[BUFFER_LENGTH];
		strcpy(filename, client->curdir);
		strcat(filename,"/");
		strcat(filename, param);

		if(unlink(filename) == 0)
			socket_send_with_code(client->sock, "File deleted", 213);
		else
			socket_send_with_code(client->sock, strerror(errno), 213);
	}
	else if(strcmp(cmd, "MKD") == 0)
	{
		char directoryname[BUFFER_LENGTH];
		strcpy(directoryname, client->curdir);
		strcat(directoryname,"/");
		strcat(directoryname, param);

		if(mkdir(directoryname,0777) == 0)
			socket_send_with_code(client->sock, "Directory created", 212);
		else
			socket_send_with_code(client->sock, strerror(errno), 212);
	}
	else
	{
		socket_send_with_code(client->sock, "Wrong command or not implemented", 202);
	}
}

// Main
int main(int argc, char *argv[])
{
	// Initialisation des variables
	int socket_server, portno;
	struct sockaddr_in serv_addr, cli_addr;
	int i;
	fd_set read_sd, rsd; // Descripteur pour la lecture d'un socket

	// Arguments
	if (argc < 2) {
		display_error("You should enter a port number");
		exit(1);
	}
	portno = atoi(argv[1]);

	// Initilisation serveur
	socket_server = socket(AF_INET, SOCK_STREAM, 0); // IPV4, intégrité+flux binaire
	if (socket_server < 0)
		display_error("Problem while opening socket");
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET; // IPV4
	serv_addr.sin_addr.s_addr = INADDR_ANY; // Accepter tout types d'adresses
	serv_addr.sin_port = htons(portno); // Conversion et assignation du port
	// On bind le socket et l'adresse
	if (bind(socket_server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		display_error("Problem while binding adress to the server");
		exit(1);
	}

	printf("Listening on port %i...\n", portno);
	listen(socket_server, MAX_USERS); // Autoriser 5 connexions simultannées

	// On alloue la mémoire pour les clients
	for (i = 0; i < MAX_USERS; i++) {
		clients[i]=(client_t *)malloc(sizeof(client_t));
		clients[i]->sock = 0;
	}

	// Descripteurs pour l'écoute
	FD_ZERO(&read_sd);
	FD_SET(1, &read_sd); // Entrée clavier
	FD_SET(socket_server, &read_sd); // Socket du serveur

	// Boucle principale
	int stop = 0;
	while(!stop) {
		rsd = read_sd;

		// On ajoute tout les descripteurs pour les clients
		for (i = 0; i < nb_users; i++)
			if (clients[i]->sock)
				FD_SET(clients[i]->sock, &rsd);

		// Select bloque tant qu'un descripteur n'est pas actif
		// TODO : Modifier la valeur qui est arbitraire (maximum du nb de descripteurs)
		select(12, &rsd, (fd_set *)0, (fd_set *)0, (struct timeval *)0);

		// Gestion des commandes serveur
		if (FD_ISSET(1, &rsd))
		{
			read(1, buffer, BUFFER_LENGTH);
			printf("Your command is : %s", buffer);
			printf("Closing server...\n");
			break;
		}

		// Gestion des nouveau clients
		if (FD_ISSET(socket_server, &rsd))
		{
			handle_clients(socket_server, cli_addr);
		}

		// Gestion des utilisateurs connectés
		for(i=0; i<nb_users; i++) // On boucle sur les utilisateurs connectés
		{
			if (clients[i]->sock && FD_ISSET(clients[i]->sock, &rsd))
			{
				char bufferclient[BUFFER_LENGTH];
				memset(bufferclient, '\0', BUFFER_LENGTH);

				int data_read = read(clients[i]->sock, bufferclient, BUFFER_LENGTH);

				if(data_read > 0)
				{
					// Lecture de la commande
					read_cmd(clients[i], bufferclient);
				}
				else
				{
					// Déconnexion du client
					printf("Client %d disconnected\n", i);
					remove_handle_client(clients[i]);
				}
			}
		}
	}

	// Fermeture de toutes les connexions clientes
	for(i=0; i<nb_users; i++) {
		remove_handle_client(clients[i]);
	}

	close(socket_server); // Fermeture du socket
	return 0;
}
