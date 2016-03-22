#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

void trimitereSpCp(char *sir , int numarByts){
	msg t;
	memcpy(t.payload,sir,numarByts);
	t.len = numarByts*sizeof(char) ;
	send_message(&t);
}

void trimitereSir(char * sir){
	msg t;
	sprintf(t.payload,"%s",sir);
	t.len = strlen(sir)*sizeof(char) + 1;
	send_message(&t);
}

void trimitereInt(int n){
	msg t;
	sprintf(t.payload,"%d",n);
	t.len = strlen(t.payload)*sizeof(char) + 1;
	send_message(&t);
}

void trimitereACK(){
	msg t;
	sprintf(t.payload,"%s","ACK");
	t.len = 4*sizeof(char);
	send_message(&t);
}


int getParity(unsigned char x){
	int paritate = 0 ;
	 while (x > 0) {
       paritate = (paritate + (x & 1)) % 2;
       x >>= 1;
    }
	return paritate;
}



void trimitereNACK(){
	msg t;
	sprintf(t.payload ,"%s","NACK");
	t.len = 5;
	send_message(&t);
}

int verificareParitateString (char* s){
	int pariti = 0 , i;
	for(i = 0 ; i < strlen(s) ; i++)
		pariti += getParity(s[i]);
	return pariti % 2;
}

int verificareParitateMemorie( char * s , int nr){
	int pariti = 0 , i;
	for(i = 0 ; i < nr ; i++)
		pariti += getParity(s[i]);
	return pariti % 2;
}

int main(int argc , char **argv)
{
	msg r ,t;
	int i, j ; 
	char *comanda = (char *) malloc (3*sizeof(char));
	char *argComanda = (char *) malloc (3*sizeof(char));
	
	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	if(argv[1] == NULL){
		for (i = 0; i < COUNT; i++) {
			/* wait for message */
			recv_message(&r);

			//retinem comanda si argumentul
			sscanf(r.payload,"%s %s",comanda , argComanda);

			if( strcmp(comanda,"cd") == 0 ){
				chdir(argComanda);
				trimitereACK();
			}

			if( strcmp(comanda,"ls") == 0 ){
				struct dirent **dp;
				int number = scandir(argComanda,&dp,NULL,alphasort);

				//trimit ACK
				trimitereACK();

				//Trimit numar de fisiere
				trimitereInt(number);
				//Verificare daca clientul a trimis ACK
				recv_message(&r);

				for( j = 0 ; j < number ; j++ ){
					//trimitem numelee de fisiere
					trimitereSir(dp[j]->d_name);

					recv_message(&r);
					// printf("%s\n",ack);
				}
			}
			if( strcmp( comanda,"exit") == 0 ){
				trimitereACK();
				break;
			}

			if ( strcmp(comanda,"cp") == 0 ){

				int size_fisier , deschidere;
				char *buffer = (char *)malloc(1400 * sizeof(char));
				//trimitem ACK
				trimitereACK();

				//deschidem fisierul
				deschidere = open(argComanda,O_RDONLY,0);
				if(deschidere < 0){
					perror("Nu se poate deschide fisierul\n");
					return -1;
				}

				//calculam dimensiunea fisierului
				size_fisier = lseek(deschidere,0,SEEK_END);
				
				//trimitem dimensiunea fisierului
				trimitereInt(size_fisier);

				int max = size_fisier % 1400;
				//verificam daca a primit clientul ACK
				recv_message(&r);

				//trecem la inceputul fisierului
				lseek(deschidere,0,SEEK_SET);

				for( j = 1 ; j <= size_fisier/1400 ; j++){
					//citim in buffer cate 1400 caractere
					read(deschidere,buffer,1400);
					
					//trimitem ce am citit in buffer
					trimitereSpCp(buffer,1400);

					//verificam daca a primit clientul ACK
					recv_message(&r);


				}

				//in caz ca ne mai ramane un rest de byti in fisier ii trimitem si pe ei
				if( max != 0 ){
					buffer = (char *) malloc (max * sizeof(char));

					//citim in buffer cate 1400 caractere
					read(deschidere,buffer,max);
					
					//trimitem ce am citit in buffer
					trimitereSpCp(buffer,max);

					//verificam daca a primit clientul ACK
					recv_message(&r);

				}

				close(deschidere);

			}

			if(strcmp(comanda,"sn") == 0){
				int size_fisier , scrie  , deschide , i , rest ;

				//creez numele noului fisier
				char *numeNou = (char *)malloc((4+strlen(argComanda))*sizeof(char));
				strcpy(numeNou,"new_");
				strcat(numeNou,argComanda);
				//trimite ACK

				deschide = open(numeNou,O_WRONLY | O_CREAT | O_TRUNC ,0777);

				//trimite ACK
				trimitereACK();

				//primeste dimensiunea
				recv_message(&r);
				size_fisier = atoi(r.payload);
				rest = size_fisier % 1400;

				//trimite ACK
				trimitereACK();
				for(i = 1 ; i <= size_fisier/1400 ; i++){
					// primim pachetelele 
					recv_message(&r);

					//punem pachetelul in fisier
					scrie = write(deschide,r.payload,1400);
					if(scrie < 0){
						perror("Eroare la scriere");
						return -1;
					}

					//Trimitem ACK
					trimitereACK();
				}

				//in caz ca mai avem un rest de byts
				if( rest != 0 ){
					recv_message(&r);
					scrie = write(deschide,r.payload,rest);
					if(scrie < 0){
						perror("Eroare la scriere");
						return -1;
					}

					//Trimitem ACK
					trimitereACK();

				}
				close(deschide);
			}

		}
	}

	if(strcmp(argv[1],"parity") == 0){
		for( i = 0 ; i < COUNT ; i++ ){

			recv_message(&r);
			while( verificareParitateString(r.payload + 1) != ( r.payload[0] & 1 ) ){
				trimitereNACK();
				recv_message(&r);
			}

			//retinem comanda si argumentul
			sscanf(r.payload + 1,"%s %s",comanda , argComanda);
			
			if(strcmp(comanda , "cd") == 0){
				chdir(argComanda);
				trimitereACK();
			}

			if(strcmp(comanda , "ls") == 0){
			 	struct dirent **dp;
				int number = scandir(argComanda,&dp,NULL,alphasort);

				//trimit ACK
				trimitereACK();

				//Trimit numar de fisiere
				sprintf((t.payload+1),"%d",number);
				t.payload[0] &= 0; // setam bitii 0
				t.payload[0] |= verificareParitateString(t.payload + 1); //setam bitul 1 ca fiin paritatea
				t.len = strlen(t.payload + 1) + 1; 
				send_message(&t);
				recv_message(&r);

				//Verificam daca s-a trimis bine , daca nu retrimitem pana primim ACK
				while(r.len != 4){
					send_message(&t);
					recv_message(&r);
				}

				for( j = 0 ; j < number ; j++ ){
					//trimitem numelee de fisiere
					t.payload[0] &= 0; // setam bitii 0
					t.payload[0] |= verificareParitateString(dp[j]->d_name);//setam bitul 1 ca fiin paritatea
					sprintf(t.payload+1,"%s",dp[j]->d_name);
					t.len = strlen(t.payload + 1) + 1;
					send_message(&t);
					recv_message(&r);

					//Verificam daca s-a trimis bine , daca nu retrimitem pana primim ACK
					while(r.len != 4){
					send_message(&t);
					recv_message(&r);
					}
				}
			}

			if(strcmp(comanda , "exit") == 0){
				trimitereACK();
				break;
			}

			if(strcmp(comanda , "cp") == 0){

				int size_fisier ;
				FILE * fisier ;

				//trimitem ACK
				trimitereACK();

				//deschidem fisierul
				fisier = fopen(argComanda,"r");

				//calculam dimensiunea fisierului
				fseek(fisier,0,SEEK_END);
				size_fisier = ftell(fisier);
				
				//trimitem dimensiunea fisierului
				sprintf(t.payload+1,"%d",size_fisier);

				t.payload[0] &= 0;//setam bitii 0
				t.payload[0] |= verificareParitateString(t.payload + 1);//setam bitul 1 ca fiin paritatea
				t.len = strlen(t.payload + 1) + 1;
				send_message(&t);
				recv_message(&r);

				//Verificam daca s-a trimis bine , daca nu retrimitem pana primim ACK
				while(r.len != 4){
					send_message(&t);
					recv_message(&r);
				}

				int max = size_fisier % 1399;

				//trecem la inceputul fisierului
				fseek(fisier,0,SEEK_SET);

				for( j = 1 ; j <= size_fisier/1399 ; j++){

					//citim in buffer cate 1399 caractere
					fread(t.payload + 1,1399,1,fisier);

					//setam paritatea
					t.payload[0] &= 0;
					t.payload[0] |= verificareParitateMemorie(t.payload + 1 , 1399);
					t.len =  1400 ;

					//trimitem
					send_message(&t);
					recv_message(&r);
					while(r.len != 4){
						send_message(&t);
						recv_message(&r);
					}

				}

				//in caz ca ne mai ramane un rest de byti in fisier ii trimitem si pe ei
				if( max != 0 ){

					//citim in buffer max caractere
					fread(t.payload + 1,max,1,fisier);

					//setam paritatea
					t.payload[0] &= 0;
					t.payload[0] |= verificareParitateMemorie(t.payload + 1 , max);
					t.len = max + 1;

					//trimitem
					send_message(&t);
					recv_message(&r);
					while(r.len != 4){
						send_message(&t);
						recv_message(&r);
					}

				}

				fclose(fisier);

			}

			if(strcmp(comanda , "sn") == 0){
				int size_fisier , scrie  , deschide , i , rest ;

				//creez numele noului fisier
				char *numeNou = (char *)malloc((4+strlen(argComanda))*sizeof(char));
				strcpy(numeNou,"new_");
				strcat(numeNou,argComanda);

				//deschid fisierul 
				deschide = open(numeNou,O_WRONLY | O_CREAT | O_TRUNC ,0777);

				//trimite ACK
				trimitereACK();

				//primeste dimensiunea
				recv_message(&r);
				
				//verificam daca dimensiunea s-a trimis bine				
				while( verificareParitateString(r.payload + 1) != ( r.payload[0] & 1 ) ){
					trimitereNACK();
					recv_message(&r);
				}

				// setam dimensiunea
				size_fisier = atoi(r.payload + 1);
				rest = size_fisier % 1399;

				//trimite ACK
				trimitereACK();
				for(i = 1 ; i <= size_fisier/1399 ; i++){
					// primim pachetelele 
					recv_message(&r);

					//verificam daca s-au primit datele corect 
					while( verificareParitateMemorie(r.payload + 1 , 1399) != ( r.payload[0] & 1 ) ){
						trimitereNACK();
						recv_message(&r);
					}

					//punem pachetelul in fisier
					scrie = write(deschide,r.payload + 1,1399);
					if(scrie < 0){
						perror("Eroare la scriere");
						return -1;
					}

					//Trimitem ACK
					trimitereACK();
				}

				//in caz ca mai avem un rest de byts
				if( rest != 0 ){
					recv_message(&r);

					//verificam daca s-au primit datele corect 
					while( verificareParitateMemorie(r.payload + 1 , rest) != ( r.payload[0] & 1 ) ){
						trimitereNACK();
						recv_message(&r);
					}

					scrie = write(deschide,r.payload + 1,rest);
					if(scrie < 0){
						perror("Eroare la scriere");
						return -1;
					}

					//Trimitem ACK
					trimitereACK();

				}
				close(deschide);
			}

		}
	}
		printf("[RECEIVER] Finished receiving..\n");
		return 0;
}
