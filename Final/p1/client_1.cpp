#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <sys/time.h>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <stdint.h>
using namespace std;

int main(int argc, char *argv[]){
    int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    string ip = argv[1];
    string port_s = argv[2];
    stringstream ss;
    int port_n;
    ss<<port_s;
    ss>>port_n;

    if (tcp_sockfd == -1){
        cout<<"fail to create TCP socket."<<endl;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_n);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    int slen = sizeof(server_addr);

    if(connect(tcp_sockfd, (struct sockaddr*)&server_addr, (socklen_t)slen)==-1){
        cout<<"fail to connect."<<endl;
    }
    cout<<"******************************\n";
    cout<<"* Welcome to the BBS server. *"<<endl;
    cout<<"******************************\n";
    char receive[500]={};
    recv(tcp_sockfd, receive, sizeof(receive), 0);
    cout<<receive;

    int ready_fdn;
    fd_set rfd_set;
    fd_set afd_set;
    int maxfd = max(tcp_sockfd, STDIN_FILENO);
    int connectfd;
    struct sockaddr_in client_addr;
    int add_len = sizeof(client_addr);
    FD_ZERO(&rfd_set); //initialize
    FD_ZERO(&afd_set);

    FD_SET(tcp_sockfd, &afd_set);
    FD_SET(STDIN_FILENO, &afd_set);
    while(1){
        char receive[500]={};
        memcpy(&rfd_set, &afd_set, sizeof(afd_set));
        ready_fdn = select(maxfd+1, &rfd_set, NULL, NULL, NULL);
        if(ready_fdn<0){
            cout<<"fail to select."<<endl;
        }

        if(FD_ISSET(STDIN_FILENO, &rfd_set)){
            //read from terminal
            string message;
            getline(cin, message);
            stringstream ss(message);
            string command;
            ss>>command;
            if(command=="Exit"){
                send(tcp_sockfd, message.c_str(), strlen(message.c_str())+1, 0);
                break;
            }
            else{
                //TCP
                send(tcp_sockfd, message.c_str(), strlen(message.c_str())+1, 0);  
            }
        }
        if(FD_ISSET(tcp_sockfd, &rfd_set)){
            //receive from server
            recv(tcp_sockfd, receive, sizeof(receive), 0);
            cout<<receive;
        }   
    }
    return 0;
}
