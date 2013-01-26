#include <stdio.h>
#include "common.h"
#include "common.c"

// Variables globales pour le serveur
#define MAX_USERS 5		// Maximum d'utilisateurs connectés

int nb_users = 0;       // Utilisateurs actuellement connectés
int server_port = 0;	// Port d'écoute du serveur
client clients[]; 		// Tableau de tout les clients

void remove_handle_client(client client); // Suppression d'un client
void read_cmd(client client, char *commande); // Lecture d'une commande
void exec_cmd(client client, char* cmd, char* param); // Exécution d'une commande
