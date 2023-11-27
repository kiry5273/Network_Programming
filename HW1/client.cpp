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
    int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    string ip = argv[1];
    string port_s = argv[2];
    stringstream ss;
    int port_n;
    ss<<port_s;
    ss>>port_n;

    if (tcp_sockfd == -1){
        cout<<"fail to create TCP socket."<<endl;
    }
    if (udp_sockfd == -1){
        cout<<"fail to create UDP socket."<<endl;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_n);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    int slen = sizeof(server_addr);

    if(connect(tcp_sockfd, (struct sockaddr*)&server_addr, (socklen_t)slen)==-1){
        cout<<"fail to connect."<<endl;
    }

    cout<<"*****Welcome to Game 1A2B*****"<<endl;
    while(1){
        string message;
        getline(cin, message);
        stringstream ss(message);
        string command;
        ss>>command;

        char receive[500]={};
        if(command=="register"||command=="game-rule"){
            //UDP
            sendto(udp_sockfd, message.c_str(), strlen(message.c_str()) +1 , 0, (struct sockaddr*)&server_addr, (socklen_t)slen);
            recvfrom(udp_sockfd, receive, sizeof(receive), 0, (struct sockaddr*)&server_addr, (socklen_t*)&slen);
            cout<<receive<<endl;
        }
        else if(command=="login"||command=="logout"||command=="start-game"){
            //TCP
            if(command=="start-game"){
                send(tcp_sockfd, message.c_str(), strlen(message.c_str())+1, 0);
                recv(tcp_sockfd, receive, sizeof(receive), 0);
                cout<<receive<<endl;
                if(strlen(receive)==strlen("Please typing a 4-digit number:")){
                    while(1){
                        string guess;
                        getline(cin, guess);
                        send(tcp_sockfd, guess.c_str(), strlen(guess.c_str())+1, 0);
                        recv(tcp_sockfd, receive, sizeof(receive), 0);
                        cout<<receive<<endl;
                        if(strlen(receive)==strlen("0A0B\nYou lose the game!")||strlen(receive)==strlen("You got the answer!")){
                            break;
                        }
                    }
                }
            }
            else{
                send(tcp_sockfd, message.c_str(), strlen(message.c_str())+1, 0);
                recv(tcp_sockfd, receive, sizeof(receive), 0);
                cout<<receive<<endl;
            }          
        }
        else if(command=="exit"){
            send(tcp_sockfd, message.c_str(), strlen(message.c_str())+1, 0);
            break;
        }
        else{
            cout<<"command not found."<<endl;
        }
    }
    
    return 0;
}
