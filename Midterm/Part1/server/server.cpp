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
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

int get_list(string, struct sockaddr_in, int);
int get_file(string, struct sockaddr_in, int);
int exit(string, struct sockaddr_in, int);

int main(int argc, char *argv[]){
    stringstream ss;
    int port_n;
    ss<<argv[1];
    ss>>port_n;

    //create socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket==-1){
        cout<<"fail to create UDP socket."<<endl;
    }

    //bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_n);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int slen = sizeof(server_addr);
    if(bind(udp_socket, (struct sockaddr*)&server_addr, (socklen_t)slen)==-1){
        cout<<"fail to bind UDP"<<endl;
    }

    //recieve
    
    struct sockaddr_in client_addr;
    int clen = sizeof(client_addr);
    char buff[1024] = {'\0'};
    while(1){
        int char_n = recvfrom(udp_socket, buff, sizeof(buff), 0, (struct sockaddr*)&client_addr, (socklen_t*)&clen);
        if(char_n==-1) cout<<"fail to recieve."<<endl;
        else if(char_n==0) cout<<"connection done."<<endl;
        else{
            string message=buff;
            stringstream ss(message);
            string command;
            ss>>command;
            if(command=="get-file-list") get_list(message, client_addr, udp_socket);
            else if(command == "get-file") get_file(message, client_addr, udp_socket);
            else if(command == "exit") exit(message, client_addr, udp_socket);
        }
    }
    return 0;
}

int get_list(string message, struct sockaddr_in client_addr, int sfd){
    int clen = sizeof(client_addr);
    string send_back = "File:";

    fs::path path("/home/kirsten/Desktop/mid_practice/server_");
    fs::directory_iterator it;
    for(const auto& iter : fs::directory_iterator(path)){
        string file_name = iter.path().filename().string();
        send_back+=(" "+file_name);
    };

    sendto(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
    return 0;
}
int get_file(string message, struct sockaddr_in client_addr, int sfd){
    int clen = sizeof(client_addr);
    string send_back="done";
    string send2 = "not_yet";
    stringstream ss(message);
    while(1){
        if(ss.fail()) break;
        
        string file_name;
        ss>>file_name;
        if(file_name=="get-file") continue;

        ifstream ifs;
        char buffer[1024] = {'\0'};
        ifs.open(file_name);
        ifs.read(buffer, sizeof(buffer));
        sendto(sfd, buffer, strlen(send_back.c_str())+1, 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
        ifs.close();
    }
    
    sendto(sfd, send_back.c_str(), strlen(send_back.c_str())+1, 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
    return 0;
}
int exit(string message, struct sockaddr_in client_addr, int sfd){
    //close(sfd);
    //udp doesn't have to close the socket
    return 0;
}

