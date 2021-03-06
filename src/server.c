#include "server.h"

// Variables globales
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
		// Répertoire par défaut
		struct passwd *pw = getpwuid(getuid());
		char *homedir = pw->pw_dir;

		// Nouveau client
		clients[nb_users]->addrip.s_addr = cli_addr.sin_addr.s_addr; // IP client
		clients[nb_users]->sock = socket_newclient; // Socket client
		clients[nb_users]->dataport = 2000; // Port du client (2000 par défaut)
		clients[nb_users]->pid = 0; // PID sous processus gérant le client
		clients[nb_users]->abort = 0; // PID sous processus gérant le client
		strcpy(clients[nb_users]->curdir, homedir); // Répertoire par défaut du client
		strcpy(clients[nb_users]->previousparam, "");
		nb_users++;

		char response[BUFFER_LENGTH];
		memset(response, '\0', BUFFER_LENGTH);
		strcpy(response, "Welcome on ESGI FTP server.\nCurrent dir is ");
		strcat(response, homedir);

		socket_send_with_code(socket_newclient, response, 220);
	}
}

// Suppression d'un client
void remove_handle_client(client client)
{
	printf("Client disconnected\n");
	close(client->sock);
	client->sock = 0;
	client->dataport = 0;

	if (client->pid)
		kill(client->pid, SIGTERM);

	nb_users--;
}

// Ouverture d'un socket data entre le client et le serveur
int open_data_socket(client client, int asclient)
{
	struct sockaddr_in to;
	int sd, tolen;

	to.sin_family = AF_INET;
	to.sin_port = htons(client->dataport);
	to.sin_addr.s_addr = asclient == 1 ? client->addrip.s_addr : INADDR_ANY;

	tolen = sizeof(to);

	if( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Erreur socket data\n");
		return -1;
	}

	if(asclient == 1)
	{
		if(connect(sd, (struct sockaddr*)&to, tolen) < 0)
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
	printf("Current dir = %s[FIN]\n", client->curdir);
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

	// Login user
	if(strcmp(cmd, "USER") == 0)
	{
		// TODO : Gérer un fichier de configuration avec des utilisateurs
		socket_send_with_code(client->sock, "Ok", 230);
	}
	// Password user
	else if(strcmp(cmd, "PASS") == 0)
	{
		// TODO : Gérer un fichier de configuration avec des utilisateurs
		socket_send_with_code(client->sock, "Ok", 230);
	}
	// Liste des fichiers/dossiers du répertoire courant
	else if(strcmp(cmd, "LIST") == 0)
	{
		if( (client->pid = fork()) == 0 )
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
	}
	// Retourne le répertoire courant
	else if(strcmp(cmd, "PWD") == 0)
	{
		socket_send_with_code(client->sock, client->curdir, 230);
	}
	// Changer le numéro de port pour les transferts
	else if(strcmp(cmd, "PORT") == 0)
	{
		client->dataport = atoi(param);
		socket_send_with_code(client->sock, "Ok", 230);
	}
	// Fermer la connexion
	else if(strcmp(cmd, "QUIT") == 0)
	{
		socket_send_with_code(client->sock, "Goodbye", 230);
		remove_handle_client(client);
	}
	// Changer le répertoire courant
	else if(strcmp(cmd, "CWD") == 0  && param)
	{
		strcpy(client->curdir, param);

		char response[BUFFER_LENGTH];
		strcpy(response, "Current dir is now ");
		strcat(response, client->curdir);

		socket_send_with_code(client->sock, response, 230);
	}
	// Suppression d'un fichier
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
	// Suppression d'un répertoire
	else if(strcmp(cmd, "RMD") == 0 && param)
	{
		char filename[BUFFER_LENGTH];
		strcpy(filename, client->curdir);
		strcat(filename,"/");
		strcat(filename, param);

		if(rmdir(filename) == 0)
			socket_send_with_code(client->sock, "Directory deleted", 213);
		else
			socket_send_with_code(client->sock, strerror(errno), 213);
	}
	// Création d'un répertoire
	else if(strcmp(cmd, "MKD") == 0 && param)
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
	// Renommer un fichier (RNFR + RNTO)
	else if(strcmp(cmd, "RNFR") == 0 && param)
	{
		strcpy(client->previousparam, param);
		socket_send_with_code(client->sock, "Ok, need RNTO", 212);
	}
	else if(strcmp(cmd, "RNTO") == 0 && param)
	{
		char newfilename[BUFFER_LENGTH];
		char oldfilename[BUFFER_LENGTH];

		strcpy(oldfilename, client->curdir);
		strcat(oldfilename, "/");
		strcat(oldfilename, client->previousparam);

		strcpy(newfilename, client->curdir);
		strcat(newfilename, "/");
		strcat(newfilename, param);

		if(rename(oldfilename, newfilename))
			socket_send_with_code(client->sock, "File renamed", 212);
		else
			socket_send_with_code(client->sock, strerror(errno), 212);
	}
	// Arrêt de la commande précédente et des tranferts
	else if(strcmp(cmd, "ABOR") == 0)
	{
		// TODO : Interrompre le transfert
		client->abort = 1;
		socket_send_with_code(client->sock, "Abort failed", 450);
	}
	// Download d'un fichier par le client
	else if(strcmp(cmd, "RETR") == 0 && param)
	{
		if( (client->pid = fork()) == 0 )
		{
			int file, size_read;
			char buffer[BUFFER_LENGTH];

			// Nom du fichier
			char filename[BUFFER_LENGTH];
			strcpy(filename, client->curdir);
			strcat(filename, "/");
			strcat(filename, param);

			// Ouverture d'une nouvelle connexion sur le dataport du client
			int socket_data = open_data_socket(client, 1);  // serveur devient client

			if(socket_data > 0)
			{
				// fopen du fichier demandé en paramêtre
				file = open(filename, O_RDONLY);
				if(file >= 0)
				{
					int size_sent = 0;
					while( (size_read = read(file, buffer, BUFFER_LENGTH)) > 0 )
					{
						// Envoi des données
						size_sent += write(socket_data, buffer, size_read);
					}
					printf("Sent %s (%d bytes)\n", filename, size_sent);
					socket_send_with_code(client->sock, "File sent", 226);
				}
				else
				{
					socket_send_with_code(client->sock, strerror(errno), 212);
				}

				close(socket_data);
				socket_data = 0;
			}
			else
			{
				socket_send_with_code(client->sock, "Error connection server>client", 212);
			}
		}
	}
	// Upload d'un fichier par le client
	else if(strcmp(cmd, "STOR") == 0 && param)
	{
		if( (client->pid = fork()) == 0 )
		{
			int file, size_read, client_datasocket = 0;
			char bufferfile[BUFFER_LENGTH];
			struct sockaddr_in from;
			socklen_t fromlen = sizeof(from);

			// Nom du fichier
			char filename[BUFFER_LENGTH];
			strcpy(filename, client->curdir);
			strcat(filename, "/");
			strcat(filename, param);

			// Ouverture d'une nouvelle connexion sur le dataport du client
			int socket_data = open_data_socket(client, 0);
			if(socket_data > 0) {
				socket_send_with_code(client->sock, "Ready for data connection", 212);
				client_datasocket = accept(socket_data, (struct sockaddr *) &from, &fromlen);
			} else {
				socket_send_with_code(client->sock, strerror(errno), 212);
			}

			if(client_datasocket > 0)
			{
				// Enregistrement du fichier
				if(0 > (file = open(filename, O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)))
				{
					socket_send_with_code(client->sock, strerror(errno), 212);
				}

				int size_received = 0, writesize = 1;
				while( writesize != 0 )
				{
					size_read = read(client_datasocket, bufferfile, sizeof(bufferfile));
					writesize = write(file, bufferfile, size_read);
					size_received += size_read;

					if( writesize == 0 )
					{
						char bufferresponse[BUFFER_LENGTH];
						sprintf(bufferresponse, "Received file \"%s\" (%d bytes)", filename, size_received);
						socket_send_with_code(client->sock, bufferresponse, 226);
					}
				}

				close(client_datasocket);
				close(socket_data);
				socket_data = client_datasocket = 0;
			}
			else
			{
				socket_send_with_code(client->sock, "Error connection client>server", 212);
			}
		}
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
