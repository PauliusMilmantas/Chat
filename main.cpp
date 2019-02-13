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
#define PORT 69
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

int clientSide() {


    unsigned int port = PORT;
    int s_socket;
    struct sockaddr_in servaddr; // Serverio adreso struktûra
    fd_set read_set;

    char recvbuffer[BUFFLEN];
    char sendbuffer[BUFFLEN];

    string ip_address;
    int iResult;

    WSADATA wsaData;   // if this doesn't work
    //WSAData wsaData; // then try this instead

    // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:



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
    * Iðvaloma ir uþpildoma serverio struktûra
    */

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET; // nurodomas protokolas (IP)
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(port); // nurodomas portas


/*
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

*/

    if (connect(s_socket,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #4: error in connect().\n");
        exit(1);
    }

    WSACleanup();

    string command = "";
    bool done = false;
    while(!done) {
        cin >> command;

        if(command != "/leave" && command != "/quit" && command != "/q") {
            char *cstr = new char[command.length() + 1];
            strcpy(cstr, command.c_str());
            send(s_socket, cstr, strlen(cstr), 0);
            delete [] cstr;
        } else {
            done = true;
        }
    }

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
    cout << "Starting server...";

    unsigned int port = PORT;
    unsigned int clientaddrlen;
    int l_socket;
    int c_sockets[MAXCLIENTS];
    fd_set read_set;

    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;

    int maxfd = 0;
    int i;

    char buffer[BUFFLEN];

    int iResult;

    WSADATA wsaData;   // if this doesn't work
    //WSAData wsaData; // then try this instead

    // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:



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
        c_sockets[i] = -1;
    }

    cout << "\nWaiting for connections...";




/*
    //Nera jokiu komandu ivesta dar
    atomic<bool> hasCommand(false);
    atomic<string> command;

    thread listenForInput([&hasCommand, &command] {
        string tmp;
        cin >> tmp;
        hasCommand = true;
    });
*/





    for (;;){
        FD_ZERO(&read_set);
        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                FD_SET(c_sockets[i], &read_set);
                if (c_sockets[i] > maxfd){
                    maxfd = c_sockets[i];
                }
            }
        }

        FD_SET(l_socket, &read_set);
        if (l_socket > maxfd){
            maxfd = l_socket;
        }

        select(maxfd+1, &read_set, NULL , NULL, NULL);

        if (FD_ISSET(l_socket, &read_set)){
            int client_id = findemptyuser(c_sockets);
            if (client_id != -1){
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                c_sockets[client_id] = accept(l_socket,
                    (struct sockaddr*)&clientaddr, (int*)&clientaddrlen);  //PRIDEJAU (int*)
                printf("Connected:  %s\n",inet_ntoa(clientaddr.sin_addr));
            }
        }

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

    main();
}
