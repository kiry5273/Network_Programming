#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <fstream>
#include <vector>


using namespace std;

int main(int argc, char *argv[]){
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    string ip = argv[1];
    string port_s = argv[2];
    stringstream ss;
    int port_n;
    ss<<port_s;
    ss>>port_n;

    if(udp_socket == -1){
        cout<<"fail to create UDP socket."<<endl;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_n);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    int slen = sizeof(server_addr);

    //start ot interact with server
    while(1){
        string message;
        cout<<"% ";
        getline(cin, message);
        stringstream ss(message);
        string command;
        vector<string> arg_v;
        while(1){
            if(ss.fail()) break;
            ss>>command;
            arg_v.push_back(command);
        }
        
        char recieve[1024] = {};
        if(message=="exit"){
            sendto(udp_socket, message.c_str(), strlen(message.c_str())+1, 0, (struct sockaddr*)&server_addr, (socklen_t)slen);
            break;
        }
        else if(command=="get-file"){
            int i=0;
            sendto(udp_socket, message.c_str(), strlen(message.c_str())+1, 0, (struct sockaddr*)&server_addr, (socklen_t)slen);
            while(1){
                ofstream ofs;

                i++;
                recvfrom(udp_socket, recieve, sizeof(recieve), 0, (struct sockaddr*)&server_addr, (socklen_t*)&slen);
                if(recieve=="done") break;

                string file_name = arg_v[i];
                ofs.open(file_name);
                ofs<<recieve;
                ofs.close();
            }
        }
        else{
            sendto(udp_socket, message.c_str(), strlen(message.c_str())+1, 0, (struct sockaddr*)&server_addr, (socklen_t)slen);
            recvfrom(udp_socket, recieve, sizeof(recieve), 0, (struct sockaddr*)&server_addr, (socklen_t*)&slen);
            cout<<recieve<<endl;
        }   
    }

    return 0;
}