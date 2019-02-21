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

void errorSwitch(int error) {
    if(error != 0) {
        switch(error) {
            case 10091:
                cout << "Network subsystem is unavailable" << endl;
                break;
            case 10092:
                cout << "Winsock.dll version out of range" << endl;
                break;
            case 10050:
                cout << "Network is down." << endl;
                break;
            case 10051:
                cout << "Network is unreachable" << endl;
                break;
            case 10052:
                cout << "Network dropped connection on reset." << endl;
                break;
            case 10093:
                cout << "Successful WSAStartup not yet performed." << endl;
                break;
            case 10036:
                cout << "Operation now in progress" << endl;
                break;
            case 10067:
                cout << "Too many processes" << endl;
                break;
            case 10014:
                cout << "Bad address" << endl;
                break;
            case 10047:
                cout << "Address family not supported by protocol family." << endl;
                break;
            case 10061:
                cout << "Connection refused." << endl;
                break;
            default:
                cout << "Error occured: " << error << endl;
                break;
        }
    }
}

//For listenForServerOutput thread [clientSide]
atomic<bool> threadStatus;

void listenForServerOutput(int socket, fd_set read_set) {

    int maxfd = 0;
    char buffer[BUFFLEN];
    struct timeval tv;      //Neveikia timeout nustatymas

    while(threadStatus == true) {
		//Nustatoma 0 bitu visiems FD
        FD_ZERO(&read_set);

		//Nustatomi socket bitai
        FD_SET(socket, &read_set);
        if (socket > maxfd){
            maxfd = socket;
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;

		//maxfd+1 Kiek fd(File descriptor) turi buti istestuota.
		//read_set output'as select'o
        int status = select(maxfd+1, &read_set, NULL , NULL, &tv);

        //Tikrinama ar select nebuvo klaidos
        if(status == 0) { //Baigesi laiko limitas
            cout << "A signal interrupted the call or the time limit expired. " << endl;
        } else if(status == -1) {
            switch(errno) {
            case 'EBADF':
                    cout << "One or more of the file descriptor sets specified a file descriptor that is not a valid open file descriptor or specified a file descriptor that does not support selection. " << endl;
                break;
            case 'EISDIR':
                cout << "One or more the file descriptor sets specified a file descriptor that refers to an open directory." << endl;
                break;
            case 'EINVAL':
                cout << "A component of timeout is outside the acceptable range." << endl;
                break;
            default:
                cout << "Unknown select error in client side." << endl;
                break;
            }
        }

        FD_ZERO(&read_set);
        FD_SET(socket, &read_set);

        //Returns a non-zero value if the bit for the file descriptor (socket) is set in the file descriptor set pointed to by read_set, and 0 otherwise.
        if (FD_ISSET(socket, &read_set)){
            memset(&buffer,0,BUFFLEN);
            int r_len = recv(socket,(char*) &buffer,BUFFLEN,0);

            //Tikrinama ar ivyko klaida
            if(r_len == 0 || r_len > 0) {
                //Jeigu ivyko klaida gaunant
                int klaida = WSAGetLastError();

                errorSwitch(klaida);
                threadStatus = false;
            }

            if(r_len > 0) {
                cout << buffer << endl;
            }
        }
    }
}

int clientSide() {
    int s_socket;
    struct sockaddr_in servaddr; // Serverio adreso struktûra
    fd_set read_set;

    int maxfd = 0;

    char recvbuffer[BUFFLEN];
    char sendbuffer[BUFFLEN];

    string ip_address;

    int iResult;

    //swaData laiko informacija apie windows socketu implementacija
    WSADATA wsaData;   // if this doesn't work

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        errorSwitch(iResult);

        return 1;
    }

    if ((s_socket = socket(AF_INET, SOCK_STREAM,0))< 0){
        fprintf(stderr,"ERROR #2: cannot create socket.\n");
        exit(1);
    }

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

    int error = connect(s_socket,(struct sockaddr*)&servaddr,sizeof(servaddr));
    if(error<0){
        errorSwitch(error);
        exit(1);
    }

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

    unsigned int clientaddrlen;
    int l_socket;
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

    string outputf;
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

                outputf = "Connected: " + string(inet_ntoa(clientaddr.sin_addr));
                strcpy(buffer, outputf.c_str());
                for(int b = 0; b < MAXCLIENTS; b++) {
                    if(clients[b].socket != -1) {
                        send(clients[b].socket, buffer, BUFFLEN, 0);
                    }
                }
            }
        }



        for (i = 0; i < MAXCLIENTS; i++){
            if (clients[i].socket != -1){
                if (FD_ISSET(clients[i].socket, &read_set)){
                    memset(&buffer,0,BUFFLEN);
                    int r_len = recv(clients[i].socket,(char*) &buffer,BUFFLEN,0);

                    if(r_len > 0) {//==========================================================
                        char buffer2[5];

                        for(int p = 0; p < 5; p++) {
                            buffer2[p] = buffer[p];
                        }

                        if(buffer2 == "/name") {
                            cout << "Changing name..." << endl;
                        } else {
                            outputf = clients[i].name + ": " + buffer;
                            cout << outputf << endl;
                            strcpy(buffer, outputf.c_str());

                            for(int a = 0; a < MAXCLIENTS; a++) {
                                if(clients[a].socket != -1 && a != i) {
                                    send(clients[a].socket, buffer, BUFFLEN, 0);
                                }
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
    cout << "[3]. Leave\n";
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
    default:
        return 0;
    }
}
