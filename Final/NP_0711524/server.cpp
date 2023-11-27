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
#include <stdint.h>
using namespace std;

#define MAX_CON 20
struct user{
    int mode;
    int id;
    int sfd;
    string name;
};

vector<string> current_user(1024, "");
map<string, struct user> user_d;
int user_id_count=0;

int initial(struct sockaddr_in, int);
int mute(string, struct sockaddr_in, int);
int unmute(string, struct sockaddr_in, int);
int yell(string, struct sockaddr_in, int);
int tell(string, struct sockaddr_in, int);
int exit(string, struct sockaddr_in, int);

int main(int argc, char *argv[]){
    stringstream ss;
    int port_n;
    ss<<argv[1];
    ss>>port_n;

    //create socket
    int tcp_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_socketfd==-1){
        cout<<"fail to create TCP socket."<<endl;
    }
    
    //bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_n);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int slen = sizeof(server_addr);
    int on=1;
    int ret = setsockopt(tcp_socketfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(bind(tcp_socketfd, (struct sockaddr*)&server_addr, (socklen_t)slen)==-1){
        cout<<"fail to bind TCP."<<endl;
    }

    //listen
    if(listen(tcp_socketfd, MAX_CON)==-1){
        cout<<"fail to listen."<<endl;
    }

    cout<<"Waiting for connections......"<<endl;

    //select and accept
    int ready_fdn;
    fd_set rfd_set;
    fd_set afd_set;
    int client[MAX_CON];
    int maxfd = tcp_socketfd;
    int max_i = MAX_CON;
    int connectfd;
    struct sockaddr_in client_addr;
    int add_len = sizeof(client_addr);
    FD_ZERO(&rfd_set); //initialize
    FD_ZERO(&afd_set);
    for(int i=0; i<MAX_CON; i++){
        client[i] = -1;
    }

    FD_SET(tcp_socketfd, &afd_set);  
    while(1){
        char buff[1024] = {'\0'};
        memcpy(&rfd_set, &afd_set, sizeof(afd_set));
        ready_fdn = select(maxfd+1, &rfd_set, NULL, NULL, NULL);
        if(ready_fdn<0){
            cout<<"fail to select."<<endl;
        }
        //TCP
        //listen
        if(FD_ISSET(tcp_socketfd, &rfd_set)){
            connectfd = accept(tcp_socketfd, (struct sockaddr*) &client_addr, (socklen_t*) &add_len);
            cout<<"New connection. "<<endl;
            initial(client_addr, connectfd);
            //add new fd in client array
            for(int i=0; i<MAX_CON; i++){
                if(client[i]==-1){
                    client[i] = connectfd;
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
                    exit("", client_addr, client[i]);
                    FD_CLR(client[i], &afd_set);
                    client[i] = -1;
                }
                else{
                    //function:
                    string message=buff;
                    stringstream ss;
                    string command;
                    ss<<message;
                    ss>>command;
                    if(command=="mute" ){
                        mute(message, client_addr, client[i]);
                    }
                    else if(command=="unmute"){
                        unmute(message, client_addr, client[i]);
                    }
                    else if(command=="yell"){
                        yell(message, client_addr, client[i]);
                    }
                    else if(command=="tell"){
                        tell(message, client_addr, client[i]);
                    }
                    else if(command=="exit"){
                        exit(message, client_addr, client[i]);
                        FD_CLR(client[i], &afd_set);
                        client[i] = -1;
                    }
                }
                ready_fdn--;
                if(ready_fdn<=1) break;
            }
        }

    }
    return 0;
}

int initial(struct sockaddr_in client_addr, int sfd){
    stringstream ss;
    ss<<user_id_count;
    string id_s;
    ss>>id_s;

    struct user p;
    p.id = user_id_count;
    p.name = "user"+id_s;
    p.mode = 1;
    p.sfd = sfd;
    user_d[p.name] = p;
    current_user[sfd] = p.name;
    user_id_count++;

    string send_back;
    send_back = "Welcome, "+p.name+"\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int mute(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;
    string user_name = current_user[sfd];
    map<string, struct user>::iterator iter;
    iter = user_d.find(user_name);
    if(iter->second.mode==0){
        send_back = "You are already in mute mode.\n";
    }
    else{
        iter->second.mode = 0;
        send_back = "Mute mode.\n";
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int unmute(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;
    string user_name = current_user[sfd];
    map<string, struct user>::iterator iter;
    iter = user_d.find(user_name);
    if(iter->second.mode==1){
        send_back = "You are already in unmute mode.\n";
    }
    else{
        iter->second.mode = 1;
        send_back = "Unmute mode.\n";
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int yell(string message, struct sockaddr_in client_addr, int sfd){
    string arg[1024]= {""} ;
    int clen = sizeof(client_addr);
    int arg_count=0;
    
    string send_back;
    stringstream ss(message);
    while(1){
        if(ss.fail()) break;
        string info;
        ss>>info;
        if(info=="") break;
        arg[arg_count]=info;
        arg_count++;
    }
    if(arg_count<2){
        send_back = "Usage: yell <message>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        return 0;
    }
    string m = arg[1]+" ";
    int i=2;
    while(arg[i]!=""){
        m+=(arg[i]+" ");
        i++;
    }
    for(map<string, struct user>::iterator iter=user_d.begin(); iter!=user_d.end(); iter++){
        struct user p = iter->second;
        if(p.mode==1&&p.sfd!=sfd){
            send_back = current_user[sfd]+": "+m+"\n";
            send(p.sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        }
    }
    return 0;
}

int tell(string message, struct sockaddr_in client_addr, int sfd){
    string arg[1024]={""};
    int clen = sizeof(client_addr);
    int arg_count=0;
    
    string send_back;
    stringstream ss(message);
    while(1){
        if(ss.fail()) break;
        string info;
        ss>>info;
        if(info=="") break;
        arg[arg_count]=info;
        arg_count++;
    }
    if(arg_count<3){
        send_back = "Usage: tell <receiver> <message>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        return 0;
    }

    send_back=arg[2]+" ";
    int i=3;
    while(arg[i]!=""){
        send_back+=(arg[i]+" ");
        i++;
    }
    map<string, struct user>::iterator iter;
    iter = user_d.find(arg[1]);
    if(iter==user_d.end()){
        send_back = arg[1]+" does not exist.\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        return 0;
    }
    struct user receiver = iter->second;
    if(receiver.sfd==sfd){
        send_back = "You cannot tell yourself.\n";
         send(receiver.sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    }
    if(receiver.mode==1){
        send_back = current_user[sfd] + " told you: " + send_back +"\n";
        send(receiver.sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    }
    return 0;
}

int exit(string message, struct sockaddr_in client_addr, int sfd){
    user_d.erase(current_user[sfd]);
    current_user[sfd]= "";
    close(sfd);
    return 0;
}
