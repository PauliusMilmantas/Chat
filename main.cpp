/*
#if defined(WIN32)
#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <Winsock.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>

#else
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif
*/


#pragma comment(lib, "Ws2_32.lib")


#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <Winsock.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <thread>
#include <atomic>
#include <stdlib.h>

using namespace std;

#define BUFFLEN 1024
#define MAXCLIENTS 32

void pos(short C, short R)
{
    COORD xy ;
    xy.X = C ;
    xy.Y = R ;
    SetConsoleCursorPosition(
    GetStdHandle(STD_OUTPUT_HANDLE), xy);
}
void cls( )
{
    pos(0,0);
    for(int j=0;j<100;j++)
    cout << string(100, ' ');
    pos(0,0);
}

//For listenForServerOutput thread [clientSide]
atomic<bool> threadStatus;

void listenForServerOutput(int socket, fd_set read_set) {

    int maxfd = 0;
    char buffer[BUFFLEN];

    while(threadStatus == true) {
        FD_ZERO(&read_set);

        FD_SET(socket, &read_set);
        if (socket > maxfd){
            maxfd = socket;
        }

        select(maxfd+1, &read_set, NULL , NULL, NULL);

        FD_ZERO(&read_set);
        FD_SET(socket, &read_set);
        if (FD_ISSET(socket, &read_set)){
                   memset(&buffer,0,BUFFLEN);
            int r_len = recv(socket,(char*) &buffer,BUFFLEN,0); //Pridejau (char*)

            if(r_len > 0) {
                cout << buffer << endl;
            }
        }
    }
}

int clientSide() {
    int s_socket;
    //int l_socket;
    struct sockaddr_in servaddr; // Serverio adreso struktûra
    fd_set read_set;

    int maxfd = 0;

    char recvbuffer[BUFFLEN];
    char sendbuffer[BUFFLEN];

    string ip_address;


    int iResult;

    WSADATA wsaData;   // if this doesn't work

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    if ((s_socket = socket(AF_INET, SOCK_STREAM,0))< 0){
        fprintf(stderr,"ERROR #2: cannot create socket.\n");
        exit(1);
    }
/*
    cout << "Enter client port: ";
    int c_port;
    cin >> c_port;

    if ((c_port < 1) || (c_port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }
*/
    cout << "Enter server port: ";
    int s_port;
    cin >> s_port;

    if ((s_port < 1) || (s_port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }

    cls();

   /*
    * Iðvaloma ir uþpildoma serverio struktûra
    */

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET; // nurodomas protokolas (IP)
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(s_port); // nurodomas portas

    unsigned long ulAddr = INADDR_NONE;
    ulAddr = inet_addr("127.0.0.1");

    if (ulAddr == INADDR_NONE ) {
        printf("inet_addr failed and returned INADDR_NONE\n");
        WSACleanup();
        return 1;
    }

    if (ulAddr == INADDR_ANY) {
        printf("inet_addr failed and returned INADDR_ANY\n");
        WSACleanup();
        return 1;
    }

    if(connect(s_socket,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #4: error in connect().\n");
        exit(1);
    }

    /*if ((l_socket = socket(AF_INET, SOCK_STREAM,0)) < 0){
        fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
        return -1;
    }*/

    /*
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(c_port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"\nERROR #3: bind listening socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) <0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        return -1;
    }
    */




    //Klausomasi serverio nurodymu
    threadStatus = true;

    thread listeningForServer(listenForServerOutput, s_socket, read_set);

    string command = "";
    bool done = false;
    while(!done) {
        getline(cin, command);

        if(command != "/leave" && command != "/quit" && command != "/q") {
            char *cstr = new char[command.length() + 1];
            strcpy(cstr, command.c_str());
            int s_len = send(s_socket, cstr, strlen(cstr), 0);
            delete [] cstr;

            if(s_len == -1) {
                cout << "Server is unreachable!" << endl;
                threadStatus = false;
                listeningForServer.join();
                done = true;
            }
        } else {
            done = true;
            threadStatus = false;
            listeningForServer.join();
        }
    }





















    closesocket(s_socket);








/*
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"\nERROR #3: bind listening socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) <0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        return -1;
    }


    atomic<bool> isOnline(true);

    thread listenForInput([&isOnline, &l_socket] {
        while(true) {
            fd_set read_set;
            char buffer[BUFFLEN];

             select(NULL, &read_set, NULL , NULL, NULL);

            if(FD_ISSET(l_socket, &read_set)) {
                int r_len = recv(l_socket,(char*) &buffer,BUFFLEN,0);









            }
        }
    });


*/
    return 0;
}

int findemptyuser(int c_sockets[]){
    int i;
    for (i = 0; i <  MAXCLIENTS; i++){
        if (c_sockets[i] == -1){
            return i;
        }
    }
    return -1;
}

int serverSide() {

    struct Client {
        string name;
        string ip;
        int socket;
    };

    cls();

    cout << "Enter server port: ";
    int port;
    cin >> port;

    if ((port < 1) || (port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }

    cls();
    cout << "Starting server...";

    //unsigned int port = SERVER_PORT;
    unsigned int clientaddrlen;
    int l_socket;
    //int c_sockets[MAXCLIENTS];
    Client clients[MAXCLIENTS];
    fd_set read_set;

    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;

    int maxfd = 0;
    int i;

    char buffer[BUFFLEN];

    int iResult;

    WSADATA wsaData;   // if this doesn't work

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    if ((l_socket = socket(AF_INET, SOCK_STREAM,0)) < 0){
        fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
        return -1;
    }

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"\nERROR #3: bind listening socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) <0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        return -1;
    }

    for (i = 0; i < MAXCLIENTS; i++){
        clients[i].socket = -1;
    }

    cout << "\nWaiting for connections..." << endl;

    int sockets[MAXCLIENTS];
    for(int a = 0; a < MAXCLIENTS; a++) {
        clients[a].socket = -1;
    }

    for (;;){
        FD_ZERO(&read_set);
        for (i = 0; i < MAXCLIENTS; i++){
            if (clients[i].socket != -1){
                FD_SET(clients[i].socket, &read_set);
                if (clients[i].socket > maxfd){
                    maxfd = clients[i].socket;
                }
            }
        }

        FD_SET(l_socket, &read_set);
        if (l_socket > maxfd){
            maxfd = l_socket;
        }

        select(maxfd+1, &read_set, NULL , NULL, NULL);

        if (FD_ISSET(l_socket, &read_set)){

            for(int a = 0; a < MAXCLIENTS; a++) {
                sockets[a] = clients[a].socket;
            }


            int client_id = findemptyuser(sockets);
            if (client_id != -1){
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                clients[client_id].socket = accept(l_socket,
                    (struct sockaddr*)&clientaddr, (int*)&clientaddrlen);
                clients[client_id].ip = inet_ntoa(clientaddr.sin_addr);
                clients[client_id].name = inet_ntoa(clientaddr.sin_addr);
                printf("Connected:  %s\n",inet_ntoa(clientaddr.sin_addr));
            }
        }


        for (i = 0; i < MAXCLIENTS; i++){
            if (clients[i].socket != -1){
                if (FD_ISSET(clients[i].socket, &read_set)){
                    memset(&buffer,0,BUFFLEN);
                    int r_len = recv(clients[i].socket,(char*) &buffer,BUFFLEN,0); //Pridejau (char*)

                    if(r_len > 0) {
                       cout << clients[i].name << ": " << buffer << endl;
                    }




                    /*
                    int j;
                    for (j = 0; j < MAXCLIENTS; j++){
                        if (c_sockets[j] != -1){
                            int w_len = send(c_sockets[j], buffer, r_len,0);
                            if (w_len <= 0){
                                printf("Someone left the room.");


                                closesocket(c_sockets[j]);
                                c_sockets[j] = -1;
                            }
                        }
                    }
                    */
                }
            }
        }


        /*
        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                if (FD_ISSET(c_sockets[i], &read_set)){
                    memset(&buffer,0,BUFFLEN);
                    int r_len = recv(c_sockets[i],(char*) &buffer,BUFFLEN,0); //Pridejau (char*)

                    int j;
                    for (j = 0; j < MAXCLIENTS; j++){
                        if (c_sockets[j] != -1){
                            int w_len = send(c_sockets[j], buffer, r_len,0);
                            if (w_len <= 0){
                                printf("Someone left the room.");


                                closesocket(c_sockets[j]);
                                c_sockets[j] = -1;
                            }
                        }
                    }
                }
            }
        }
        */
    }





    return 0;
}

void showMenu() {
    cls();
    cout << "--------------------\n";
    cout << "[1]. Enter chat\n";
    cout << "[2]. Host chat\n";
    cout << "[3]. Configurations\n";
    cout << "[4]. Leave\n";
    cout << "--------------------\n";
}

int main()
{
    showMenu();

    int choice;
    cin >> choice;

    switch(choice) {
    case 1:
        clientSide();
        break;
    case 2:
        serverSide();
        break;
    case 3:
        break;
    default:
        return 0;
    }

    //main();
}
