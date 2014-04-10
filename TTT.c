#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>


int isGameInProgress = 0;
int numPlayers = 0;
int isPlayerWaitingForGame = 0;
int numClientsInQueue = 0;
int isMaxCapcityReached = 0;
int isXTurn = 0;

char pos1 = ' ';
char pos2 = ' ';
char pos3 = ' ';
char pos4 = ' ';
char pos5 = ' ';
char pos6 = ' ';
char pos7 = ' ';
char pos8 = ' ';
char pos9 = ' ';

char XBoardPlaces[5];
char OBoardPlaces[5];

char playerXHandle[32];
char playerOHandle[32];

int playerXVictory = 0;
int playerOVictory = 0;

int tmpPortNum;

void sendDatagram(char message[34],char socket_num_str[8],char hostName[16]);

char* recieveDatagram(char message[34], char socket_num_str[8],char hostName[16]);

void printsin(struct sockaddr_in*, char*, char*);

int setupUdp(char socket_num_str[8]);

int sendTcp(char* hostName, char* portNum,char msg[34]);

int setupTcpV2();

int getTcpSendFd(char* hostName, char* portNum);

int writeTcp(char serverHostName[16],char serverPortNum[6], char message[34]);

int didXWin();

int didOWin();

int isDraw();

void resetBoard();

int main(int argc, char *argv[]){
  
  char hostName[16];
  hostName[15] = '\0';
  gethostname(hostName, 15);//get the name of the computer we're running on
  struct hostent* h;
  h = gethostbyname(hostName);
  strncpy(hostName,h->h_name,16);

  
  char OHostName[16];
  char OPortNum[6];
  char XHostName[16];
  char XPortNum[6];
  char OUDPPortNum[6];
  char XUDPPortNum[6];
  
  char Qclient1HostName[16];
  char Qclient2HostName[16];
  char Qclient3HostName[16];
  char Qclient4HostName[16];
  char Qclient5HostName[16];
  char Qclient1PortNum[6];
  char Qclient2PortNum[6];
  char Qclient3PortNum[6];
  char Qclient4PortNum[6];
  char Qclient5PortNum[6];
  
  
  int gotMove = 1;
  
  fd_set mask;
  
  int xPlayerPortNum;//write to x player
  int oPlayerPortNum;//write to O player
  
  int connX;//read from X player 
  int connO;//read from O player
  
  char message[34];
  char response[34];
  
  char recieve_socket_num_str[6] = "23124\0";//the socket we're listening to for datagrams
  char send_socket_num_str[6] = "56547\0";//the socket for that ttt is listening on for a response datagram
  int datagramFd = -1;//the fd we should be listening on for incoming datagrams (-1 is a placeholder)
  int tcpXPlayerFd = -1;//the fd for the X player TCP socket (-1 is a placeholder)
  int tcpOPlayerFd = -2;//the fd for the O player TCP socket (-2 is a placeholder)
  
  FILE *fp;
  int ul = unlink("socket");
  if (ul < 0){
      fprintf(stderr,"Problem with unlinking file 'socket'. Do we have r/w access?\n");
      exit(1);
  }
  fp = fopen("socket", "ab+");
  fputs(recieve_socket_num_str,fp);
  fputs("\n",fp);
  fputs(send_socket_num_str,fp);
  fputs("\n",fp);
  fputs(hostName,fp);
  //fp = fopen("socket", "rw");
  //fscanf(fp,"%s",recieve_socket_num_str);
  //fscanf(fp,"%s",send_socket_num_str);
  //fscanf(fp,"%s",hostName);
  fclose(fp);
  
  fprintf(stderr,"recieve_socket_num_str is %s\n",recieve_socket_num_str);
  fprintf(stderr,"send_socket_num_str is %s\n",send_socket_num_str);
  //fprintf(stderr,"host name is: %s\n",hostName);
 
  if (argc > 1){
      fprintf(stderr,"TTT should not be called with arguments! Exiting...\n");
      exit(1);
  }
  
  fprintf(stderr,"About to get the X TCP Port!\n");
  tcpXPlayerFd = setupTcpV2();
  listen(tcpXPlayerFd,1);
  xPlayerPortNum = tmpPortNum;
  
  fprintf(stderr,"About to get the O TCP Port!\n");
  tcpOPlayerFd = setupTcpV2();
  listen(tcpOPlayerFd,1);
  oPlayerPortNum = tmpPortNum;
  
  datagramFd = setupUdp(recieve_socket_num_str);
  fprintf(stderr,"Fd we're recieving datagrams on is: %d\n",datagramFd);
  fprintf(stderr,"Fd we're recieving X's TCP on is: %d\n",tcpXPlayerFd);
  fprintf(stderr,"Fd we're recieving O's TCP on is: %d\n",tcpOPlayerFd);
  
  fprintf(stderr,"listening!\n");

 // printf("\n\nRSTREAM:: data from stream:\n");
  //while ( read(conn, &ch, 1) == 1)
  //  putchar(ch);
  //putchar('\n');
  
  struct timeval timeout;
  int hits;
  struct sockaddr_in peer,from;
  socklen_t length;
  char ch;
    
  int gotXPortInfo = 0;
  int gotOPortInfo = 0;
  
  while (1){//This loop should never terminate  
    
    if ((isGameInProgress) && (gotOPortInfo) && (gotXPortInfo)){
      memset(response,0,34);//reset the message
      response[0] = 'Y';
      int result = writeTcp(OHostName,OPortNum,response);
      if (result){//if O died
	  printf("O died\n!");
	  memset(response,0,34);//reset the message
	  response[0] = 'R';
	  response[1] = 'W';
	  int result = writeTcp(XHostName,XPortNum,response);
	   //do more to prepare for another game
	      isGameInProgress = 0;
	      numPlayers = (numPlayers-2);
	      gotMove = 1;
	      resetBoard();
	      gotXPortInfo = 0;
	      gotOPortInfo = 0;
	      FD_ZERO(&mask);
	      FD_SET(datagramFd,&mask);
	      FD_SET(tcpOPlayerFd,&mask);
	    FD_SET(tcpXPlayerFd,&mask);
	    continue;
      }
      memset(response,0,34);//reset the message
      response[0] = 'Y';
      result = writeTcp(XHostName,XPortNum,response);
      if (result){//if X died
	  printf("X died\n!");
	  memset(response,0,34);//reset the message
	  response[0] = 'R';
	  response[1] = 'W';
	  int result = writeTcp(OHostName,OPortNum,response);
	   //do more to prepare for another game
	      isGameInProgress = 0;
	      numPlayers = (numPlayers-2);
	      gotMove = 1;
	      resetBoard();
	      gotXPortInfo = 0;
	      gotOPortInfo = 0;
	      FD_ZERO(&mask);
	      FD_SET(datagramFd,&mask);
	      FD_SET(tcpOPlayerFd,&mask);
	    FD_SET(tcpXPlayerFd,&mask);
	    continue;
      }
    }
    
    //Yes, we need to reset all these values every time we loop through- otherwise select gets screwed up
    FD_ZERO(&mask);
    FD_SET(datagramFd,&mask);
    FD_SET(tcpOPlayerFd,&mask);
    FD_SET(tcpXPlayerFd,&mask);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if ((hits = select(20, &mask, (fd_set *)0, (fd_set *)0,
                           &timeout)) < 0) {
      perror("fancy_recv_udp:select");
      exit(1);
    }
    if ( ((hits>0) && (FD_ISSET(datagramFd,&mask))) ) {
      fprintf(stderr,"Datagram received!\n");
      
      socklen_t fsize = sizeof(from);//this is basically an int- but GCC whines if you actually declare it an int
      
       int cc = recvfrom(datagramFd,&message,sizeof(message),0,(struct sockaddr *)&from,&fsize);
	if (cc < 0) perror("recv_udp:recvfrom");
      printsin( &from, "recv_udp: ", "Packet from:");
      printf("Got data ::%s\n",message);
      fflush(stdout);
      memset(response,0,34);//reset the message
      
      if (message[0] == 'Q'){
	  fprintf(stderr,"Querying client!");
	  //querying client
	  if (isGameInProgress == 0){
	      memset(response,0,34);//reset the message
	      response[0] = 'I';
	      sendDatagram(response,send_socket_num_str,hostName);
	      continue;
	  }
	  else{
	    memset(message,0,34);//reset the message
	    message[0] = 'B';
	    message[1] = pos1;
	    message[2] = pos2;
	    message[3] = pos3;
	    message[4] = pos4;
	    message[5] = pos5;
	    message[6] = pos6;
	    message[7] = pos7;
	    message[8] = pos8;
	    message[9] = pos9;
	    sleep(1);
	    sendDatagram(message,send_socket_num_str,hostName);
	    
	    memset(message,0,34);//reset the message
	    message[0] = 'H';
	    int j;
	    for (j=0;j < 34; j++){
	      message[j+1] = playerOHandle[j];
	    }
	    sleep(1);
	    sendDatagram(message,send_socket_num_str,hostName);
	    
	    memset(message,0,34);//reset the message
	    message[0] = 'H';
	    for (j=0;j < 34; j++){
	      message[j+1] = playerXHandle[j];
	    }
	    sleep(1);
	    sendDatagram(message,send_socket_num_str,hostName);
	    continue;
	  }
	  
      }
      
      
      if (isMaxCapcityReached){
	  Qclient5PortNum[0] = message[1];
	  Qclient5PortNum[1] = message[2];
	Qclient5PortNum[2] = message[3];
	Qclient5PortNum[3] = message[4];
	Qclient5PortNum[4] = message[5];
	Qclient5HostName[0] = message[6];
	Qclient5HostName[1] = message[7];
	Qclient5HostName[2] = message[8];
	Qclient5HostName[3] = message[9];
	Qclient5HostName[4] = message[10];
	Qclient5HostName[5] = message[11];
	Qclient5HostName[6] = message[12];
	Qclient5HostName[7] = message[13];
	Qclient5HostName[8] = message[14];
	Qclient5HostName[9] = message[15];
	Qclient5HostName[10] = message[16];
	Qclient5HostName[11] = message[17];
	Qclient5HostName[12] = message[18];
	Qclient5HostName[13] = message[19];
	Qclient5HostName[14] = message[20];
	int Qclient5PortNumInt = strtol(Qclient5PortNum,NULL,10);//convert to int
	memset(&Qclient5PortNum,0,8);
	sprintf(Qclient5PortNum,"%d",Qclient5PortNumInt);//Convert port number to a string, stripping out the padding zeros
	  
	  //tell client the server is unreachable
	  response[0] = 'N';
	  sendDatagram(response,Qclient5PortNum,Qclient5HostName);
	  memset(response,0,34);//reset the message
      }
      else if (isGameInProgress){
	  //tell client to wait
	  fprintf(stderr,"telling client to wait!\n");
	  numPlayers++;
	  numClientsInQueue++;
	  if (numClientsInQueue == 4){
	      isMaxCapcityReached = 1;
	  }
	  
	  //record this client's datagram port/hostname
	  //get client info
	 if (('P' == message[0]) && (numClientsInQueue == 1)){
	  //player UDP port
	Qclient1PortNum[0] = message[1];
	Qclient1PortNum[1] = message[2];
	Qclient1PortNum[2] = message[3];
	Qclient1PortNum[3] = message[4];
	Qclient1PortNum[4] = message[5];
	Qclient1HostName[0] = message[6];
	Qclient1HostName[1] = message[7];
	Qclient1HostName[2] = message[8];
	Qclient1HostName[3] = message[9];
	Qclient1HostName[4] = message[10];
	Qclient1HostName[5] = message[11];
	Qclient1HostName[6] = message[12];
	Qclient1HostName[7] = message[13];
	Qclient1HostName[8] = message[14];
	Qclient1HostName[9] = message[15];
	Qclient1HostName[10] = message[16];
	Qclient1HostName[11] = message[17];
	Qclient1HostName[12] = message[18];
	Qclient1HostName[13] = message[19];
	Qclient1HostName[14] = message[20];
	int Qclient1PortNumInt = strtol(Qclient1PortNum,NULL,10);//convert to int
	memset(&Qclient1PortNum,0,8);
	sprintf(Qclient1PortNum,"%d",Qclient1PortNumInt);//Convert port number to a string, stripping out the padding zeros
	//fprintf(stderr,"Got Qclient1's UDP info, Port No is: %s and Hostname is: %s\n",Qclient1PortNum,Qclient1HostName);
	
	//tell the client to be patient
	response[0] = 'W';
	sendDatagram(response,Qclient1PortNum,Qclient1HostName);
	memset(response,0,34);//reset the message
      }
      
       if (('P' == message[0]) && (numClientsInQueue == 2)){
	  //player UDP port
	Qclient2PortNum[0] = message[1];
	Qclient2PortNum[1] = message[2];
	Qclient2PortNum[2] = message[3];
	Qclient2PortNum[3] = message[4];
	Qclient2PortNum[4] = message[5];
	Qclient2HostName[0] = message[6];
	Qclient2HostName[1] = message[7];
	Qclient2HostName[2] = message[8];
	Qclient2HostName[3] = message[9];
	Qclient2HostName[4] = message[10];
	Qclient2HostName[5] = message[11];
	Qclient2HostName[6] = message[12];
	Qclient2HostName[7] = message[13];
	Qclient2HostName[8] = message[14];
	Qclient2HostName[9] = message[15];
	Qclient2HostName[10] = message[16];
	Qclient2HostName[11] = message[17];
	Qclient2HostName[12] = message[18];
	Qclient2HostName[13] = message[19];
	Qclient2HostName[14] = message[20];
	int Qclient2PortNumInt = strtol(Qclient2PortNum,NULL,10);//convert to int
	memset(&Qclient2PortNum,0,8);
	sprintf(Qclient2PortNum,"%d",Qclient2PortNumInt);//Convert port number to a string, stripping out the padding zeros
	//fprintf(stderr,"Got Qclient2's UDP info, Port No is: %s and Hostname is: %s\n",Qclient1PortNum,Qclient2HostName);
	
	//tell the client to be patient
	response[0] = 'W';
	sendDatagram(response,Qclient2PortNum,Qclient2HostName);
	memset(response,0,34);//reset the message
      }
      
      if (('P' == message[0]) && (numClientsInQueue == 3)){
	  //player UDP port
	Qclient3PortNum[0] = message[1];
	Qclient3PortNum[1] = message[2];
	Qclient3PortNum[2] = message[3];
	Qclient3PortNum[3] = message[4];
	Qclient3PortNum[4] = message[5];
	Qclient3HostName[0] = message[6];
	Qclient3HostName[1] = message[7];
	Qclient3HostName[2] = message[8];
	Qclient3HostName[3] = message[9];
	Qclient3HostName[4] = message[10];
	Qclient3HostName[5] = message[11];
	Qclient3HostName[6] = message[12];
	Qclient3HostName[7] = message[13];
	Qclient3HostName[8] = message[14];
	Qclient3HostName[9] = message[15];
	Qclient3HostName[10] = message[16];
	Qclient3HostName[11] = message[17];
	Qclient3HostName[12] = message[18];
	Qclient3HostName[13] = message[19];
	Qclient3HostName[14] = message[20];
	int Qclient3PortNumInt = strtol(Qclient3PortNum,NULL,10);//convert to int
	memset(&Qclient3PortNum,0,8);
	sprintf(Qclient3PortNum,"%d",Qclient3PortNumInt);//Convert port number to a string, stripping out the padding zeros
	//fprintf(stderr,"Got Qclient2's UDP info, Port No is: %s and Hostname is: %s\n",Qclient1PortNum,Qclient2HostName);
	
	//tell the client to be patient
	response[0] = 'W';
	sendDatagram(response,Qclient3PortNum,Qclient3HostName);
	memset(response,0,34);//reset the message
      }
      
      if (('P' == message[0]) && (numClientsInQueue == 4)){
	  //player UDP port
	Qclient4PortNum[0] = message[1];
	Qclient4PortNum[1] = message[2];
	Qclient4PortNum[2] = message[3];
	Qclient4PortNum[3] = message[4];
	Qclient4PortNum[4] = message[5];
	Qclient4HostName[0] = message[6];
	Qclient4HostName[1] = message[7];
	Qclient4HostName[2] = message[8];
	Qclient4HostName[3] = message[9];
	Qclient4HostName[4] = message[10];
	Qclient4HostName[5] = message[11];
	Qclient4HostName[6] = message[12];
	Qclient4HostName[7] = message[13];
	Qclient4HostName[8] = message[14];
	Qclient4HostName[9] = message[15];
	Qclient4HostName[10] = message[16];
	Qclient4HostName[11] = message[17];
	Qclient4HostName[12] = message[18];
	Qclient4HostName[13] = message[19];
	Qclient4HostName[14] = message[20];
	int Qclient4PortNumInt = strtol(Qclient4PortNum,NULL,10);//convert to int
	memset(&Qclient4PortNum,0,8);
	sprintf(Qclient4PortNum,"%d",Qclient4PortNumInt);//Convert port number to a string, stripping out the padding zeros
	//fprintf(stderr,"Got Qclient2's UDP info, Port No is: %s and Hostname is: %s\n",Qclient1PortNum,Qclient2HostName);
	
	//tell the client to be patient
	response[0] = 'W';
	sendDatagram(response,Qclient4PortNum,Qclient4HostName);
	memset(response,0,34);//reset the message
      }
	/*  
	  sleep(2);//Give time for the client to monitor their datagram socket
	  response[0] = 'W';//Tell client to wait until the game is finished
	  sendDatagram(response,send_socket_num_str,hostName);
	  memset(response,0,34);//reset the message
	  */
      }
      else if (numPlayers == 1){
	  //start a new game
	numPlayers++;
	
	
	//get client info
	 if ('P' == message[0]){
	  //player O UDP port
	XUDPPortNum[0] = message[1];
	XUDPPortNum[1] = message[2];
	XUDPPortNum[2] = message[3];
	XUDPPortNum[3] = message[4];
	XUDPPortNum[4] = message[5];
	XHostName[0] = message[6];
	XHostName[1] = message[7];
	XHostName[2] = message[8];
	XHostName[3] = message[9];
	XHostName[4] = message[10];
	XHostName[5] = message[11];
	XHostName[6] = message[12];
	XHostName[7] = message[13];
	XHostName[8] = message[14];
	XHostName[9] = message[15];
	XHostName[10] = message[16];
	XHostName[11] = message[17];
	XHostName[12] = message[18];
	XHostName[13] = message[19];
	XHostName[14] = message[20];
	int XUDPPortNumInt = strtol(XUDPPortNum,NULL,10);//convert to int
	memset(&XUDPPortNum,0,8);
	sprintf(XUDPPortNum,"%d",XUDPPortNumInt);//Convert port number to a string, stripping out the padding zeros
	fprintf(stderr,"Got X's UDP info, X Port No is: %s and Hostname is: %s\n",XUDPPortNum,XHostName);
      }
	/*
	  response[0] = 'A';//Accepted the connection
	  //sendDatagram(response,send_socket_num_str,XHostName);
	  sendDatagram(response,XUDPPortNum,XHostName);
	  memset(response,0,34);//reset the message
	  */
	  char portNumStr[8];
	  sprintf(portNumStr,"%d",xPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	
	//sleep(2);//Give time for the client to monitor their datagram socket
	fprintf(stderr,"Datagram sending to client with portNumber is: %s\n",response);
	//sendDatagram(response,send_socket_num_str,XHostName);
	sendDatagram(response,XUDPPortNum,XHostName);
	
	
	  response[0] = 'A';//Accepted the connection
	  //sendDatagram(response,send_socket_num_str,XHostName);
	  sendDatagram(response,XUDPPortNum,XHostName);
	  memset(response,0,34);//reset the message
	  isGameInProgress = 1;
	
      }
      else{
	  //this is the first connection- tell client to wait
	numPlayers++;
	
	//get client info
	
	//get client info
	 if ('P' == message[0]){
	  //player O UDP port
	OUDPPortNum[0] = message[1];
	OUDPPortNum[1] = message[2];
	OUDPPortNum[2] = message[3];
	OUDPPortNum[3] = message[4];
	OUDPPortNum[4] = message[5];
	OHostName[0] = message[6];
	OHostName[1] = message[7];
	OHostName[2] = message[8];
	OHostName[3] = message[9];
	OHostName[4] = message[10];
	OHostName[5] = message[11];
	OHostName[6] = message[12];
	OHostName[7] = message[13];
	OHostName[8] = message[14];
	OHostName[9] = message[15];
	OHostName[10] = message[16];
	OHostName[11] = message[17];
	OHostName[12] = message[18];
	OHostName[13] = message[19];
	OHostName[14] = message[20];
	int OUDPPortNumInt = strtol(OUDPPortNum,NULL,10);//convert to int
	memset(&OUDPPortNum,0,8);
	sprintf(OUDPPortNum,"%d",OUDPPortNumInt);//Convert port number to a string, stripping out the padding zeros
	fprintf(stderr,"Got O's UDP info, O Port No is: %s and Hostname is: %s\n",OUDPPortNum,OHostName);
      }
      
	/*
	  response[0] = 'A';//Accepted the connection
	  //sendDatagram(response,send_socket_num_str,OHostName);
	  sendDatagram(response,OUDPPortNum,OHostName);
	  fprintf(stderr,"Datagram sending to client with portNumber is: %s\n",response);
	  memset(response,0,34);//reset the message
	  */
	   char portNumStr[8];
	  sprintf(portNumStr,"%d",oPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];

	//sleep(2);//Give time for the client to monitor their datagram socket
	fprintf(stderr,"Datagram sending to client with portNumber is: %s\n",response);
	//sendDatagram(response,send_socket_num_str,OHostName);
	sendDatagram(response,OUDPPortNum,OHostName);
	
	
	  response[0] = 'A';//Accepted the connection
	  //sendDatagram(response,send_socket_num_str,OHostName);
	  sendDatagram(response,OUDPPortNum,OHostName);
	  fprintf(stderr,"Datagram sending to client with portNumber is: %s\n",response);
	  memset(response,0,34);//reset the message
	  

      }
  }
      
    
    if (((hits>0) && (FD_ISSET(tcpOPlayerFd,&mask))) ) {
      fprintf(stderr,"O TCP received!\n");
      memset(message,0,34);//reset the message
      fprintf(stderr,"tcpOPlayerFd is: %d\n",tcpOPlayerFd);
      
      length = sizeof(socklen_t);
      if ((connO=accept(tcpOPlayerFd, (struct sockaddr *)&peer, &length)) < 0) {
	perror("inet_rstream:accept");
	exit(1);
      }
      else{
	  fprintf(stderr,"conn0 is: %d\n",connO);
      }
      int i;
      for (i = 0;i < 34; i++){
	read(connO, &ch, 1);
        //fprintf(stderr,"%c",ch); 
	message[i] = ch;
      }
      fprintf(stderr,"Finished reading message! Message is: %s\n",message);
      
      if('\n' == message[0]){
	fprintf(stderr,"newline on O tcp!\n");
      }
      
      if((' ' == message[0]) || (NULL == message[0])){
	fprintf(stderr,"blank message on O tcp!\n");
      }
      
      //Now to parse that message
      if ('H' == message[0]){
	  //player handle
	  for (i = 0; i < 31; i++){
	      playerOHandle[i] = message[i+1];
	  }
	  fprintf(stderr,"Got handle, playerOhandle is: %s\n",playerOHandle);
      }
      
      if ('P' == message[0]){
	  //player O TCP port
	OPortNum[0] = message[1];
	OPortNum[1] = message[2];
	OPortNum[2] = message[3];
	OPortNum[3] = message[4];
	OPortNum[4] = message[5];
	OHostName[0] = message[6];
	OHostName[1] = message[7];
	OHostName[2] = message[8];
	OHostName[3] = message[9];
	OHostName[4] = message[10];
	OHostName[5] = message[11];
	OHostName[6] = message[12];
	OHostName[7] = message[13];
	OHostName[8] = message[14];
	OHostName[9] = message[15];
	OHostName[10] = message[16];
	OHostName[11] = message[17];
	OHostName[12] = message[18];
	OHostName[13] = message[19];
	OHostName[14] = message[20];
	
	int OPortNumInt = strtol(OPortNum,NULL,10);//convert to int
	memset(&OPortNum,0,8);
	sprintf(OPortNum,"%d",OPortNumInt);//Convert port number to a string
	fprintf(stderr,"Got O's TCP info, O Port No is: %s and Hostname is: %s\n",OPortNum,OHostName);
	
	gotOPortInfo = 1;
      }
      
      if ('M' == message[0]){
	  //got O's move
	  
	  fprintf(stderr,"Got O's move- it was %c\n",message[1]);
	  if (message[1] == '9'){
	      pos9 = 'O';
	  }
	  if (message[1] == '8'){
	      pos8 = 'O';
	  }
	  if (message[1] == '7'){
	      pos7 = 'O';
	  }
	  if (message[1] == '6'){
	      pos6 = 'O';
	  }
	  if (message[1] == '5'){
	      pos5 = 'O';
	  }
	  if (message[1] == '4'){
	      pos4 = 'O';
	  }
	  if (message[1] == '3'){
	      pos3 = 'O';
	  }
	  if (message[1] == '2'){
	      pos2 = 'O';
	  }
	  if (message[1] == '1'){
	      pos1 = 'O';
	  }
	  if (didOWin()){
	      fprintf(stderr,"O Won!\n");
	      
	       //alert O that they won
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'W';
	      //actually write the message
	      writeTcp(OHostName,OPortNum,message);
	      
	       //alert X that they lost
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'L';
	      //actually write the message
	      writeTcp(XHostName,XPortNum,message);
	      
	      //do more to prepare for another game
	      isGameInProgress = 0;
	      numPlayers = (numPlayers-2);
	      gotMove = 1;
	      resetBoard();
	      gotXPortInfo = 0;
	      gotOPortInfo = 0;
	      //if there are clients in the queue, deal with them
	      //deal with any clients
	      if(numClientsInQueue > 1){
	      //BEGIN copypasta
	      //get client info
	      strncpy(OUDPPortNum,Qclient2PortNum,5);
	      strncpy(OHostName,Qclient2HostName,15);
	      strncpy(XUDPPortNum,Qclient1PortNum,5);
	      strncpy(XHostName,Qclient1HostName,15);
	      
	  char portNumStr[8];
	  sprintf(portNumStr,"%d",xPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	
	sendDatagram(response,XUDPPortNum,XHostName);
	
	  //char portNumStr[8];
	  sprintf(portNumStr,"%d",oPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	sendDatagram(response,OUDPPortNum,OHostName);
	
	
	  memset(response,0,34);//reset the message
	  response[0] = 'A';//Accepted the connection
	  isGameInProgress = 1;
	  //gotXPortInfo = 1;
	  //gotOPortInfo = 1;
	  sendDatagram(response,OUDPPortNum,OHostName);
	  sendDatagram(response,XUDPPortNum,XHostName);
	  memset(response,0,34);//reset the message

	       isXTurn = 0;
	      //END copypasta
	      }
	      
	      //We have to do this, otherwise select is screwed up
	      close(connO);
	      close(connX);
	      FD_ZERO(&mask);
	      FD_SET(tcpOPlayerFd,&mask);
	      FD_SET(tcpXPlayerFd,&mask);
	      continue;
	      
	  }
	  if (isDraw()){
	     //alert X that they drew
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'D';
	      //actually write the message
	      writeTcp(XHostName,XPortNum,message);
	      
	       //alert O that they drew
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'D';
	      //actually write the message
	      writeTcp(OHostName,OPortNum,message);
	      
	      //do more to prepare for another game
	      isGameInProgress = 0;
	      numPlayers = (numPlayers-2);
	      gotMove = 1;
	      resetBoard();
	      gotXPortInfo = 0;
	      gotOPortInfo = 0;
	      //deal with any clients
	      if(numClientsInQueue > 1){
	      //BEGIN copypasta
	      //get client info
	      strncpy(OUDPPortNum,Qclient2PortNum,5);
	      strncpy(OHostName,Qclient2HostName,15);
	      strncpy(XUDPPortNum,Qclient1PortNum,5);
	      strncpy(XHostName,Qclient1HostName,15);
	      
	  char portNumStr[8];
	  sprintf(portNumStr,"%d",xPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	
	sendDatagram(response,XUDPPortNum,XHostName);
	
	  //char portNumStr[8];
	  sprintf(portNumStr,"%d",oPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	sendDatagram(response,OUDPPortNum,OHostName);
	
	
	  memset(response,0,34);//reset the message
	  response[0] = 'A';//Accepted the connection
	  isGameInProgress = 1;
	  //gotXPortInfo = 1;
	  //gotOPortInfo = 1;
	  sendDatagram(response,OUDPPortNum,OHostName);
	  sendDatagram(response,XUDPPortNum,XHostName);
	  memset(response,0,34);//reset the message

	       isXTurn = 0;
	      //END copypasta
	      }
	      //We have to do this, otherwise select is screwed up
	      close(connX);
	      close(connO);
	      FD_ZERO(&mask);
	      FD_SET(tcpXPlayerFd,&mask);
	      FD_SET(tcpOPlayerFd,&mask);
	      continue;
	  }
	  gotMove = 1;//we can now contact X
      }
      
      //We have to do this, otherwise select is screwed up
      close(connO);
      FD_ZERO(&mask);
      FD_SET(tcpOPlayerFd,&mask);
      continue;
    }
    
    if (((hits>0) && (FD_ISSET(tcpXPlayerFd,&mask))) ) {
      fprintf(stderr,"X TCP received!\n");
      memset(message,0,34);//reset the message

      length = sizeof(socklen_t);
      if ((connX=accept(tcpXPlayerFd, (struct sockaddr *)&peer, &length)) < 0) {
	perror("inet_rstream:accept");

      }
      else{
	  fprintf(stderr,"connX is: %d\n",connX);
      }
       int i;
      for (i = 0;i < 34; i++){
	read(connX, &ch, 1);
        //fprintf(stderr,"%c",ch); 
	message[i] = ch;
      }
      fprintf(stderr,"Finished reading message! Message is: %s\n",message);
      
      
       if('\n' == message[0]){
	fprintf(stderr,"newline on X tcp!\n");
      }
      
      if((' ' == message[0]) || (NULL == message[0])){
	fprintf(stderr,"blank message on X tcp!\n");
      }
      
      //Now to parse that message
      if ('H' == message[0]){
	  //player handle
	  for (i = 0; i < 31; i++){
	      playerXHandle[i] = message[i+1];
	  }
	  fprintf(stderr,"Got handle, playerXhandle is: %s\n",playerXHandle);
      }
      
      if ('M' == message[0]){
	  //got X's move
	  
	  fprintf(stderr,"Got X's move- it was %c\n",message[1]);
	  if (message[1] == '9'){
	      pos9 = 'X';
	  }
	  if (message[1] == '8'){
	      pos8 = 'X';
	  }
	  if (message[1] == '7'){
	      pos7 = 'X';
	  }
	  if (message[1] == '6'){
	      pos6 = 'X';
	  }
	  if (message[1] == '5'){
	      pos5 = 'X';
	  }
	  if (message[1] == '4'){
	      pos4 = 'X';
	  }
	  if (message[1] == '3'){
	      pos3 = 'X';
	  }
	  if (message[1] == '2'){
	      pos2 = 'X';
	  }
	  if (message[1] == '1'){
	      pos1 = 'X';
	  }
	  if (didXWin()){
	      fprintf(stderr,"X Won!\n");
	      
	        //alert X that they won
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'W';
	      //actually write the message
	      writeTcp(XHostName,XPortNum,message);
	      
	       //alert O that they lost
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'L';
	      //actually write the message
	      writeTcp(OHostName,OPortNum,message);
	      
	      //do more to prepare for another game
	      isGameInProgress = 0;
	      numPlayers = (numPlayers-2);
	      gotMove = 1;
	      resetBoard();
	      gotXPortInfo = 0;
	      gotOPortInfo = 0;
	      //deal with any clients
	      if(numClientsInQueue > 1){
	      //BEGIN copypasta
	      //get client info
	      strncpy(OUDPPortNum,Qclient2PortNum,5);
	      strncpy(OHostName,Qclient2HostName,15);
	      strncpy(XUDPPortNum,Qclient1PortNum,5);
	      strncpy(XHostName,Qclient1HostName,15);
	      
	  char portNumStr[8];
	  sprintf(portNumStr,"%d",xPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	
	sendDatagram(response,XUDPPortNum,XHostName);
	
	  //char portNumStr[8];
	  sprintf(portNumStr,"%d",oPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	sendDatagram(response,OUDPPortNum,OHostName);
	
	
	  memset(response,0,34);//reset the message
	  response[0] = 'A';//Accepted the connection
	  isGameInProgress = 1;
	  //gotXPortInfo = 1;
	  //gotOPortInfo = 1;
	  sendDatagram(response,OUDPPortNum,OHostName);
	  sendDatagram(response,XUDPPortNum,XHostName);
	  memset(response,0,34);//reset the message
	  isXTurn = 0;
	      
	      //END copypasta
	      }
	      
	      //We have to do this, otherwise select is screwed up
	      close(connX);
	      close(connO);
	      FD_ZERO(&mask);
	      FD_SET(tcpXPlayerFd,&mask);
	      FD_SET(tcpOPlayerFd,&mask);
	      continue;
	  }
	  if (isDraw()){
	     //alert X that they drew
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'D';
	      //actually write the message
	      writeTcp(XHostName,XPortNum,message);
	      
	       //alert O that they drew
	      memset(message,0,34);//reset the message
	      message[0] = 'R';
	      message[1] = 'D';
	      //actually write the message
	      writeTcp(OHostName,OPortNum,message);
	      
	      //do more to prepare for another game
	      isGameInProgress = 0;
	      numPlayers = (numPlayers-2);
	      gotMove = 1;
	      resetBoard();
	      gotXPortInfo = 0;
	      gotOPortInfo = 0;
	      //deal with any clients
	      if(numClientsInQueue > 1){
	      //BEGIN copypasta
	      //get client info
	      strncpy(OUDPPortNum,Qclient2PortNum,5);
	      strncpy(OHostName,Qclient2HostName,15);
	      strncpy(XUDPPortNum,Qclient1PortNum,5);
	      strncpy(XHostName,Qclient1HostName,15);
	      
	  char portNumStr[8];
	  sprintf(portNumStr,"%d",xPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	
	sendDatagram(response,XUDPPortNum,XHostName);
	
	  //char portNumStr[8];
	  sprintf(portNumStr,"%d",oPlayerPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = hostName[0];
	  response[7] = hostName[1];
	  response[8] = hostName[2];
	  response[9] = hostName[3];
	  response[10] = hostName[4];
	  response[11] = hostName[5];
	  response[12] = hostName[6];
	  response[13] = hostName[7];
	  response[14] = hostName[8];
	  response[15] = hostName[9];
	  response[16] = hostName[10];
	  response[17] = hostName[11];
	  response[18] = hostName[12];
	  response[19] = hostName[13];
	  response[20] = hostName[14];
	sendDatagram(response,OUDPPortNum,OHostName);
	
	
	  memset(response,0,34);//reset the message
	  response[0] = 'A';//Accepted the connection
	  isGameInProgress = 1;
	  //gotXPortInfo = 1;
	  //gotOPortInfo = 1;
	  sendDatagram(response,OUDPPortNum,OHostName);
	  sendDatagram(response,XUDPPortNum,XHostName);
	  memset(response,0,34);//reset the message
	  isXTurn = 0;
	      
	      //END copypasta
	      }
	      
	      //We have to do this, otherwise select is screwed up
	      close(connX);
	      close(connO);
	      FD_ZERO(&mask);
	      FD_SET(tcpXPlayerFd,&mask);
	      FD_SET(tcpOPlayerFd,&mask);
	      continue;
	  }
	  gotMove = 1;//we can now contact O
      }
      
        if ('P' == message[0]){
	  //player X TCP port
	XPortNum[0] = message[1];
	XPortNum[1] = message[2];
	XPortNum[2] = message[3];
	XPortNum[3] = message[4];
	XPortNum[4] = message[5];
	XHostName[0] = message[6];
	XHostName[1] = message[7];
	XHostName[2] = message[8];
	XHostName[3] = message[9];
	XHostName[4] = message[10];
	XHostName[5] = message[11];
	XHostName[6] = message[12];
	XHostName[7] = message[13];
	XHostName[8] = message[14];
	XHostName[9] = message[15];
	XHostName[10] = message[16];
	XHostName[11] = message[17];
	XHostName[12] = message[18];
	XHostName[13] = message[19];
	XHostName[14] = message[20];
	int XPortNumInt = strtol(XPortNum,NULL,10);//convert to int
	memset(XPortNum,0,8);
	sprintf(XPortNum,"%d",XPortNumInt);//Convert port number to a string
	fprintf(stderr,"Got X's TCP info, X Port No is: %s and Hostname is: %s\n",XPortNum,XHostName);
	
	isGameInProgress = 1;
	gotXPortInfo = 1;
      }
      
      
      isXTurn = 0;
      
      //We have to do this, otherwise select is screwed up
      close(connX);
      FD_ZERO(&mask);
      FD_SET(tcpXPlayerFd,&mask);
      continue;
    }
    
       if ((isGameInProgress) && (gotOPortInfo) && (gotXPortInfo)){
	fprintf(stderr,"Game in progress!\n");
	if ((isXTurn) && (gotMove)){
	    //send instructions to X
	    fprintf(stderr,"It's X's Turn!\n");
	    
	     //alert X that it's their turn
	    memset(message,0,34);//reset the message
	    message[0] = 'T';
	      //actually write the message
	    writeTcp(XHostName,XPortNum,message);
	    
	     //Send board info to X
	    memset(message,0,34);//reset the message
	    message[0] = 'B';
	    message[1] = pos1;
	    message[2] = pos2;
	    message[3] = pos3;
	    message[4] = pos4;
	    message[5] = pos5;
	    message[6] = pos6;
	    message[7] = pos7;
	    message[8] = pos8;
	    message[9] = pos9;
	    //actually write the message
	    writeTcp(XHostName,XPortNum,message);
	    
	    //tell X they are X
	    memset(message,0,34);//reset the message
	    message[0] = 'S';
	    message[1] = 'X';
	   //actually write the message
	    writeTcp(XHostName,XPortNum,message);
	    /*
	    //alert X that it's their turn
	    memset(message,0,34);//reset the message
	    message[0] = 'T';
	      //actually write the message
	    writeTcp(XHostName,XPortNum,message);
	    */
	    isXTurn = 0;
	    gotMove = 0;//don't contact O until we've got a move back from X
	  
	}
	else if (gotMove){
	    //send instructions to O
	    fprintf(stderr,"It's O's Turn!\n");
	    
	    //alert O that it's their turn
	    memset(message,0,34);//reset the message
	    message[0] = 'T';
	      //actually write the message
	    writeTcp(OHostName,OPortNum,message);
	    
	     //Send board info to O
	    memset(message,0,34);//reset the message
	    message[0] = 'B';
	    message[1] = pos1;
	    message[2] = pos2;
	    message[3] = pos3;
	    message[4] = pos4;
	    message[5] = pos5;
	    message[6] = pos6;
	    message[7] = pos7;
	    message[8] = pos8;
	    message[9] = pos9;
	    //actually write the message
	    writeTcp(OHostName,OPortNum,message);
	    
	    //tell O they are O
	    memset(message,0,34);//reset the message
	    message[0] = 'S';
	    message[1] = 'O';
	   //actually write the message
	    writeTcp(OHostName,OPortNum,message);
	    /*
	    //alert O that it's their turn
	    memset(message,0,34);//reset the message
	    message[0] = 'T';
	      //actually write the message
	    writeTcp(OHostName,OPortNum,message);
	    */
	    isXTurn = 1;
	    gotMove = 0;//don't contact X until we've got a move back from O
	}
    }
    
  }
  
  exit(0);//to satisfy the compiler- we should never actually reach this code
  
}

void sendDatagram(char message[34], char socket_num_str[8],char hostName[16]){
  
  int socket_fd, cc, ecode;
  struct sockaddr_in *dest;
  struct addrinfo hints, *addrlist;  
  
  struct {
   // char   head;
    char   body[34];
  //  char   tail;
  } msgbuf;
  
  fprintf(stderr,"Sending socket number is: %s\n",socket_num_str);
  
  
  /*
   Use getaddrinfo to create a SOCK_DGRAM sockarddr_in set
   up for the host specified by hostName and port specified by socket_num_str
*/
    
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_NUMERICSERV; hints.ai_protocol = 0;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;

  ecode = getaddrinfo(hostName, socket_num_str, &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  dest = (struct sockaddr_in *) addrlist->ai_addr; // Will use in sendto().

/*
   Set up a datagram (UDP/IP) socket in the Internet domain.
   We will use it as the handle thru which we will send a
   single datagram. Note that, since this no message is ever
   addressed TO this socket, we do not bind an internet address
   to it. It will be assigned a (temporary) address when we send
   a message thru it.
*/

  socket_fd = socket (addrlist->ai_family, addrlist->ai_socktype, 0);
  if (socket_fd == -1) {
    perror ("send_udp:socket");
    exit (1);
  }

  //msgbuf.head = '<';
  strncpy(msgbuf.body,message,34);
  //msgbuf.tail = '>';

  cc = sendto(socket_fd,&msgbuf,sizeof(msgbuf),0,(struct sockaddr *) dest,
                  sizeof(struct sockaddr_in));
  if (cc < 0) {
    perror("send_udp:sendto");
    exit(1);
  }
  
  close(socket_fd);

}

char* recieveDatagram(char message[34], char socket_num_str[8],char hostName[16]){
  
    int	socket_fd, cc, ecode;
    socklen_t fsize;
    struct sockaddr_in	*s_in, from;
    struct addrinfo hints, *addrlist; 
  
    void printsin();
    
    
    struct {
	//char	head;
	char    body[34];
	//char	tail;
    } msg;
    
    fprintf(stderr,"Recieving socket number is: %s\n",socket_num_str);

/*
   In order to attach a name to the socket created above, first fill
   in the appropriate blanks in an inet socket address data structure
   called "s_in". We will use the port number passed in. The second step
   is to BIND the address to the socket. If the port is in use, the
   bind system call will fail detectably.
*/

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE; hints.ai_protocol = 0;
  //hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;
  

/*
 getaddrinfo() should return a single result, denoting
 a SOCK_DGRAM socket on any interface of this system at
 the specified port.
*/

  ecode = getaddrinfo(NULL, socket_num_str, &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  s_in = (struct sockaddr_in *) addrlist->ai_addr;

  printsin(s_in, "RECV_UDP", "Local socket is:"); fflush(stdout);

/*
   Create the socket to be used for datagram reception. Initially,
   it has no name in the internet (or any other) domain.
*/
  socket_fd = socket (addrlist->ai_family, addrlist->ai_socktype, 0);
  if (socket_fd < 0) {
    perror ("recv_udp:socket");
    exit (1);
  }

/*
   bind the specified port on the current host to the socket accessed through
   socket_fd. If port in use, bind() will fail and we die.
*/

  if (bind(socket_fd, (struct sockaddr *)s_in, sizeof(struct sockaddr_in)) < 0) {
    perror("recv_udp:bind");
    exit(1);
  }
  
  //int waitingForData = 1;

  for(;;) {
  //while (waitingForData){
    fsize = sizeof(from);
    cc = recvfrom(socket_fd, &msg, sizeof(msg), 0, (struct sockaddr *)&from, &fsize);
    if (cc < 0) perror("recv_udp:recvfrom");
    printsin( &from, "recv_udp: ", "Packet from:");
    printf("Got data ::%s\n",msg.body);
    fflush(stdout);
    //waitingForData = 0;
  }
  
  close(socket_fd);//i have no idea if this is necessary

  strncpy(message,msg.body,34);
  return message;
}

void printsin(struct sockaddr_in *sin, char *m1, char *m2 )
{
  char fromip[INET_ADDRSTRLEN];

  printf ("%s %s:\n", m1, m2);
  printf ("  family %d, addr %s, port %d\n", sin -> sin_family,
	    inet_ntop(AF_INET, &(sin->sin_addr.s_addr), fromip, sizeof(fromip)),
            ntohs((unsigned short)(sin -> sin_port)));
}

int sendTcp(char* hostName, char* portNum, char msg[34]){
  int sock, left, num, put, ecode;
  struct sockaddr_in *server;
  struct addrinfo hints, *addrlist;
  
  //char msg[] = { "false pearls before real swine" };

/*
   Want a sockaddr_in containing the ip address for the system
   specified in hostName and the port specified in portNum.
*/

  memset( &hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV; hints.ai_protocol = 0;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;

  ecode = getaddrinfo(NULL, NULL, &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  server = (struct sockaddr_in *) addrlist->ai_addr;

/*
   Create the socket.
*/
  if ( (sock = socket( addrlist->ai_family, addrlist->ai_socktype, 0 )) < 0 ) {
    perror("inet_wstream:socket");
    exit(1);
  }

/*
   Connect to data socket on the peer at the specified Internet
   address.
*/
  if ( connect(sock, (struct sockaddr *)server, sizeof(struct sockaddr_in)) < 0) {
    perror("inet_wstream:connect");
    exit(1);
  }

/*
   Write the quote and terminate. This will close the connection on
   this end, so eventually an "EOF" will be detected remotely.
*/
  left = sizeof(msg); put=0;
  while (left > 0){
    if((num = write(sock, msg+put, left)) < 0) {
      perror("inet_wstream:write");
      exit(1);
    }
    else left -= num;
    put += num;
  }
  exit(0);
}


int setupUdp(char socket_num_str[8]){
  
    int	socket_fd, ecode;
    struct sockaddr_in	*s_in;
    struct addrinfo hints, *addrlist; 
  
   // void printsin();
    
    //fprintf(stderr,"Recieving socket number is: %s\n",socket_num_str);

/*
   In order to attach a name to the socket created above, first fill
   in the appropriate blanks in an inet socket address data structure
   called "s_in". We will use the port number passed in. The second step
   is to BIND the address to the socket. If the port is in use, the
   bind system call will fail detectably.
*/

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE; hints.ai_protocol = 0;
  //hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;
  

/*
 getaddrinfo() should return a single result, denoting
 a SOCK_DGRAM socket on any interface of this system at
 the specified port.
*/

  ecode = getaddrinfo(NULL, socket_num_str, &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  s_in = (struct sockaddr_in *) addrlist->ai_addr;

  printsin(s_in, "RECV_UDP", "Local socket is:"); fflush(stdout);

/*
   Create the socket to be used for datagram reception. Initially,
   it has no name in the internet (or any other) domain.
*/
  socket_fd = socket (addrlist->ai_family, addrlist->ai_socktype, 0);
  if (socket_fd < 0) {
    perror ("recv_udp:socket");
    exit (1);
  }

/*
   bind the specified port on the current host to the socket accessed through
   socket_fd. If port in use, bind() will fail and we die.
*/

  if (bind(socket_fd, (struct sockaddr *)s_in, sizeof(struct sockaddr_in)) < 0) {
    perror("recv_udp:bind");
    exit(1);
  }

  return socket_fd;//return the filedescriptor for the udp socket

}

int setupTcpV2(void)
{
  int listener;  /* fd for socket on which we get connection requests */
  struct sockaddr_in *localaddr;
  int ecode;
  socklen_t length;
  struct addrinfo hints, *addrlist;


/* 
   Want to specify local server address of:
      addressing family: AF_INET
      ip address:        any interface on this system 
      port:              0 => system will pick free port
*/

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE; hints.ai_protocol = 0;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;

  ecode = getaddrinfo(NULL, "0", &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  localaddr = (struct sockaddr_in *) addrlist->ai_addr;
 
/*
   Create socket on which we will accept connections. This is NOT the
   same as the socket on which we pass data.
*/
  if ( (listener = socket( addrlist->ai_family, addrlist->ai_socktype, 0 )) < 0 ) {
    perror("inet_rstream:socket");
    exit(1);
  }


  if (bind(listener, (struct sockaddr *)localaddr, sizeof(struct sockaddr_in)) < 0) {
    perror("inet_rstream:bind");
    exit(1);
  }

/*
   Print out the port number assigned to this process by bind().
*/
  length = sizeof(struct sockaddr_in);
  if (getsockname(listener, (struct sockaddr *)localaddr, &length) < 0) {
    perror("inet_rstream:getsockname");
    exit(1);
  }
  printf("RSTREAM:: assigned port number %d\n", ntohs(localaddr->sin_port));
  
  tmpPortNum = ntohs(localaddr->sin_port);


  return listener;
}

int getTcpSendFd(char* hostName, char* portNum){
  
  int sock, ecode;
  struct sockaddr_in *server;
  struct addrinfo hints, *addrlist;



  //int sock, left, num, put, ecode;
  //struct sockaddr_in *server;
  //struct addrinfo hints, *addrlist;
/*
   Want a sockaddr_in containing the ip address for the system
   specified in hostname and the port specified in portnumber.
*/

  memset( &hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV; hints.ai_protocol = 0;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;

  ecode = getaddrinfo(hostName, portNum, &hints, &addrlist);
  //ecode = getaddrinfo(hostname, portnumber, &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  server = (struct sockaddr_in *) addrlist->ai_addr;

/*
   Create the socket.
*/
  if ( (sock = socket( addrlist->ai_family, addrlist->ai_socktype, 0 )) < 0 ) {
    perror("inet_wstream:socket");
    exit(1);
  }

/*
   Connect to data socket on the peer at the specified Internet
   address.
*/
  if ( connect(sock, (struct sockaddr *)server, sizeof(struct sockaddr_in)) < 0) {
    perror("inet_wstream:connect");
    exit(1);
  }

  return sock;//the fd we're sending through

}

int writeTcp(char serverHostName[16], char serverPortNum[6], char msg[34])

{
  int sock, left, num, put, ecode;
  struct sockaddr_in *server;
  struct addrinfo hints, *addrlist;

  //int sock, left, num, put, ecode;
  //struct sockaddr_in *server;
  //struct addrinfo hints, *addrlist;
/*
   Want a sockaddr_in containing the ip address for the system
   specified in hostname and the port specified in portnumber.
*/

  memset( &hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV; hints.ai_protocol = 0;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;

  ecode = getaddrinfo(serverHostName, serverPortNum, &hints, &addrlist);
  //ecode = getaddrinfo(hostname, portnumber, &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    //exit(1);
    return 1;
  }
  fprintf(stderr,"in writetcp, serverHostName is %s, serverPortNum is %s and msg is %s\n",serverHostName,serverPortNum,msg);
  server = (struct sockaddr_in *) addrlist->ai_addr;

/*
   Create the socket.
*/
  if ( (sock = socket( addrlist->ai_family, addrlist->ai_socktype, 0 )) < 0 ) {
    perror("inet_wstream:socket");
   // exit(1);
    return 1;
  }

/*
   Connect to data socket on the peer at the specified Internet
   address.
*/
  if ( connect(sock, (struct sockaddr *)server, sizeof(struct sockaddr_in)) < 0) {
    perror("inet_wstream:connect");
    //exit(1);
    return 1;
  }

  
  fprintf(stderr,"In writeTcp, msg is: %s and length is: %lu\n",msg, (sizeof(msg) / sizeof(msg[0])));
  
/*
   Write the quote and terminate. This will close the connection on
   this end, so eventually an "EOF" will be detected remotely.
*/
  //left = sizeof(msg); put=0;
  left = 34; put=0;
  while (left > 0){
    if((num = write(sock, msg+put, left)) < 0) {
      perror("inet_wstream:write");
      //exit(1);
      return 1;
    }
    else left -= num;
    put += num;
  }
  close(sock);
  
  return 0;
}

int didXWin(void){
    if ((pos1 == pos2) && (pos2 == pos3) && (pos3 == 'X')){
	return 1;
    }
    if ((pos1 == pos4) && (pos4 == pos7) && (pos7 == 'X')){
	return 1;
    }
    if ((pos4 == pos5) && (pos5 == pos6) && (pos6 == 'X')){
	return 1;
    }
    if ((pos7 == pos8) && (pos8 == pos9) && (pos9 == 'X')){
	return 1;
    }
    if ((pos2 == pos5) && (pos5 == pos8) && (pos8 == 'X')){
	return 1;
    }
    if ((pos3 == pos6) && (pos6 == pos9) && (pos9 == 'X')){
	return 1;
    }
    if ((pos1 == pos5) && (pos5 == pos9) && (pos9 == 'X')){
	return 1;
    }
    if ((pos3 == pos5) && (pos5 == pos7) && (pos7 == 'X')){
	return 1;
    }
    return 0;
}

int didOWin(void){
    if ((pos1 == pos2) && (pos2 == pos3) && (pos3 == 'O')){
	return 1;
    }
    if ((pos1 == pos4) && (pos4 == pos7) && (pos7 == 'O')){
	return 1;
    }
    if ((pos4 == pos5) && (pos5 == pos6) && (pos6 == 'O')){
	return 1;
    }
    if ((pos7 == pos8) && (pos8 == pos9) && (pos9 == 'O')){
	return 1;
    }
    if ((pos2 == pos5) && (pos5 == pos8) && (pos8 == 'O')){
	return 1;
    }
    if ((pos3 == pos6) && (pos6 == pos9) && (pos9 == 'O')){
	return 1;
    }
    if ((pos1 == pos5) && (pos5 == pos9) && (pos9 == 'O')){
	return 1;
    }
    if ((pos3 == pos5) && (pos5 == pos7) && (pos7 == 'O')){
	return 1;
    }
    return 0;
}

void resetBoard(void){
    pos1 = ' ';
    pos2 = ' ';
    pos3 = ' ';
    pos4 = ' ';
    pos5 = ' ';
    pos6 = ' ';
    pos7 = ' ';
    pos8 = ' ';
    pos9 = ' ';
}

int isDraw(void){
    if ((pos1 != ' ') && (pos2 != ' ') && (pos3 != ' ') && (pos4 != ' ') && (pos5 != ' ') && (pos6 != ' ') && (pos7 != ' ') && (pos8 != ' ') && (pos9 != ' ') && (didOWin() == 0) && (didXWin() == 0)) {
	return 1;
    }
    else{
	return 0;
    }
}

