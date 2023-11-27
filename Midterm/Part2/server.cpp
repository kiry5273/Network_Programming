#include <stdio.h>
#include <stdlib.h>
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
#include <sys/select.h>

using namespace std;

#define MAX_CON 20
int user_n=1;
int list_user(string, struct sockaddr_in, int);
int get_ip(string, struct sockaddr_in, int);
int exit(string, struct sockaddr_in, int);
vector<int> user_list(1024, 0);
vector<string> current_user(1024, "");

int main(int argc, char *argv[]){
    stringstream ss;
    int port_n;
    ss<<argv[1];
    ss>>port_n;

    //create socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_socket==-1){
        cout<<"fail to create TCP socket."<<endl;
    }

    //bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_n);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int slen = sizeof(server_addr);
    int on=1;
    int ret = setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(bind(tcp_socket, (struct sockaddr*)&server_addr, (socklen_t)slen)==-1){
        cout<<"fail to bind TCP."<<endl;
    }

    //listen
    if(listen(tcp_socket, MAX_CON)==-1){
        cout<<"fail to listen."<<endl;
    }

    //select and accept
    int ready_fdn;
    fd_set rfd_set;
    fd_set afd_set;
    int client[MAX_CON];
    int maxfd = tcp_socket;
    int max_i = MAX_CON;
    int connectfd;
    struct sockaddr_in client_addr;
    struct sockaddr_in addr[MAX_CON];
    int add_len = sizeof(client_addr);
    char buff[1024] = {'\0'};
    //initialize
    FD_ZERO(&rfd_set); 
    FD_ZERO(&afd_set); 
    for(int i=0; i<MAX_CON; i++){
        client[i] = -1;
    }

    FD_SET(tcp_socket, &afd_set);
    while(1){
        rfd_set = afd_set;
        ready_fdn = select(maxfd+1, &rfd_set, NULL, NULL, NULL);
        if(ready_fdn<0){
            cout<<"fail to select."<<endl;
        }

        //accept
        if(FD_ISSET(tcp_socket, &rfd_set)){
            connectfd = accept(tcp_socket, (struct sockaddr*) &client_addr, (socklen_t*) &add_len);

            stringstream ss;
            string num_s;
            ss<<user_n;
            ss>>num_s;
            cout<<"New connection from "<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port)<<"user"<<num_s<<endl;
            current_user[connectfd] = num_s; //add new user
            user_list[user_n]=1; //add new user
            string send_back = "Welcome, you are user" + num_s + ".";
            send(connectfd, send_back.c_str(),  strlen(send_back.c_str())+1, 0);
            user_n++;

            //add new fd in client array
            for(int i=0; i<MAX_CON; i++){
                if(client[i]==-1){
                    client[i] = connectfd;
                    addr[i] = client_addr;
                    FD_SET(client[i], &afd_set);
                    if(client[i]>maxfd) maxfd=client[i];
                    break;
                }
            }
        }
        //process client request
        for(int i=0; i<max_i; i++){
            if(client[i]==-1) continue;
            if(FD_ISSET(client[i], &rfd_set)){
                int char_n = recv(client[i], buff, sizeof(buff), 0);
                if(char_n==-1){
                    cout<<"fail to recieve."<<endl; 
                }
                else if(char_n==0){
                    close(client[i]);
                    FD_CLR(client[i], &afd_set);
                    client[i] = -1;
                }
                else{
                    string message=buff;
                    string send_back = "command format.";
                    if(message=="list-users") list_user(message, addr[i], client[i]);
                    else if(message=="get-ip") get_ip(message, addr[i], client[i]);
                    else if(message=="exit") {
                        exit(message, addr[i], client[i]);
                        close(client[i]);
                        FD_CLR(client[i], &afd_set);
                        client[i] = -1;
                    }
                    else send(client[i], send_back.c_str(), strlen(send_back.c_str())+1, 0);
                }
                ready_fdn--;
                if(ready_fdn<=1) break;
            }
        }
    }
    return 0;
}

int list_user(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;

    int flag=1;
    for(int i=0; i<1024; i++){
        if(user_list[i]==1){
            stringstream ss;
            string nums;
            ss<<i;
            ss>>nums;
            if(flag) {
                send_back+=("user"+nums);
                flag=0;
            }
            else send_back+=("\nuser"+nums);
        }
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int get_ip(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;

    string ip(inet_ntoa(client_addr.sin_addr));
    stringstream ss;
    string port;
    ss<<ntohs(client_addr.sin_port);
    ss>>port;
    send_back = "IP: "+ ip +":"+port;

    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int exit(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;

    cout<<"user"<<current_user[sfd]<<" "<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port)<<"disconnected"<<endl;
    send_back = "Bye, user"+current_user[sfd]+".";
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);

    //close
    stringstream ss;
    int user_n;
    ss<<current_user[sfd];
    ss>>user_n;
    current_user[sfd]='\0';
    user_list[user_n]=0;

    return 0;
}
