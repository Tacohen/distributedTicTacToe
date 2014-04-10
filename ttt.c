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

int timeoutRequested = 0;
long int timeoutLength = 60;

int tcpSocket;

void printBoard();

char pos1 = ' ';
char pos2 = ' ';
char pos3 = ' ';
char pos4 = ' ';
char pos5 = ' ';
char pos6 = ' ';
char pos7 = ' ';
char pos8 = ' ';
char pos9 = ' ';

int sendingPortNum;
int sendingUdpPortNum;

void sendDatagram(char message[34],char socket_num_str[8],char hostName[16]);

char* recieveDatagram(char message[34], char socket_num_str[8],char hostName[16]);

void printsin(struct sockaddr_in*, char*, char*);

int getTcpSendFd(char* hostName, char* portNum);

int setupTcpV2();
int setupTcpV4();
void setupTcpV3(char* hostname, char* portnumber);

int setupUdpOrig(char socket_num_str[8]);
int setupUdpV2();

void writeTcp(char serverHostName[16],char serverPortNum[6], char message[34]);

int main(int argc, char *argv[]){
  
  
  int queryMode = 0;
  char *timeoutInputStr;
  char message[34];
 
  
  if (argc < 2){
	  //fprintf(stderr,"Query mode false, no timeout \n");
	}
	
  if (argc == 2){
    if ((strcmp(argv[1],"-q")==0)) {
	  queryMode = 1;
	  //fprintf(stderr,"Query mode true, no timeout\n");
    }
   else{
	fprintf(stderr,"Invalid input, exiting!\n");
	exit(1);
   }
  }
  
  if (argc == 3){
     if ((strcmp(argv[1],"-t"))) {
	  fprintf(stderr,"Invalid input, exiting!\n");
	  exit(1);
    }
    timeoutRequested = 1;
    timeoutLength = strtol(argv[2],&timeoutInputStr,10);
    if (strlen(timeoutInputStr)){
	  fprintf(stderr,"Timeout must be an integer, exiting!\n");
	  exit(1);
    }
    if (timeoutLength < 1){
	  fprintf(stderr,"Timeout must be a positive integer, exiting!\n");
	  exit(1);
    }
  }
  
  if (argc > 3){
      fprintf(stderr,"Too many parameters, exiting!\n");
      exit(1);
  }
  char send_socket_num_str[6];
  send_socket_num_str[6] = (char)'\0';
  char recieve_socket_num_str[6];
  recieve_socket_num_str[6] = (char)'\0';
  char hostName[16];
  
  char thisHostName[16];
  thisHostName[15] = '\0';
  gethostname(thisHostName, 15);//get the name of the computer we're running on
  struct hostent* h;
  h = gethostbyname(thisHostName);
  strncpy(thisHostName,h->h_name,16);
  //fprintf(stderr,"This host name is: %s\n",thisHostName);
  
  char serverHostName[16];
  int tcpServerPort;
  char tcpServerPortStr[8];
  char response[34];
  char handle[33];
  char clientHostName[16];
  
  int notAlertedToTurn = 1;
  int notGotSymbol = 1;
  int notShownBoard = 1;
  int notPromptedForMove = 1;
  
  
  FILE *fp;
  fp = fopen("socket","r");  
  if (fp == NULL){
      printf("Could not open file, waiting and attempting to retrieve it!\n");
      sleep(60);
      fp = fopen("socket","r");  
      if (fp == NULL){
	    printf("Could not open file, exiting!\n");
	    exit(1);
      }
  }
  fscanf(fp,"%s",send_socket_num_str);
  fscanf(fp,"%s",recieve_socket_num_str);
  fscanf(fp,"%s",hostName);
  
  fclose(fp);
  
  //fprintf(stderr,"recieve_socket_num_str is: %s\n",recieve_socket_num_str);
  //fprintf(stderr,"thisHostName is: %s\n",thisHostName);
  
  //int datagramFdOrig = setupUdpOrig(recieve_socket_num_str);
  int datagramFd = setupUdpV2();
  //fprintf(stderr,"Setup udp!\n");
  int tcpRecieveFd = setupTcpV2();
  listen(tcpRecieveFd,1);
  
  struct sockaddr_in tmpAddr;
  socklen_t tmpLength = (socklen_t)sizeof(struct sockaddr_in);
  getsockname(datagramFd,(struct sockaddr *)&tmpAddr,&tmpLength);
  sendingUdpPortNum = ntohs((unsigned short)(tmpAddr.sin_port));
  
  fd_set mask;
  struct timeval timeout;
  struct sockaddr_in from,peer;
  int hits;
  char ch;
  
  memset(response,0,34);//reset the message
  
  if (queryMode){
      response[0] = 'Q';
      sendDatagram(response,send_socket_num_str,hostName);
      recieveDatagram(response, recieve_socket_num_str,serverHostName);
      if (response[0] == 'I'){
	  printf("No game in progress\n");
	  exit(0);
      }
      else{
	printf("Game in progress. Board is: \n");
	pos1 = response[1];
	    pos2 = response[2];
	    pos3 = response[3];
	    pos4 = response[4];
	    pos5 = response[5];
	    pos6 = response[6];
	    pos7 = response[7];
	    pos8 = response[8];
	    pos9 = response[9];
	    printBoard();
	    memset(response,0,34);//reset the message
      	recieveDatagram(response, recieve_socket_num_str,serverHostName);
	printf("Players are: \n");
	int j;
	for (j=1; j < 34; j++){
	    printf("%c",response[j]);
	}
	printf("\n");
	    memset(response,0,34);//reset the message
      	recieveDatagram(response, recieve_socket_num_str,serverHostName);
	//printf("Players are: \n");
	for (j=1; j < 34; j++){
	    printf("%c",response[j]);
	}
	printf("\n");
	exit(0);
      }
  }
  
  
	  
  char portNumStr[8];

  response[0] = 'P';
  char sendingPortStr[5];
  sprintf(sendingPortStr, "%d", sendingUdpPortNum);

  //response[1] = recieve_socket_num_str[0];
  //response[2] = recieve_socket_num_str[1];
  //response[3] = recieve_socket_num_str[2];
  //response[4] = recieve_socket_num_str[3];
  //response[5] = recieve_socket_num_str[4];
  response[1] = sendingPortStr[0];
  response[2] = sendingPortStr[1];
  response[3] = sendingPortStr[2];
  response[4] = sendingPortStr[3];
  response[5] = sendingPortStr[4];
  response[6] = thisHostName[0];
  response[7] = thisHostName[1];
  response[8] = thisHostName[2];
  response[9] = thisHostName[3];
  response[10] = thisHostName[4];
  response[11] = thisHostName[5];
  response[12] = thisHostName[6];
  response[13] = thisHostName[7];
  response[14] = thisHostName[8];
  response[15] = thisHostName[9];
  response[16] = thisHostName[10];
  response[17] = thisHostName[11];
  response[18] = thisHostName[12];
  response[19] = thisHostName[13];
  response[20] = thisHostName[14];
  
  
  //fprintf(stderr,"datagram sent to server is: %s\n",response);
  
  //strncpy(message,"xyz",34);
  
  //send server the client's host name
  sendDatagram(response,send_socket_num_str,hostName);  
  
  memset(message,0,34);
  memset(response,0,34);
  
  int firstTimeGotMessage = 1;
  int timeoutFlagFirstTimeThrough = 2;
  
   while (1){//This loop should never terminate  

    //Yes, we need to reset all these values every time we loop through- otherwise select gets screwed up
    FD_ZERO(&mask);
    FD_SET(datagramFd,&mask);
    FD_SET(tcpRecieveFd,&mask);
    memset(response,0,34);
    
    if (timeoutRequested){
	timeout.tv_sec = timeoutLength;
        timeout.tv_usec = 0;
	
	if(timeoutFlagFirstTimeThrough){
	    timeoutFlagFirstTimeThrough--;
	}
	else{
	    printf("Connection to server timed out!\n");
	    exit(0);
	}
    }
    else{
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
    }
    if ((hits = select(20, &mask, (fd_set *)0, (fd_set *)0,
                           &timeout)) < 0) {
      perror("fancy_recv_udp:select");
      exit(1);
    }
    if ( ((hits>0) && (FD_ISSET(datagramFd,&mask))) ) {
      //fprintf(stderr,"Datagram received!\n");
      
      socklen_t fsize = sizeof(from);//this is basically an int- but GCC whines if you actually declare it an int
      
      int cc = recvfrom(datagramFd,&response,sizeof(response),0,(struct sockaddr *)&from,&fsize);
      if (cc < 0) perror("recv_udp:recvfrom");
      //printsin( &from, "recv_udp: ", "Packet from:");
      fflush(stdout);
      //fprintf(stderr,"Received message back from server! Message is: %s\n",response);
      
      if ('Y' == (char)response[0]){
	  //do nothing, this was just a test from the server to see if we are still active
	  
      }
      
      if ('N' == (char)response[0]){
      printf("Server has too many queued players, please try again later\n");
      exit(0);
      }
      /*
      if ('A' == (char)response[0]){
	printf("Server accepted the connection! Waiting for other player\n");
	//sleep(2);
      }*/
      if ('W' == (char)response[0]){
	printf("Game is in progress. You have been queued. Please be patient\n");
      }
      
      if ('P' == (char)response[0]){
	printf("Server accepted the connection! Waiting for other player\n");
	tcpServerPortStr[0] = response[1];
	tcpServerPortStr[1] = response[2];
	tcpServerPortStr[2] = response[3];
	tcpServerPortStr[3] = response[4];
	tcpServerPortStr[4] = response[5];
	serverHostName[0] = response[6];
	serverHostName[1] = response[7];
	serverHostName[2] = response[8];
	serverHostName[3] = response[9];
	serverHostName[4] = response[10];
	serverHostName[5] = response[11];
	serverHostName[6] = response[12];
	serverHostName[7] = response[13];
	serverHostName[8] = response[14];
	serverHostName[9] = response[15];
	serverHostName[10] = response[16];
	serverHostName[11] = response[17];
	serverHostName[12] = response[18];
	serverHostName[13] = response[19];
	serverHostName[14] = response[20];
	tcpServerPort = strtol(tcpServerPortStr,NULL,10);//convert to int
	memset(&tcpServerPortStr,0,8);
	sprintf(tcpServerPortStr,"%d",tcpServerPort);//Convert port number to a string, stripping out the padding zeros
	
      }
      
      
      if ('A' == (char)response[0]){
	printf("Enter your handle (up to 31 ASCII characters): \n");
	int needInput = 1;


	memset(message,0,34);
	message[0] = 'H';
	int handleCount = 1;
	int c = getchar();
	while((c != EOF) && (c != '\n') && (c != ' ')){
	    //fprintf(stderr,"c is: %c\n",c);
	    message[handleCount] = c;
	    c = getchar();
	    handleCount++;
	    if (timeoutRequested){
	      sleep(timeoutLength);
	      printf("Server timed out!\n");
	      exit(1);
	    }
	}
	
	
	//sleep(3);
	writeTcp(hostName,tcpServerPortStr,message);
	
	//send our tcp port to the server!
	//fprintf(stderr,"Preparing to write the first tcp message!\n");
	  memset(response,0,34);//reset the message	  
	  portNumStr[5];
	  sprintf(portNumStr,"%d",sendingPortNum);//Convert port number to a string
	  
	  response[0] = 'P';
	  response[1] = portNumStr[0];
	  response[2] = portNumStr[1];
	  response[3] = portNumStr[2];
	  response[4] = portNumStr[3];
	  response[5] = portNumStr[4];
	  response[6] = thisHostName[0];
	  response[7] = thisHostName[1];
	  response[8] = thisHostName[2];
	  response[9] = thisHostName[3];
	  response[10] = thisHostName[4];
	  response[11] = thisHostName[5];
	  response[12] = thisHostName[6];
	  response[13] = thisHostName[7];
	  response[14] = thisHostName[8];
	  response[15] = thisHostName[9];
	  response[16] = thisHostName[10];
	  response[17] = thisHostName[11];
	  response[18] = thisHostName[12];
	  response[19] = thisHostName[13];
	  response[20] = thisHostName[14];
	  
	//fprintf(stderr,"calling writeTCP with parameters %s,%s,%s\n",  hostName,tcpServerPortStr,response);
	writeTcp(hostName,tcpServerPortStr,response);
	
      }
  
    }
    if ( ((hits>0) && (FD_ISSET(tcpRecieveFd,&mask))) ) {
	//fprintf(stderr,"TCP message receieved!\n");
	memset(message,0,34);//reset the message
      
      socklen_t length = sizeof(socklen_t);//GCC wants this to be declared this way, even though it's basically an int
      int conn;
      if ((conn=accept(tcpRecieveFd, (struct sockaddr *)&peer, &length)) < 0) {	
	perror("inet_rstream:accept");
	exit(1);
      }

      int i;
      for (i = 0;i < 34; i++){
	read(conn, &ch, 1);
        //fprintf(stderr,"%c",ch); 
	message[i] = ch;
      }
      //fprintf(stderr,"Finished reading message! Message is: %s\n",message);
      
      if (message[0] == 'R'){
	  if (message[1] == 'W'){
	      printf("You Won! Congratulations! Exiting Now!\n");
	      exit(0);
	  }
	  else if (message[1] == 'L'){
	      printf("You Lost. Exiting Now!\n");
	      exit(0);
	  }
	  else{
	      printf("You played to a draw. Exiting Now!\n");
	      exit(0);
	  }
      }
      

      if ((message[0] == 'B')  && notShownBoard){
	    printf("Here is the Board:!\n");
	    pos1 = message[1];
	    pos2 = message[2];
	    pos3 = message[3];
	    pos4 = message[4];
	    pos5 = message[5];
	    pos6 = message[6];
	    pos7 = message[7];
	    pos8 = message[8];
	    pos9 = message[9];
	    printBoard();
	    notShownBoard = 0;
      }
      
      if ((message[0] == 'S') && notGotSymbol){
	printf("You are %c\n",message[1]);
	notGotSymbol = 0;
      }
      

      if ((message[0] == 'T') && notAlertedToTurn){
	    printf("It's your turn!\n");
	    notAlertedToTurn = 0;
	    notGotSymbol = 1;
          notShownBoard = 1;
	  notPromptedForMove = 1;
	  
	
      }
      //remember to reset these flags after we make a move!
      ////notAlertedToTurn
      ////notGotSymbol
      ////notShownBoard
      
      int move;
      
      //when we have gotten all necessary info from the server
      if ((notAlertedToTurn == 0) && (notGotSymbol == 0) && (notShownBoard == 0) && notPromptedForMove){
	  printf("Enter your move: \n");
	  int c = getchar();
	  notPromptedForMove = 0;
	  //fprintf(stderr,"User input was %c\n",c);
	  memset(response,0,34);//reset the message
	  response[0] = 'M';
	  response[1] = c;
	  writeTcp(hostName,tcpServerPortStr,response);
	  //reset flags
	  notAlertedToTurn = 1;
          //notGotSymbol = 1;
          //notShownBoard = 1;
	  //notPromptedForMove = 1;
	  
	  printf("Move submitted to server\n");
	  
	  //We have to do this, otherwise select is screwed up
	  close(conn);
	  FD_ZERO(&mask);
	  FD_SET(tcpRecieveFd,&mask);
	  continue;
      }
      
      //We have to do this, otherwise select is screwed up
      close(conn);
      FD_ZERO(&mask);
      FD_SET(tcpRecieveFd,&mask);
      continue;
    }
  }
  
  exit(1);///to keep the compiler happy
  
}

void printBoard(void){
      printf("Board layout is: \n");
      printf("%c | %c | %c \n",pos1,pos2,pos3);
      printf("%c | %c | %c \n",pos4,pos5,pos6);
      printf("%c | %c | %c \n",pos7,pos8,pos9);
}

void sendDatagram(char message[34], char socket_num_str[8],char hostName[16]){
  
  int socket_fd, cc, ecode;
  struct sockaddr_in *dest;
  struct addrinfo hints, *addrlist;  
  
  struct {
    //char   head;
    char   body[34];
    //char   tail;
  } msgbuf;
  
  //fprintf(stderr,"Sending socket number is: %s\n",socket_num_str);
  
  
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
  
    //void printsin();
    
    
    struct {
	//char	head;
	char    body[34];
	//char	tail;
    } msg;
    
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
  hints.ai_canonname = hostName; hints.ai_addr = NULL;
  hints.ai_next = NULL;
  

/*
 getaddrinfo() should return a single result, denoting
 a SOCK_DGRAM socket on any interface of this system at
 the specified port.
*/

  ecode = getaddrinfo(NULL, socket_num_str, &hints, &addrlist);
  if (ecode != 0) {
    //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  s_in = (struct sockaddr_in *) addrlist->ai_addr;

  //printsin(s_in, "RECV_UDP", "Local socket is:"); fflush(stdout);

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
  
  fd_set          input_set;
  struct timeval  timeout;
  
  int waitingForData = 1;
  
   /* Empty the FD Set */
    FD_ZERO(&input_set );
    /* Listen to the input descriptor */
    FD_SET(socket_fd, &input_set);

    /* Waiting for some seconds */
    timeout.tv_sec = 120;    // 120 seconds
    timeout.tv_usec = 0;    // 0 milliseconds
    if (timeoutRequested){
      timeout.tv_sec = timeoutLength;
    }
    
    
    /*
    while (1){
      waitingForData = select(1, &input_set, NULL, NULL, &timeout);
      if (waitingForData){
	break;
      }
      else{
	 printf("Server timed out! Exiting...\n");
	exit(1);
    }
    }
  //for(;;) {
  */
  while (waitingForData){

  //if (waitingForData){
    fsize = sizeof(from);
    cc = recvfrom(socket_fd, &msg, sizeof(msg), 0, (struct sockaddr *)&from, &fsize);
    if (cc < 0) perror("recv_udp:recvfrom");
   // printsin( &from, "recv_udp: ", "Packet from:");
    //printf("Got data ::%s\n",msg.body);
    fflush(stdout);
    waitingForData = 0;
  }
  //else{
  //    printf("Server timed out! Exiting...\n");
  //    exit(1);
  //}

  close(socket_fd);//we have to do this otherwise we can't accept any more datagrams (which would be almost as terrible as stealing 40 cakes!)
  
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

void setupTcp(char* hostName, char* portNum){
  int sock, left, num, put, ecode;
  struct sockaddr_in *server;
  struct addrinfo hints, *addrlist;
  
/*
   Want a sockaddr_in containing the ip address for the system
   specified in hostName and the port specified in portNum.
*/

  memset( &hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV; hints.ai_protocol = 0;
  hints.ai_canonname = NULL; hints.ai_addr = NULL;
  hints.ai_next = NULL;

  ecode = getaddrinfo(NULL, portNum, &hints, &addrlist);
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

  
  tcpSocket = sock;
}


//int readTcp(char* hostName, char* portNum){
int getTcpSendFd(char* hostName, char* portNum){
  
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
  
  sendingPortNum = ntohs(localaddr->sin_port);
  
  return listener;
}

void setupTcpV3(char* hostname, char* portnumber){
  int listener;  /* fd for socket on which we get connection requests */
  int conn;      /* fd for socket thru which we pass data */
  int sock, left, num, put, ecode;
  struct sockaddr_in *server, localaddr,peer;
  struct addrinfo hints, *addrlist;
  socklen_t length;


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

  //ecode = getaddrinfo(argv[1], argv[2], &hints, &addrlist);
  ecode = getaddrinfo(hostname, portnumber, &hints, &addrlist);
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
  
    sendingPortNum = ntohs(server->sin_port);

  
}

int setupTcpV4(void)
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
  
  sendingPortNum = ntohs(localaddr->sin_port);


  return listener;
}


int setupUdpV2(){
  
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

  ecode = getaddrinfo(NULL, "0", &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  s_in = (struct sockaddr_in *) addrlist->ai_addr;

  //printsin(s_in, "RECV_UDP", "Local socket is:"); fflush(stdout);

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

  //sendingUdpPortNum = ntohl((unsigned short)(s_in -> sin_port));
  
  return socket_fd;//return the filedescriptor for the udp socket

}


void writeTcp(char serverHostName[16], char serverPortNum[6], char msg[34])

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

  //fprintf(stderr,"In writeTcp, msg is: %s and length is: %lu\n",msg, (sizeof(msg) / sizeof(msg[0])));
  
/*
   Write the quote and terminate. This will close the connection on
   this end, so eventually an "EOF" will be detected remotely.
*/
  //left = sizeof(msg); put=0;
  left = 34; put=0;
  while (left > 0){
    if((num = write(sock, msg+put, left)) < 0) {
      perror("inet_wstream:write");
      exit(1);
    }
    else left -= num;
    put += num;
  }
  close(sock);
}

int setupUdpOrig(char sendingPort[6]){
  
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

  ecode = getaddrinfo(NULL, sendingPort, &hints, &addrlist);
  if (ecode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
  }

  s_in = (struct sockaddr_in *) addrlist->ai_addr;

  //printsin(s_in, "RECV_UDP", "Local socket is:"); fflush(stdout);

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





