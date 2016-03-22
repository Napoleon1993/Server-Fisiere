In fisierul server.c am implementat protocolul de comunicatie STOP AND WAIT , si STOP AND WAIT cu paritate.Timpul alocat pentru aceasta tema a fost cam de 20 de ore.

Implementarea modului stop and wait este exact ca si in tema :  clientul ----> comanda
		server ------> ACK
		server ------> ceea ce doreste clientul
		server <------ asteapta ACK de la client

Pentru a implementa modul stop and wait cu paritate m-am folosit de 3 functii ajutatoare si anume:
	- getParity(char x) care returna paritatea unui caracter
	- getParitateStrig(char *x) care returna paritatea unui sir de caractere 
	- getParitateMemorie(char *x , numarOct) care returna paritatea pentru N octeti cititi dintr-un fisier intr-un vector de caracter x

