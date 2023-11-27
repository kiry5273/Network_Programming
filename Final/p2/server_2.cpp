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
int account1=0;
int account2=0;
int user_id_count=1;
int show_account(string, struct sockaddr_in, int);
int deposit(string, struct sockaddr_in, int);
int withdraw(string, struct sockaddr_in, int);
int exit(string, struct sockaddr_in, int);
//vector<int> user_list(1024, 0);
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
    //initialize
    FD_ZERO(&rfd_set); 
    FD_ZERO(&afd_set); 
    for(int i=0; i<MAX_CON; i++){
        client[i] = -1;
    }

    FD_SET(tcp_socket, &afd_set);
    while(1){
        char buff[1024] = {'\0'};
        memcpy(&rfd_set, &afd_set, sizeof(afd_set));
        ready_fdn = select(maxfd+1, &rfd_set, NULL, NULL, NULL);
        if(ready_fdn<0){
            cout<<"fail to select."<<endl;
        }

        //accept
        if(FD_ISSET(tcp_socket, &rfd_set)){
            connectfd = accept(tcp_socket, (struct sockaddr*) &client_addr, (socklen_t*) &add_len);

            string name;
            if(user_id_count==1) name = "A";
            else if(user_id_count==2) name = "B";
            else if(user_id_count==3) name = "C";
            else if(user_id_count==4) name = "D";
            else user_id_count=1;

            cout<<"New connection from "<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port)<<" "<<name<<endl;
            current_user[connectfd] = name; //add new user
            user_id_count++;

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
                    stringstream ss;
                    string command;
                    ss<<message;
                    ss>>command;
                    string send_back = "command not found.";
                    if(command=="show-accounts") show_account(message, addr[i], client[i]);
                    else if(command=="deposit") deposit(message, addr[i], client[i]);
                    else if(command=="withdraw") withdraw(message, addr[i], client[i]);
                    else if(command=="exit") {
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

int show_account(string message, struct sockaddr_in client_addr, int sfd){
    stringstream ss;
    ss<<account1;
    string s1;
    ss>>s1;

    stringstream ss2;
    ss2<<account2;
    string s2;
    ss2>>s2;

    string send_back = "ACCOUNT1: "+s1+"\nACCOUNT2: "+s2+"\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int deposit(string message, struct sockaddr_in client_addr, int sfd){
    string arg[3];
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
    if(arg_count!=3){
        send_back = "Usage: deposit <account> <money>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        return 0;
    }

    stringstream ss2;
    ss2<<arg[2];
    int money;
    ss2>>money;
    if(money<=0){
        cout<<money<<endl;
        send_back = "Deposit a non-positive number into accounts.\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        return 0;
    }

    if(arg[1]=="ACCOUNT1"){
        account1+=money;
    }
    else if(arg[1]=="ACCOUNT2"){
        account2+=money;
    }
    send_back = "Successfully deposits "+arg[2]+" into "+arg[1]+".\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int withdraw(string message, struct sockaddr_in client_addr, int sfd){
    string arg[3];
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
    if(arg_count!=3){
        send_back = "Usage: withdraw <account> <money>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        return 0;
    }

    stringstream ss2;
    ss2<<arg[2];
    int money;
    ss2>>money;
    if(money<=0){
        send_back = "Withdraw a non-positive number into accounts.\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
        return 0;
    }

    if(arg[1]=="ACCOUNT1"){
        if(account1-money<0){
            send_back = "Withdraw excess money from accounts.\n";
            send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
            return 0;
        }
        else{
            account1-=money;
        }
    }
    else if(arg[1]=="ACCOUNT2"){
        if(account2-money<0){
            send_back = "Withdraw excess money from accounts.\n";
            send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
            return 0;
        }
        else{
            account2-=money;
        }
    }
    send_back = "Successfully withdraws "+arg[2]+" from "+arg[1]+".\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int exit(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;

    cout<<current_user[sfd]<<" "<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port)<<" disconnected"<<endl;
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);

    //close
    current_user[sfd]="";
    return 0;
}
