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


#if defined(WIN32)
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wsock32.lib")
#else
#pragma comment(lib, "Pthread.lib")
#endif

#include <iostream>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <string.h>
#include <stdlib.h>

#if defined(WIN32)
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
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#endif


using namespace std;

#define BUFFLEN 1024
#define MAXCLIENTS 32

#if defined(WIN32)
void pos(short C, short R)
{
    COORD xy ;
    xy.X = C ;
    xy.Y = R ;
    SetConsoleCursorPosition(
    GetStdHandle(STD_OUTPUT_HANDLE), xy);
}
#endif
void cls( )
{
    #if defined(WIN32)
    pos(0,0);
    for(int j=0;j<100;j++)
    cout << string(100, ' ');
    pos(0,0);
    #else
    system("clear");
    #endif
}


void errorSwitch(int error) {
    #if defined(WIN32)
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
            case 10054:
                cout << "Connection reset by peer." << endl;
                break;
            default:
                cout << "Error occured: " << error << endl;
                break;
        }
    }
    #else
        if(error != 0) {
            switch(error) {
                case 98:
                    cout << "Address already in use" << endl;
                    break;
                case 104:
                    cout << "Connection reset by peer" << endl;
                    break;
                case 111:
                    cout << "Connection refused!" << endl;
                    break;
                default:
                    cout << "Error occured: " << error << endl;
                    break;
            } 
        }
    #endif
}

//For listenForServerOutput thread [clientSide]
atomic<bool> threadStatus;

void listenForServerOutput(int socket, fd_set read_set) {

    int maxfd = 0;
    char buffer[BUFFLEN];

    while(threadStatus == true) {
		//Nustatoma 0 bitu visiems FD
        FD_ZERO(&read_set);

		//Nustatomi socket bitai
        FD_SET(socket, &read_set);
        if (socket > maxfd){
            maxfd = socket;
        }

		//maxfd+1 Kiek fd(File descriptor) turi buti istestuota.
		//read_set output'as select'o
        int status = select(maxfd+1, &read_set, NULL , NULL, NULL);

        if(status < 0) {
            #if defined(WIN32)
            int kl = WSAGetLastError();
            #else
            int kl = errno;
            #endif

            errorSwitch(kl);
            threadStatus = false;
        } else if(status != 0) {
            FD_ZERO(&read_set);
            FD_SET(socket, &read_set);

            //Returns a non-zero value if the bit for the file descriptor (socket) is set in the file descriptor set pointed to by read_set, and 0 otherwise.
            if (FD_ISSET(socket, &read_set)){
                memset(&buffer,0,BUFFLEN);
                int r_len = recv(socket,(char*) &buffer,BUFFLEN,0);

                //Tikrinama ar ivyko klaida
                if(r_len < 0) {
                    //Jeigu ivyko klaida gaunant
                    #if defined(WIN32)
                    int klaida = WSAGetLastError();
                    #else
                    int klaida = errno;
                    #endif

                    errorSwitch(klaida);
                    threadStatus = false;
                }

                if(r_len > 0) {
                    if(buffer[0] != '[' && buffer[1] != ']'){
                        cout << buffer << endl;
                    }
                }
            }
        }
    }
}

int clientSide() {
    int s_socket;
    struct sockaddr_in servaddr; // Serverio adreso strukt�ra
    fd_set read_set;

    int maxfd = 0;

    char recvbuffer[BUFFLEN];
    char sendbuffer[BUFFLEN];

    string ip_address;

    cls();

    #if defined(WIN32)
    int iResult;

    //swaData laiko informacija apie windows socketu implementacija
    WSADATA wsaData;   // if this doesn't work

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        errorSwitch(iResult);

        return 1;
    }
    #endif

    s_socket = socket(AF_INET, SOCK_STREAM,0);
    if (s_socket< 0){
            #if defined(WIN32)
            int error = WSAGetLastError();
            #else
            int error = errno;
            #endif

        errorSwitch(error);
        exit(1);
    }

    cout << "Enter server port: ";
    int s_port;
    cin >> s_port;

    if ((s_port < 1) || (s_port > 65535)){
        printf("Invalid port specified.\n");
        exit(1);
    }

    cls();

   /*
    * I�valoma ir u�pildoma serverio strukt�ra
    */

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET; // nurodomas protokolas (IP)
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(s_port); // nurodomas portas
    unsigned long ulAddr = INADDR_NONE;
    ulAddr = inet_addr("127.0.0.1");

    if (ulAddr == INADDR_NONE ) {
        printf("inet_addr failed and returned INADDR_NONE\n");
        #if defined(WIN32)
        WSACleanup();
        #endif
        return 1;
    }

    if (ulAddr == INADDR_ANY) {
        printf("inet_addr failed and returned INADDR_ANY\n");
        #if defined(WIN32)
        WSACleanup();
        #endif
        return 1;
    }

    int error = connect(s_socket,(struct sockaddr*)&servaddr,sizeof(servaddr));
    if(error<0){
        #if defined(WIN32)
        error = WSAGetLastError();
        #else
        error = errno;
        #endif
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

            if(s_len < 0) {
                #if defined(WIN32)
                int error = WSAGetLastError();
                #else
                int error = errno;
                #endif
                errorSwitch(error);

                threadStatus = false;
                listeningForServer.join();
                done = true;
            }
        } else {
            done = true;
            threadStatus = false;

            string yy = "&" + to_string(s_socket);
            char *tt = new char[yy.length() + 1];
            strcpy(tt, yy.c_str());

            int st = send(s_socket, tt, strlen(tt), 0);

            listeningForServer.join();
        }
    }

    #if defined(WIN32)
    int err = closesocket(s_socket);
    #else
    int err = close(s_socket);
    #endif
    if(err != 0) {
        #if defined(WIN32)
        err = WSAGetLastError();
        #else
        err = errno;
        #endif
        errorSwitch(err);
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

    cout << "Enter server port: ";
    int port;
    cin >> port;

    if ((port < 1) || (port > 65535)){
        printf("Invalid port specified.\n");
        exit(1);
    }

    cls();

    cout << "Starting server..."  << endl;

    unsigned int clientaddrlen;
    int l_socket;
    Client clients[MAXCLIENTS];
    fd_set read_set;

    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;

    int maxfd = 0;
    int i;

    char buffer[BUFFLEN];

    #if defined(WIN32)
    int iResult;

    WSADATA wsaData;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    #endif

    if ((l_socket = socket(AF_INET, SOCK_STREAM,0)) < 0){
        errorSwitch(errno);        

        return -1;
    }

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        errorSwitch(errno);  

        return -1;
    }

    if (listen(l_socket, 5) <0){
        errorSwitch(errno);

        return -1;
    }

    for (i = 0; i < MAXCLIENTS; i++){
        clients[i].socket = -1;
    }

    cout << "Waiting for connections..." << endl;

    int sockets[MAXCLIENTS];
    for(int a = 0; a < MAXCLIENTS; a++) {
        clients[a].socket = -1;
    }

    int usersConnectedInTotal = 0;
    string outputf;
    while(true){
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

        #if defined(WIN32)
        static TIMEVAL tv;
        #else
        static timeval tv;
        #endif
        tv.tv_sec = 2;

        int rr = select(maxfd+1, &read_set, NULL , NULL, &tv);
        if(rr < 0) {
            errorSwitch(errno);
            return 1;
        }

        if (FD_ISSET(l_socket, &read_set)){
            for(int a = 0; a < MAXCLIENTS; a++) {
                sockets[a] = clients[a].socket;
            }

            int client_id = findemptyuser(sockets);
            if (client_id != -1){
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                #if defined(WIN32)
                clients[client_id].socket = accept(l_socket,
                    (struct sockaddr*)&clientaddr, (int*)&clientaddrlen);
                #else
                clients[client_id].socket = accept(l_socket,
                    (struct sockaddr*)&clientaddr, &clientaddrlen);
                #endif

                clients[client_id].ip = inet_ntoa(clientaddr.sin_addr);
                clients[client_id].name = "Guest_" + to_string(usersConnectedInTotal);

                //For naming users
                usersConnectedInTotal++;

                string msg = clients[client_id].name + " connected from: " + string(inet_ntoa(clientaddr.sin_addr));
                cout << msg << endl;

                strcpy(buffer, msg.c_str());
                for(int b = 0; b < MAXCLIENTS; b++) {
                    if(clients[b].socket != -1) {
                        int yy = send(clients[b].socket, buffer, BUFFLEN, 0);

                        if(yy < 0) {
                            errorSwitch(errno);
                            return 1;
                        }
                    }
                }
            }
        }

        for (i = 0; i < MAXCLIENTS; i++){
            if (clients[i].socket != -1){
                if (FD_ISSET(clients[i].socket, &read_set)){
                    memset(&buffer,0,BUFFLEN);
                    int r_len = recv(clients[i].socket,(char*) &buffer,BUFFLEN,0);

                    string msg = string(buffer);
                    if(r_len > 0) {
                        if(buffer[0] == '&') {
                            outputf = clients[i].name + " has disconnected!";
                            
                            #if defined(WIN32)
                            int rt = closesocket(clients[i].socket);
                            #else
                            int rt = close(clients[i].socket);
                            #endif

                            if(rt < 0) {
                                #if defined(WIN32)
                                errorSwitch(WSAGetLastError());
                                #else
                                errorSwitch(errno);
                                #endif
                            }

                            clients[i].socket = -1;
                            cout << outputf << endl;
                            strcpy(buffer, outputf.c_str());

                            for(int a = 0; a < MAXCLIENTS; a++) {
                                if(clients[a].socket != -1 && a != i) {
                                    int yy = send(clients[a].socket, buffer, BUFFLEN, 0);

                                    if(yy < 0) {
                                        errorSwitch(errno);
                                        return 1;
                                    }
                                }
                            }
                        } else if(msg.substr(0, 5) == "/name") {
                            string name = msg.substr(6, msg.length() - 6);
                            
                            clients[i].name = name;
                        } else {
                            outputf = clients[i].name + ": " + buffer;
                            cout << outputf << endl;
                            strcpy(buffer, outputf.c_str());

                            for(int a = 0; a < MAXCLIENTS; a++) {
                                if(clients[a].socket != -1 && a != i) {
                                    int yy = send(clients[a].socket, buffer, BUFFLEN, 0);

                                    if(yy < 0) {
                                        errorSwitch(yy);
                                        return 1;
                                    }
                                }
                            }
                        }
                    } else if(r_len < 0) {
                       errorSwitch(errno);
                       return 1;
                    }
                }
            }
        }

        for(int a = 0; a < MAXCLIENTS; a++) {
            char hg[3];
            hg[0] = '[';
            hg[1] = ']';

            if(clients[a].socket != -1) {
                int st = send(clients[a].socket, hg, 3, 0);

                if(st == 0) {
                    cout << clients[a].name << " has disconnected!" << endl;
                    #if defined(WIN32)
                    int st = closesocket(clients[a].socket);
                    #else
                    int st = close(clients[a].socket);
                    #endif

                    if(st < 0) {
                        #if defined(WIN32)
                        errorSwitch(WSAGetLastError());
                        #else
                        errorSwitch(st);
                        #endif
                    }
                    clients[a].socket = -1;
                } else if(st < 0) {
                    errorSwitch(errno);
                    return 1;
                }
            }
        }
    }
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

    return 0;
}
