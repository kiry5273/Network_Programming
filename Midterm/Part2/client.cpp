#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

int main(int argc, char *argv[]){
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    string ip = argv[1];
    string port_s = argv[2];
    stringstream ss;
    int port_n;
    ss<<port_s;
    ss>>port_n;
    if (tcp_socket == -1){
        cout<<"fail to create TCP socket."<<endl;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_n);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    int slen = sizeof(server_addr);

    if(connect(tcp_socket, (struct sockaddr*)&server_addr, (socklen_t)slen)==-1){
        cout<<"fail to connect."<<endl;
    }
    char receive[500]={};
    recv(tcp_socket, receive, sizeof(receive), 0);
    cout<<receive<<endl;

    while(1){
        string message;
        cout<<"% ";
        getline(cin, message);
        char receive[500]={};

        sendto(tcp_socket, message.c_str(), strlen(message.c_str())+1, 0, (struct sockaddr*)&server_addr, (socklen_t)slen);
        recv(tcp_socket, receive, sizeof(receive), 0);
        cout<<receive<<endl;

        if(message=="exit"){
            break;
        }
    }
}
