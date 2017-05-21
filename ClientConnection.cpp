//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2ª de grado de Ingeniería Informática
//                       
//              This class processes an FTP transactions.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"




ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }
    
    ok = true;
    data_socket = -1;
   
  
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP( uint32_t address,  uint16_t  port) {
	struct sockaddr_in sin;
	int s;
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);	// es necesario?
	
	memcpy(&sin.sin_addr.s_addr, &address, sizeof(address));
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
		errexit("No se puede crear el socket: %s\n", strerror(errno));

	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errexit("No se puede conectar con %lu: %s\n", address, strerror(errno));

   return s;
}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {


 
      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
	  /*
	  230
	  530
	  500, 501, 421
	  331, 332
	  */
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
		fprintf(fd, "%s\n", arg);
	    if(!strcmp("pepe", arg) == 0){
			fprintf(fd, "530 Not logged in\n");
			return;
		//Salir;
	    }
	    else if(arg[0] == '\0'){
			fprintf(fd, ">>%s<<\n", arg);
			fprintf(fd, "332 Need account for login\n");
			return;
		//Salir;
	    }
      }
      else if (COMMAND("PWD")) {
	  /*
	  257
	  500, 501, 502, 421, 550
	  */
	  char pwd[1024];
	  getcwd(pwd, sizeof(pwd));
	  fprintf(fd, "%s\n", pwd);
      }
      else if (COMMAND("PASS")) {
	  /*
	  230
	  202
	  530
	  500, 501, 503,421
	  332
	  */
	  fscanf(fd, "%s", arg);
	  if(!strcmp("pepe", arg) == 0)
	    fprintf(fd, "230 User logged in, proceed\n");
	else{
		fprintf(fd, "332 Need account for login\n");
		//Salir;
	    }
	   
      }
      else if (COMMAND("PORT")) {
	  /*
	  200
	  500, 501, 421, 530
	  */
	  unsigned h1, h2, h3, h4, p1, p2;
	  fscanf(fd, "%u,%u,%u,%u,%u,%u", &h1, &h2, &h3, &h4, &p1, &p2);
	  uint32_t address;
	  uint16_t port;

	  address = h1 << 24 | h2 << 16 | h3 << 8 | h4;
	  port = p1 << 8 | p2;
	  data_socket = connect_TCP(address, port);

	fprintf(fd, "200 Command okay");
      }
      else if (COMMAND("PASV")) {
	  /*
	  227
	  500, 501, 502, 421, 530
	  */
	  unsigned h1, h2, h3, h4, p1, p2;
	  uint32_t address;
	  uint16_t port;
	  fscanf(fd, "%u,%u,%u,%u,%u,%u", &h1, &h2, &h3, &h4, &p1, &p2);
	  address = h1 << 24 | h2 << 16 | h3 << 8 | h4;
	  port = p1 << 8 | p2;
	struct sockaddr_in sin;
	sin.sin_port = port;
	sin.sin_addr.s_addr = address;
	socklen_t len = sizeof(sin);
	getsockname(control_socket, (sockaddr*)&sin, & len);
	fprintf(fd, "227 Entering Passive Mode %u.%u.%u.%u:%u.%u\n", h1, h2, h3, h4, p1, p2);
      }
      else if (COMMAND("CWD")) {
	  /*
	  250
	  500, 501, 502, 421, 530, 550
	  */
	  //fprintf(fd, "250 Requested file action okay, completed\n"); 
      }
      else if (COMMAND("STOR") ) { // Revisar, hablar con el profe
	  	
		fscanf(fd, "%s", arg);
		if(arg[0] != '\0')
			fprintf(fd, "125 Data connection already opened; transfer starting\n");
		/*
		125, 150
			(110)
			226, 250
			425, 426, 451, 551, 552
		532, 450, 452, 553
		500, 501, 421, 530
		*/
		struct sockaddr_in sin;
		int rval;
		
		char buffer[2000];
		char nombre[1024];
		int remain_data;
		ssize_t len;
		FILE* archivo_recibido;
		archivo_recibido = fopen(arg, "w");

		if(!archivo_recibido){
			exit(EXIT_FAILURE);
			fprintf(fd, "450 Requested file action not taken\n");
		}

		while(1) {
			int bytes = recv(data_socket, buffer, 2000, 0);

			fwrite(buffer, 1, bytes, archivo_recibido);

			if (bytes != 2000)
				break;
		}

		fclose(archivo_recibido);
		fprintf(fd, "226 Closing data connection. Requested file action successful\n");	
      }
      else if (COMMAND("SYST")) {
		  fprintf(fd, "215 UNIX system type\n");

	   /*
	   215
	   500, 501, 502, 421
	   */

      }
      else if (COMMAND("TYPE")) {
		  fscanf(fd, "%s", arg);
		  fprintf(fd, "200 ok\n");
		  /*
		  200
		  500, 501, 504, 421, 530
		  */
		  
      }
      else if (COMMAND("RETR")) {
		  /*
		125, 150
			(110)
			226, 250
			425, 426, 451, 551, 552
		532, 450, 452, 553
		500, 501, 421, 530
		*/
		fscanf(fd, "%s", arg);
		if(arg[0] != '\0')
			fprintf(fd, "125 Data connection already opened; transfer starting\n");
		
		struct sockaddr_in sin;
		int rval;
		
		char buffer[2000];
		char nombre[1024];
		int remain_data;
		ssize_t len;
		FILE* archivo_mandar;
		archivo_mandar = fopen(arg, "r");

		if(!archivo_mandar){ //No se abre el archivo
			exit(EXIT_FAILURE);
			fprintf(fd, "450 Requested file action not taken\n");
		}		

		int bytes = 2000;
		while(1) {

			fread(buffer, 1, bytes, archivo_mandar);

			bytes = send(data_socket, buffer, 2000, 0);

			if (bytes != 2000)
				break;
		}

		fclose(archivo_mandar);
		fprintf(fd, "226 Closing data connection. Requested file action successful\n");
	   
      }
      else if (COMMAND("QUIT")) {
		/*
		221
		500
		*/
		fprintf(fd, "Service closing control connection\n");
		return;
      }
      else if (COMMAND("LIST")) {
		/*
		Returns information of a file or directory if specified, else information of the current working directory is returned
		*/
		/*
		125, 150
			226, 250
			425, 426, 451
		450
		500, 501, 502, 421, 530
		*/
		fprintf(fd, "125 Data connection already opened; transfer starting\n");
		fprintf(fd, "150 File status okay; about to open data connection\n");

      }
      else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
	
      }
      
    }
    
    fclose(fd);

    
    return;
  
};
