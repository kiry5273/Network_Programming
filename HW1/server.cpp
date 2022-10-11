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

using namespace std;

#define MAX_CON 20

struct user{
    string name;
    string email;
    string password;
    int login;
};
map<string, struct user> user_d;
vector<string> mail_v;
vector<string> current_user(1024, "");
vector<int> user_life(1024, 5);
vector<string> answer_v(1024, "");
vector<int> start_game_v(1024, 0);
bool is_num(string str){
    auto it = str.begin();
    while (it != str.end() && isdigit(*it)) {
        it++;
    }
    return it == str.end();
}

int regist(string, struct sockaddr_in, int);
int game_rule(string, struct sockaddr_in, int);
int login(string, struct sockaddr_in, int);
int logout(string, struct sockaddr_in, int);
int start_game(string, struct sockaddr_in, int);
int exit(string, struct sockaddr_in, int);

int main(int argc, char *argv[]){
    stringstream ss;
    int port_n;
    ss<<argv[1];
    ss>>port_n;

    //create socket
    int tcp_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    int udp_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(tcp_socketfd==-1){
        cout<<"fail to create TCP socket."<<endl;
    }
    if (udp_socketfd==-1){
        cout<<"fail to create UDP socket."<<endl;
    }
    // cout<<"tcp socketfd: "<<tcp_socketfd<<endl;
    // cout<<"udp socketfd: "<<udp_socketfd<<endl;
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
    if(bind(udp_socketfd, (struct sockaddr*)&server_addr, (socklen_t)slen)==-1){
        cout<<"fail to bind UDP."<<endl;
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
    int maxfd = max(tcp_socketfd, udp_socketfd);
    int max_i = MAX_CON;
    int connectfd;
    struct sockaddr_in client_addr;
    int add_len = sizeof(client_addr);
    char buff[1024] = {'\0'};
    FD_ZERO(&rfd_set); //initialize
    for(int i=0; i<MAX_CON; i++){
        client[i] = -1;
    }

    FD_SET(tcp_socketfd, &afd_set);  
    FD_SET(udp_socketfd, &afd_set);
    while(1){
        
        rfd_set = afd_set;
        // cout<<"here"<<endl;
        ready_fdn = select(maxfd+1, &rfd_set, NULL, NULL, NULL);
        // cout<<"there\n";
        // cout<<"readt_fdn: "<<ready_fdn<<endl;
        if(ready_fdn<0){
            cout<<"fail to select."<<endl;
        }

        //TCP
        //listen
        if(FD_ISSET(tcp_socketfd, &rfd_set)){
            connectfd = accept(tcp_socketfd, (struct sockaddr*) &client_addr, (socklen_t*) &add_len);
            cout<<"New connection. "<<endl;
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
                    close(client[i]);
                    FD_CLR(client[i], &rfd_set);
                    client[i] = -1;
                }
                else{
                    //function: login, logout, start game, exit
                    string message=buff;
                    stringstream ss(message);
                    string command;
                    ss>>command;
                    if(command=="login"){
                        login(message, client_addr, client[i]);
                    }
                    else if(command=="logout"){
                        logout(message, client_addr, client[i]);
                    }
                    else if(command=="exit"){
                        // cout<<client[i]<<endl;
                        exit(message, client_addr, client[i]);
                        FD_CLR(client[i], &afd_set);
                    }
                    else{
                        start_game(message, client_addr, client[i]);
                    }
                    
                }
                ready_fdn--;
                if(ready_fdn<=2) break;
            }
        }


        //UDP
        if(FD_ISSET(udp_socketfd, &rfd_set)){
            int clen = sizeof(client_addr);
            int char_n = recvfrom(udp_socketfd, buff, sizeof(buff), 0, (struct sockaddr*) &client_addr, (socklen_t*) &clen);
            if(char_n==-1){
                    cout<<"fail to recieve."<<endl;
            }
            else if(char_n==0){
                cout<<"connection done."<<endl;
            }
            else{
                //function: register, game rule
                string message=buff;
                stringstream ss(message);
                string command;
                ss>>command;
                if(command=="register"){
                    regist(message, client_addr, udp_socketfd);
                }
                else if(command=="game-rule"){
                    game_rule(message, client_addr, udp_socketfd);
                }
            }
        }

    }
return 0;
}

int login(string message, struct sockaddr_in client_addr, int sfd){
    int clen = sizeof(client_addr);
    int arg_count=0;
    string send_back;
    stringstream ss(message);
    map<string, struct user>::iterator iter;
    int flag=1;
    string name;
    while(1){
        if(ss.fail()) break;
        string info;
        ss>>info;
        arg_count++;
        if(arg_count==2){
            name = info;
            iter = user_d.find(info);
            if(iter==user_d.end()){
                //fail 2: can't found user
                send_back = "Username not found.";
                flag=0;
            }
            else{
                if(user_d[info].login==1){
                    //fail 1: already login
                    send_back = "Please logout first.";
                    flag=0;
                }
            }
        }
        else if(arg_count==3&&flag){
            if(user_d[name].password!=info&&flag){
                //fail 3: password incorrect
                send_back = "Password not correct.";
            }
            else if(flag){
                current_user[sfd] = name;
                send_back = "Welcome, "+current_user[sfd]+".";
                user_d[current_user[sfd]].login = 1;
            }
        }
    }
    if(arg_count!=4){
        send_back = "Usage: login <username> <password>";
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int logout(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;
    if(current_user[sfd]==""){
        //fail
        send_back = "Please login first.";
    }
    else{
        send_back = "Bye, "+current_user[sfd]+".";
        user_d[current_user[sfd]].login = 0;
        current_user[sfd] = "";
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int start_game(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;
    if(is_num(message)&&message.length()==4){
        //guess
        int a=0;
        int b=0;
        string ans = answer_v[sfd];
        int opened[4]={1, 1, 1, 1};
        for(int i=0; i<4; i++){
            if(message[i]==ans[i]){
                a++;
                opened[i]=0;
                continue;
            }
            else{
                for(int j=0; j<4; j++){
                    if(message[i]==ans[j]&&opened[j]){
                        b++;
                        opened[j]=0;
                        break;
                    }
                }
            }
        }
        stringstream ss, ss2;
        string a_s, b_s;
        ss<<a;
        ss>>a_s;
        ss2<<b;
        ss2>>b_s;
        if(a==4&&b==0){
            send_back="You got the answer!";
            start_game_v[sfd] = 0;
        }
        else{
            user_life[sfd]--;
            if(user_life[sfd]==0){
                send_back = a_s+"A"+b_s+"B\nYou lose the game!";
                start_game_v[sfd] = 0;
            }
            else{
                send_back = a_s+"A"+b_s+"B";
            }
        }
        
    }
    else{
        //start game
        if(start_game_v[sfd]){
            send_back = "Your guess should be a 4-digit number.";
        }
        else if(current_user[sfd]==""){
            send_back = "Please login first.";
        }
        else{
            stringstream ss(message);
            int arg_count=0;
            while(1){
                if(ss.fail()) break;
                string info;
                ss>>info;
                arg_count++;
                if(arg_count==2){
                    if(is_num(info)&&info.length()==4){
                        answer_v[sfd]=info;
                        send_back = "Please typing a 4-digit number:";
                        start_game_v[sfd] = 1;
                    }
                    else{
                        send_back = "Usage: start-game <4-digit number>";
                    }
                }
            }
            if(arg_count!=3){
                //random an answer
                unsigned seed = (unsigned)time(NULL);
                srand(seed);
                int answer = rand()% 8889 + 1111;
                stringstream ss;
                string ans_s;
                ss<<answer;
                ss>>ans_s;
                answer_v[sfd] = ans_s;
                send_back = "Please typing a 4-digit number:";
                start_game_v[sfd] = 1;
            }
        }
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0);
    return 0;
}

int exit(string message, struct sockaddr_in client_addr, int sfd){
    if(current_user[sfd]!=""){
        user_d[current_user[sfd]].login = 0;
        current_user[sfd] = "";
    }
    close(sfd);
    return 0;
}

int regist(string message, struct sockaddr_in client_addr, int udp_socketfd){
    int clen = sizeof(client_addr);
    int arg_count=0;
    string send_back;
    stringstream ss(message);
    struct user p;
    map<string, struct user>::iterator iter;
    vector<string>::iterator iter_v;
    int flag=1;
    while(1){
        if(ss.fail()) break;
        string info;
        ss>>info;
        arg_count++;
        if(arg_count==2){
            //name
            iter = user_d.find(info);
            if(iter==user_d.end()){
                p.name = info;
            }
            else{
                //fail 1
                send_back = "Username is already used.";
                cout<<iter->second.name<<endl;
                flag=0;
            }
        }
        else if(arg_count==3){
            //email
            iter_v = find(mail_v.begin(), mail_v.end(), info);
            if(iter_v==mail_v.end()){
                p.email = info;
                mail_v.push_back(info);
            }
            else{
                //fail 2
                send_back = "Email is already used.";
                flag=0;
            }
        }
        else if(arg_count==4&&flag){
            //password
            p.password=info;
            p.login=0;
            user_d[p.name] = p;
            send_back = "Register successfully.";
        }
        
    }
    if(arg_count!=5){
        //command format
        send_back = "Usage: register <username> <email> <password>";
    }
    sendto(udp_socketfd, send_back.c_str(), strlen(send_back.c_str())+1, 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
    return 0;
}

int game_rule(string message, struct sockaddr_in client_addr, int udp_socketfd){
    int clen = sizeof(client_addr);
    string rule = "1. Each question is a 4-digit secret number.\n\
2. After each guess, you will get a hint with the following information:\n\
2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n\
2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\nThe hint will be formatted as \"xAyB\".\n\
3. 5 chances for each question.";
    sendto(udp_socketfd, rule.c_str(), strlen(rule.c_str())+1, 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
    return 0;
}



