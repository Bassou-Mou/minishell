// Bibliothèques standards
#include <stdio.h>      // printf, perror.
#include <stdlib.h>     // malloc, free, exit.
#include <stdbool.h>    // bool, true, false
#include <string.h>     // strcmp, strcpy.
// Gestion des processus et des fichiers
#include <unistd.h>     // fork, exec, pipe, dup2.
#include <fcntl.h>      // open, O_RDONLY, O_CREAT.
#include <sys/types.h>  // types systèmes (pid_t.)
#include <sys/wait.h>   // wait, waitpid, WIFEXITED.
#include <signal.h>     // signaux (sigaction, SIGINT.)
// Bibliothèque spécifique du projet
#include "readcmd.h"

#define Max_Taille 10 // Taille maximale du tableau de tubes.

bool Processus_Avant_Plan = false; //  Variable globale : Indiquer si le processus en avant plan est terminé.
pid_t PID; // PID du dernier processus lancé.
//Gestion du signal SIGCHLD : Cette fonction est appelée quand un processus fils change d’état 
void traitement_SIGCHLD () {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED))>0){
        // Cas : processus terminé normalement
       if (WIFEXITED(status)) {
          printf("Le processus terminé (%d).\n", pid);
       }  else if (WIFSTOPPED(status)) {  // Cas : processus suspendu
          printf("Le processus suspendu (%d).\n", pid);
       } else if (WIFCONTINUED(status)) { //Cas : processus relancé
          printf("Le processus repris (%d).\n", pid);
       }
       if (PID == pid){
         Processus_Avant_Plan = true; 
       }
    }
    return;  
}

int main(void) {
    // Configuration des signaux (SIGCHLD):
    struct sigaction action;
    action.sa_handler = traitement_SIGCHLD; 
    sigemptyset(&action.sa_mask); 
    action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &action, NULL);
    // Masquage temporaire des signaux SIGINT et SIGTSTP:
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);  // Ajout de Ctrl+C
    sigaddset(&mask,SIGTSTP);  // Ajout de Ctrl+Z
    sigprocmask(SIG_BLOCK, &mask, NULL); // Blocage de ces signaux
    
    bool fini= false;

    while (!fini) {
        printf("> "); // Affiche un prompt ">"
        struct cmdline *commande= readcmd();// lire la commande utilisateur via readcmd() 
        // Gestion des erreurs de lecture
        if (commande == NULL) {
            // commande == NULL -> erreur readcmd()
            perror("erreur lecture commande \n");
            exit(EXIT_FAILURE);
    
        } else {

            if (commande->err) {
                // commande->err != NULL -> commande->seq == NULL
                printf("erreur saisie de la commande : %s\n", commande->err);
        
            } else {
                // Comptage du nombre de commandes et création des tubes:
                int tubes[Max_Taille][2];
                int nbrCommandes = 0;
                while (commande->seq[nbrCommandes]){nbrCommandes++;}
                int nbrTubes = nbrCommandes - 1; // Nombre de tubes dans le tableau
                
                // Création des pipes 
                for (int i = 0; i<nbrTubes ; i++) {
                    if (pipe(tubes[i]) == -1) {
                            printf("Error: failed to create pipe\n");
                            exit(EXIT_FAILURE);
                    }
                } 

                /* Pour le moment le programme ne fait qu'afficher les commandes 
                   tapees et les affiche à l'écran. 
                   Cette partie est à modifier pour considérer l'exécution de ces
                   commandes 
                */
                int indexseq= 0;
                char **cmd;
                // Parcours des commandes et fork():
                while ((cmd= commande->seq[indexseq])) {
                
                    if (cmd[0]) {
                        if (strcmp(cmd[0], "exit") == 0) {
                            fini= true;
                            printf("Au revoir ...\n");
                        }
                        else {
                            // Affichage de la commande
                            printf("commande : ");
                            int indexcmd= 0;
                            while (cmd[indexcmd]) {
                                printf("%s ", cmd[indexcmd]);
                                indexcmd++;
                            }
                            printf("\n");
                            PID = fork();
                            if (PID == -1) {
                                printf("Erreur \n");
                                exit(EXIT_FAILURE);
                            // Processus fils
                            } else if (PID == 0) {
                                if (nbrCommandes > 1) {
                                	// Premier commande du pipeline.
                                	if (indexseq ==0){
				                        dup2(tubes[indexseq][1],1);
				                        close(tubes[indexseq][0]);
				            	// Dernier commande du pipeline.
				                    } else if (indexseq == nbrCommandes-1){
				                        dup2(tubes[indexseq-1][0],0);
				                        close(tubes[indexseq-1][1]);
				                    // Commande du milieu.                    
				                    } else {
				                        dup2(tubes[indexseq][1],1);
				                        dup2(tubes[indexseq-1][0],0);
				                        close(tubes[indexseq][0]); 
				                        close(tubes[indexseq-1][1]);
				                    }
				        		// Fermeture des tubes non utilisés
				                    for (int i=0; i<=nbrTubes-2; i++) {
				                        close(tubes[i][0]); //Pour la lecture
				                        close(tubes[i][1]); //Pour l'écriture
				                    }
                                }
                                //Déblocage des signaux dans le fils
		                        sigprocmask(SIG_UNBLOCK, &mask, NULL);
                                //Gestion des processus en arrière-plan                
                                if (commande -> backgrounded != NULL) { 
                                    setpgrp();                                  
                                }
                                
                                if (commande->in != NULL) {
                                    //Ouverture du fichier en lecture uniquement.
                                    int desc_in = open(commande->in, O_RDONLY);
                                    if (desc_in == -1) {
                                        perror("Error: problem opening file for reading");
                                        exit(EXIT_FAILURE);
                                    }
                                    dup2(desc_in, STDIN_FILENO);
                                    //Fermeture du descripteur ouvert
                                    close(desc_in);
                                }
                                // Redirection de sortie
                                if (commande->out != NULL) {
                                    int desc_out = open(commande->out, O_WRONLY|O_TRUNC|O_CREAT, 0640);
                                    if (desc_out == -1) {
                                        perror("Error: problem opening file for writing");
                                        exit(EXIT_FAILURE);
                                    }
                                    dup2(desc_out, STDOUT_FILENO);
                                    //Fermeture du descripteur ouvert
                                    close(desc_out);
                                }
                                //Exécution de la commande        
                                if (execvp(cmd[0], cmd) == -1) {
                                    printf("Erreur \n");
                                    exit(EXIT_FAILURE);
                                } 
                                
                            } else {
                                // Processus père
                                if ((indexseq == nbrCommandes - 1) && (commande -> backgrounded == NULL)) {
                                	for (int i = 0; i<nbrTubes ; i++){
		                    		    close(tubes[i][0]);
		                    		    close(tubes[i][1]);
		                     	    }
                                    // Attente passive du processus père jusqu’à ce que le processus de avant plan se termine.
                                    // Cette attente est levée lorsque le signal SIGCHLD est reçu
                                    // et que la variable globale Processus_Avant_Plan est mise à true dans le handler.                   	
                                	while (!Processus_Avant_Plan) {
                                		pause(); // Met le processus en veille jusqu’à réception d’un signal
                                	}
                                }  
                            
                            }
                            
                        }
  
                        indexseq++;
                    }
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
