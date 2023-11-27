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
#include <aws/core/Aws.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/transfer/TransferManager.h>
#include <aws/transfer/TransferHandle.h>
#include <aws/s3/S3Client.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/core/utils/StringUtils.h>
#include <fstream>

using namespace std;
using namespace std;
using namespace Aws;
using namespace Aws::Utils;
using namespace Aws::S3;

#define MAX_CON 20

struct user{
    string name;
    string email;
    string password;
    int login;
    int room_id;
    int sfd;
    map<int, string> invite_m;
}user;

struct room{
    int id;
    int type;
    int status=0;
    uint32_t code=-1;
    string host;
    int round;
    int turn=0;
    string answer;
    int user_num = 0;
    int user_count = 0;
    vector<int> users_sfd;
}room;

map<string, struct user> user_d;
map<int, struct room> room_d;
map<string, string> mail_d;
vector<string> mail_v;
vector<string> current_user(1024, "");
vector<int> user_life(1024, 5);
vector<string> answer_v(1024, "");
vector<int> start_game_v(1024, 0);
int user_amount = 0;
static const size_t BUFFER_SIZE = 512 * 1024 * 1024; // 512MB Buffer 
static size_t g_file_size = 0;

class MyUnderlyingStream : public Aws::IOStream
{
    public:
        using Base = Aws::IOStream;
        // Provide a customer-controlled streambuf to hold data from the bucket.
        MyUnderlyingStream(std::streambuf* buf)
            : Base(buf)
        {}

        virtual ~MyUnderlyingStream() = default;
};

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
int guess(string, struct sockaddr_in, int);
int exit(string, struct sockaddr_in, int);
int creat_public(string, struct sockaddr_in, int);
int creat_private(string, struct sockaddr_in, int);
int list_room(string, struct sockaddr_in, int);
int list_users(string, struct sockaddr_in, int);
int join_room(string, struct sockaddr_in, int);
int invite(string, struct sockaddr_in, int);
int list_invite(string, struct sockaddr_in, int);
int accept(string, struct sockaddr_in, int);
int leave(string, struct sockaddr_in, int);
int build_file();
int upload_file();
int download_file(string);
int status(string, struct sockaddr_in, int);

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
    
    //bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
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
    build_file();
    upload_file();

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
    FD_ZERO(&rfd_set); //initialize
    FD_ZERO(&afd_set);
    for(int i=0; i<MAX_CON; i++){
        client[i] = -1;
    }

    FD_SET(tcp_socketfd, &afd_set);  
    FD_SET(udp_socketfd, &afd_set);
    while(1){
        char buff[1024] = {'\0'};
        //rfd_set = afd_set;
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
                    //function: login, logout, start game, exit, create public/private, join room,
                    //invite, list invitation, accept, leave room
                    string message=buff;
                    stringstream ss;
                    string command;
                    //cout<<message;
                    if(message=="exit"||message=="\0"){
                        exit(message, client_addr, client[i]);
                        FD_CLR(client[i], &afd_set);
                        client[i] = -1;
                    }
                    else{
                        ss<<message;
                        ss>>command;
                    }
                    //cout<<command<<endl;
                    if(command=="login" ){
                        login(message, client_addr, client[i]);
                    }
                    else if(command=="logout"){
                        logout(message, client_addr, client[i]);
                    }
                    else if(command=="create"){
                        string second;
                        ss>>second;
                        if(second=="public") creat_public(message, client_addr, client[i]);
                        else if(second=="private") creat_private(message, client_addr, client[i]);
                    }
                    else if(command=="list"){
                        list_invite(message, client_addr, client[i]);
                    }
                    else if(command=="join"){
                        join_room(message, client_addr, client[i]);
                    }
                    else if(command=="accept"){
                        accept(message, client_addr, client[i]);
                    }
                    else if(command=="leave"){
                        leave(message, client_addr, client[i]);
                    }
                    else if(command=="start"){
                        start_game(message, client_addr, client[i]);
                    }
                    else if(command=="guess"){
                        guess(message, client_addr, client[i]);
                    }
                    else if(command=="invite"){
                        invite(message, client_addr, client[i]);
                    }
                    else if(command=="status"){
                        status(message, client_addr, client[i]);
                    }
                    else if(command=="exit"){
                        exit(message, client_addr, client[i]);
                        FD_CLR(client[i], &afd_set);
                        client[i] = -1;
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
                //function: register, game rule, list room, list user
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
                else if(command=="list"){
                    string second;
                    ss>>second;
                    if(second=="rooms") list_room(message, client_addr, udp_socketfd);
                    else if(second=="users") list_users(message, client_addr, udp_socketfd);
                }
            }
        }
    }
    return 0;
}

int status(string message, struct sockaddr_in client_addr, int sfd){
    int clen = sizeof(client_addr);
    string send_back="";

    download_file("user_count1.txt");
    download_file("user_count2.txt");
    download_file("user_count3.txt");

    ifstream file1;
    ifstream file2;
    ifstream file3;

    file1.open("user_count1.txt");
    file2.open("user_count2.txt");
    file3.open("user_count3.txt");
    
    string line;
    while (getline(file1, line)){
        send_back+="Server1: ";
        send_back+=line;
        send_back+="\n";
    }
    while (getline(file2, line)){
        send_back+="Server2: ";
        send_back+=line;
        send_back+="\n";
    }
    while (getline(file3, line)){
        send_back+="Server3: ";
        send_back+=line;
        send_back+="\n";
    }

    file1.close();
    file2.close();
    file3.close();

    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int build_file(){
    ofstream ofs;
    ofs.open("user_count2.txt");
    if(!ofs.is_open()){
        cout<<"Failed to open file.\n";
        return 1;
    }
    ofs<<user_amount<<"\n";
    ofs.close();
    return 0;
}

int upload_file(){
    const char* BUCKET = "0711524";
    const char* KEY ="user_count2.txt";
    const char* LOCAL_FILE = "user_count2.txt";
    Aws::String LOCAL_FILE_COPY(LOCAL_FILE);

    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace; //Turn on logging.

    Aws::InitAPI(options);
    {
        // snippet-start:[transfer-manager.cpp.transferOnStream.code]
        auto s3_client = Aws::MakeShared<Aws::S3::S3Client>("S3Client");
        auto executor = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("executor", 25);
        Aws::Transfer::TransferManagerConfiguration transfer_config(executor.get());
        transfer_config.s3Client = s3_client;

        auto transfer_manager = Aws::Transfer::TransferManager::Create(transfer_config);

        auto uploadHandle = transfer_manager->UploadFile(LOCAL_FILE, BUCKET, KEY, "text/plain", Aws::Map<Aws::String, Aws::String>());
        uploadHandle->WaitUntilFinished();
        bool success = uploadHandle->GetStatus() == Transfer::TransferStatus::COMPLETED; 
      
        if (!success)
        {
            auto err = uploadHandle->GetLastError();           
            std::cout << "File upload failed:  "<< err.GetMessage() << std::endl;
        }
        else
        {
            std::cout << "File upload finished." << std::endl;
            // Verify that the upload retrieved the expected amount of data.
            assert(uploadHandle->GetBytesTotalSize() == uploadHandle->GetBytesTransferred());
            g_file_size = uploadHandle->GetBytesTotalSize();
        }
    }
    Aws::ShutdownAPI(options);
}

int download_file(string filename){
    const char* BUCKET = "0711524";
    const char* KEY =filename.c_str();
    const char* LOCAL_FILE =filename.c_str();
    Aws::String LOCAL_FILE_COPY(LOCAL_FILE);

    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace; //Turn on logging.

    Aws::InitAPI(options);
    {
        // snippet-start:[transfer-manager.cpp.transferOnStream.code]
        auto s3_client = Aws::MakeShared<Aws::S3::S3Client>("S3Client");
        auto executor = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("executor", 25);
        Aws::Transfer::TransferManagerConfiguration transfer_config(executor.get());
        transfer_config.s3Client = s3_client;

        auto transfer_manager = Aws::Transfer::TransferManager::Create(transfer_config);

        Aws::Utils::Array<unsigned char> buffer(BUFFER_SIZE);
        auto downloadHandle = transfer_manager->DownloadFile(BUCKET,
            KEY,
            [&]() { //Define a lambda expression for the callback method parameter to stream back the data.
                return Aws::New<MyUnderlyingStream>("TestTag", Aws::New<Stream::PreallocatedStreamBuf>("TestTag", buffer.GetUnderlyingData(), BUFFER_SIZE));
            });
        downloadHandle->WaitUntilFinished();// Block calling thread until download is complete.
        auto downStat = downloadHandle->GetStatus();
        if (downStat != Transfer::TransferStatus::COMPLETED)
        {
            auto err = downloadHandle->GetLastError();
            std::cout << "File download failed:  " << err.GetMessage() << std::endl;
        }
        std::cout << "File download to memory finished."  << std::endl;
        // snippet-end:[transfer-manager.cpp.transferOnStream.code]
            
        // Verify the download retrieved the expected length of data.
        assert(downloadHandle->GetBytesTotalSize() == downloadHandle->GetBytesTransferred());

        // Verify that the length of the upload equals the download. 
        //assert(uploadHandle->GetBytesTotalSize() == downloadHandle->GetBytesTotalSize());

        // Write the buffered data to local file copy.
        Aws::OFStream storeFile(LOCAL_FILE_COPY.c_str(), Aws::OFStream::out | Aws::OFStream::trunc);
        storeFile.write((const char*)(buffer.GetUnderlyingData()), downloadHandle->GetBytesTransferred());
        storeFile.close();

        std::cout << "File dumped to local file finished. You can verify the two files' content using md5sum." << std::endl;
        // Verify the upload file is the same as the downloaded copy. This can be done using 'md5sum' or any other file compare tool.
    }
    Aws::ShutdownAPI(options);
}



int creat_public(string message, struct sockaddr_in client_addr, int sfd){
    string arg[4];
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
    if(arg_count!=4){
        send_back = "Usage: create public room <game room id>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
        return 0;
    }
    
    string user_name = current_user[sfd];
    map<int, struct room>::iterator iter;
    stringstream ss2;
    ss2<<arg[3];
    int id;
    ss2>>id;
    iter = room_d.find(id);
    if(user_name==""){
        //fail 1: not logged in
        send_back = "You are not logged in";
    }
    else if(user_d[user_name].room_id!=-1){
        //fail 2: already in room
        stringstream ss2;
        ss2<<user_d[user_name].room_id;
        string num;
        ss2>>num;
        send_back = "You are already in game room "+ num +", please leave game room";
    }
    else if(iter!=room_d.end()){
        //fail 3: room already used
        send_back = "Game room ID is used, choose another one";
    }
    else{
        //success
        send_back = "You create public game room " + arg[3];
        vector<int> v;
        struct room r;
        r.id = id;
        r.users_sfd = v;
        r.users_sfd.push_back(sfd);
        r.user_num++;
        r.type = 0;
        r.status = 1;
        r.host = user_name;
        room_d[id] = r;
        user_d[user_name].room_id = id;
    }
    send_back+="\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int creat_private(string message, struct sockaddr_in client_addr, int sfd){
    string arg[5];
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
    if(arg_count!=5){
        send_back = "Usage: create private room <game room id> <invitation code>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
        return 0;
    }

    string user_name = current_user[sfd];
    map<int, struct room>::iterator iter;
    stringstream ss2;
    ss2<<arg[3];
    int id;
    ss2>>id;
    iter = room_d.find(id);
    if(user_name==""){
        //fail 1: not logged in
        send_back = "You are not logged in";
    }
    else if(user_d[user_name].room_id!=-1){
        //fail 2: already in room
        stringstream ss2;
        ss2<<user_d[user_name].room_id;
        string num;
        ss2>>num;
        send_back = "You are already in game room "+ num +", please leave game room";
    }
    else if(iter!=room_d.end()){
        //fail 3: room already used
        send_back = "Game room ID is used, choose another one";
    }
    else{
        //success
        send_back = "You create private game room " + arg[3];
        vector<int> v;
        struct room r;
        stringstream ss2;
        ss2<<arg[4];
        uint32_t invite_code;
        ss2>>invite_code;
        r.id = id;
        r.users_sfd = v;
        r.users_sfd.push_back(sfd);
        r.user_num++;
        r.type = 1;
        r.code = invite_code;
        r.status = 1;
        r.host = user_name;
        room_d[id] = r;
        user_d[user_name].room_id = id;
    }
    send_back+="\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int list_room(string message, struct sockaddr_in client_addr, int sfd){
    int clen = sizeof(client_addr);
    string send_back="List Game Rooms\n";
    map<int, struct room>::iterator iter;
    int count=1;
    for(iter=room_d.begin(); iter!=room_d.end(); iter++){
        stringstream ss2;
        ss2<<count;
        string num;
        ss2>>num;
        send_back+=num;
        ss2.clear();
        int id = iter->first;
        struct room r = iter->second;
        ss2<<id;
        string id_s;
        ss2>>id_s;
        //publice or private
        if(r.type==0) send_back+=(". (Public) Game Room "+id_s);
        else send_back+=(". (Private) Game Room "+id_s);
        //open or start
        if(r.status==1) send_back+=" is open for players\n";
        else send_back+="has started playing\n";
        count++;
    }
    if(count==1){
        send_back+="No Rooms\n";
    }
    sendto(sfd, send_back.c_str(), strlen(send_back.c_str()), 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
    return 0;
}

int list_users(string message, struct sockaddr_in client_addr, int sfd){
    int clen = sizeof(client_addr);
    string send_back="List Users\n";
    map<string, struct user>::iterator iter;
    int count=1;
    for(iter=user_d.begin(); iter!=user_d.end(); iter++){
        stringstream ss2;
        ss2<<count;
        string num;
        ss2>>num;
        send_back+=num;
        struct user u = iter->second;
        send_back+=(". "+ u.name + "<"+u.email+">");
        if(u.login==0) send_back+=" Offline\n";
        else send_back+=" Online\n";
        count++;
    }
    if(count==1){
        send_back+="No Users\n";
    }
    sendto(sfd, send_back.c_str(), strlen(send_back.c_str()), 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
    return 0;
}

int list_invite(string message, struct sockaddr_in client_addr, int sfd){
    int clen = sizeof(client_addr);
    string send_back="List invitations\n";
    map<int, string> invite_d = user_d[current_user[sfd]].invite_m; 
    map<int, string>::iterator iter;
    int count=1;
    for(iter=invite_d.begin(); iter!=invite_d.end(); iter++){
        stringstream ss2;
        ss2<<count;
        string num;
        ss2>>num;
        send_back+=num;

        string name = iter->second;
        int rid = iter->first;
        stringstream ss3;
        ss3<<rid;
        string rids;
        ss3>>rids;
        ss3.clear();
        ss3<<room_d[rid].code;
        string codes;
        ss3>>codes;
        send_back+=(". "+ name + "<" + user_d[name].email + "> invite you to join game room "+rids+", invitation code is "+codes+"\n");
        count++;
    }
    if(count==1){
        send_back+="No Invitations\n";
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int accept(string message, struct sockaddr_in client_addr, int sfd){
    string arg[3];
    int clen = sizeof(client_addr);
    int arg_count=0;
    
    string send_back;
    stringstream ss;
    ss<<message;
    while(1){
        if(ss.fail()) break;
        string info;
        ss>>info;
        if(info=="") break;
        arg[arg_count]=info;
        arg_count++;
    }
    if(arg_count!=3){
        send_back = "Usage: accept <inviter email> <invitation code>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
        return 0;
    }
    ss.clear();

    string user_name = current_user[sfd];
    string mail = arg[1];
    string codes = arg[2];
    ss<<codes;
    uint32_t code;
    ss>>code;
    ss.clear();
    if(user_name==""){ //fail 1
        send_back = "You are not logged in\n";
    }
    else if(user_d[user_name].room_id!=-1){ //fail 2
        int id = user_d[user_name].room_id;
        ss<<id;
        string ids;
        ss>>ids;
        send_back = "You are already in game room "+ ids +", please leave game room\n";
    }
    else{
        string inviter=mail_d[mail];
        int invite_room=user_d[inviter].room_id;
        map<int, string> invite_list = user_d[user_name].invite_m;
        map<int, string>::iterator iter=invite_list.find(invite_room);
        if(iter==invite_list.end()){ //fail 3
            send_back = "Invitation not exist\n";
        }
        else if(room_d[invite_room].code!=code){ //fail 4
            send_back = "Your invitation code is incorrect\n";
        }
        else if(room_d[invite_room].status==2){ //fail 5
            send_back = "Game has started, you can't join now\n";
        }
        else{ //success
            //other
            vector<int> sfd_v = room_d[invite_room].users_sfd;
            for(int i=0; i<sfd_v.size(); i++){
                if(sfd_v[i]!=-1&&sfd_v[i]!=sfd){
                    send_back = "Welcome, "+user_name+" to game!\n";
                    send(sfd_v[i], send_back.c_str(), strlen(send_back.c_str()), 0);
                }
            }
            //user
            ss<<invite_room;
            string room_s;
            ss>>room_s;
            send_back = "You join game room "+room_s+"\n";
            user_d[user_name].room_id = invite_room;
            room_d[invite_room].users_sfd.push_back(sfd);
            room_d[invite_room].user_num++;
        }
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int leave(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;
    string user_name = current_user[sfd];
    
    if(user_name==""){ //fail 1
        send_back = "You are not logged in\n";
    }
    else if(user_d[user_name].room_id==-1){ //fail 2
        send_back = "You did not join any game room\n";
    }
    else{ //success
        int id = user_d[user_name].room_id;
        stringstream ss;
        ss<<id;
        string ids;
        ss>>ids;
        if(room_d[id].host==user_name){ //success 1
            struct room r=room_d[id];
            vector<int> sfd_v = r.users_sfd;
            //other
            int v_size = sfd_v.size();
            int i=0;
            while(1){
                if(v_size==i) break;
                if(sfd_v[i]!=-1&&sfd_v[i]!=sfd){
                    send_back = "Game room manager leave game room "+ ids +", you are forced to leave too\n";
                    string other_user = current_user[sfd_v[i]];
                    user_d[other_user].room_id = -1;
                    send(sfd_v[i], send_back.c_str(), strlen(send_back.c_str()), 0);
                }
                i++;
            }
            //host
            send_back = "You leave game room " + ids +"\n";
            user_d[user_name].room_id = -1;
            //delete room
            int n = room_d.erase(id);
            //delete invitation in all user
            for(map<string, struct user>::iterator iter=user_d.begin(); iter!=user_d.end(); iter++){
                int n = iter->second.invite_m.erase(id);
            }
        }
        else if(room_d[id].status==2){ //success 2
            struct room r=room_d[id];
            vector<int> sfd_v = r.users_sfd;
            room_d[id].status=1;
            //other
            int v_size = sfd_v.size();
            int i=0;
            while(1){
                if(v_size==i) break;
                if(sfd_v[i]!=-1&&sfd_v[i]!=sfd){
                    send_back = user_name + " leave game room "+ids+", game ends\n";
                    send(sfd_v[i], send_back.c_str(), strlen(send_back.c_str()), 0);
                }
                i++;
            }
            //user
            send_back = "You leave game room " + ids + ", game ends\n";
            user_d[user_name].room_id = -1;
            for(int i=0; i<sfd_v.size(); i++){
                if(sfd_v[i]==sfd){
                    room_d[id].users_sfd[i]=-1;
                    room_d[id].user_num--;
                }
            }
        }
        else{ //success 3
            struct room r=room_d[id];
            vector<int> sfd_v = r.users_sfd;
            //other
            int v_size = sfd_v.size();
            int i=0;
            while(1){
                if(v_size==i) break;
                if(sfd_v[i]!=-1&&sfd_v[i]!=sfd){
                    send_back = user_name + " leave game room "+ids+"\n";
                    send(sfd_v[i], send_back.c_str(), strlen(send_back.c_str()), 0);
                }
                i++;
            }
            //user
            send_back = "You leave game room " + ids + "\n";
            user_d[user_name].room_id = -1;
            for(int i=0; i<sfd_v.size(); i++){
                if(sfd_v[i]==sfd) {
                    room_d[id].users_sfd[i]=-1;
                    room_d[id].user_num--;
                }
            }
        }
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int join_room(string message, struct sockaddr_in client_addr, int sfd){
    string arg[3];
    int clen = sizeof(client_addr);
    int arg_count=0;
    
    string send_back="";
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
        send_back = "Usage: join room <game room id>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
        return 0;
    }

    string user_name = current_user[sfd];
    map<int, struct room>::iterator iter;
    stringstream ss2;
    ss2<<arg[2];
    int id;
    ss2>>id;
    iter = room_d.find(id);
    if(user_name==""){ //fail 1: not log in
        send_back = "You are not logged in";
    }
    else if(user_d[user_name].room_id != -1){ //fail 2: already in room
        stringstream ss2;
        ss2<<user_d[user_name].room_id;
        string num;
        ss2>>num;
        send_back = "You are already in game room "+ num +", please leave game room";
    }
    else if(iter==room_d.end()){ //fail 3: room doesn't exist
        send_back = "Game room "+arg[2]+" is not exist";
    }
    else if(iter->second.type==1){ //fail 4
        send_back = "Game room is private, please join game by invitation code";
    }
    else if(iter->second.status==2){ //fail 5
        send_back = "Game has started, you can't join now";
    }
    else{ //success
        //send to others in room
        struct room r=iter->second;
        vector<int> sfd_v = r.users_sfd;
        int v_size = sfd_v.size();
        int i=0;
        while(1){
            if(v_size==i) break;
            if(sfd_v[i]!=-1&&sfd_v[i]!=sfd){
                send_back = "Welcome, "+user_name+" to game!\n";
                send(sfd_v[i], send_back.c_str(), strlen(send_back.c_str()), 0);
            }
            i++;
        }
        //add to vector
        iter->second.users_sfd.push_back(sfd);
        iter->second.user_num++;
        //send to yourself
        user_d[user_name].room_id = id;
        send_back = "You join game room " + arg[2];
    }
    send_back+="\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int invite(string message, struct sockaddr_in client_addr, int sfd){
    string arg[2];
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
    if(arg_count!=2){
        send_back = "Usage: invite <invitee email>\n";
        send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
        return 0;
    }
    
    string user_name = current_user[sfd];
    string invitee_name = mail_d[arg[1]];
    if(user_name==""){ //fail 1: not log in
        send_back = "You are not logged in";
    }
    else if(user_d[user_name].room_id==-1){ //fail 2: hasn't joined room
        send_back = "You did not join any game room";
    }
    else if(user_name != room_d[user_d[user_name].room_id].host){ //fail 3: not room manager
        send_back = "You are not private game room manager";
    }
    else if(user_d[invitee_name].login==0){ //fail 4: invitee hasn't logged in
        //cout<<invitee_name<<endl;
        send_back = "Invitee not logged in";
    }
    else{ //success
        //send to invitee
        send_back = "You receive invitation from "+user_name+ "<"+user_d[user_name].email+">\n";
        int id = user_d[user_name].room_id;
        user_d[invitee_name].invite_m[id] = user_name;
        send(user_d[invitee_name].sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
        //send to inviter
        send_back = "You send invitation to "+invitee_name+"<"+user_d[invitee_name].email+">";
    }
    send_back+="\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
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
        if(info=="") break;
        arg_count++;
        if(arg_count==2){
            name = info;
            iter = user_d.find(info);
            if(iter==user_d.end()){
                //fail 1: can't found user
                send_back = "Username does not exist";
                flag=0;
            }
            else{
                if(user_d[info].login==1){
                    //fail 3: accout already login
                    send_back = "Someone already logged in as "+ user_d[info].name;
                    flag=0;
                }
            }
        }
        else if(arg_count==3&&flag){
            if(user_d[name].password!=info&&flag){
                //fail 4: password incorrect
                send_back = "Wrong password";
                flag = 0;
            }
        }
    }
    
    if(current_user[sfd]!=""&&flag){
        //fail 2: already logged in another account
        send_back = "You already logged in as "+current_user[sfd];
        flag = 0;
    }

    if(arg_count!=3){
        send_back = "Usage: login <username> <password>";
    }
    else if(flag){
        current_user[sfd] = name;
        send_back = "Welcome, "+current_user[sfd];
        user_d[current_user[sfd]].login = 1;
        user_d[current_user[sfd]].sfd = sfd;
        user_amount++;
        build_file();
        upload_file();
    }
    send_back+="\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int logout(string message, struct sockaddr_in client_addr, int sfd){
    string send_back;
    if(current_user[sfd]==""){
        //fail 1
        send_back = "You are not logged in";
    }
    else if(user_d[current_user[sfd]].room_id!=-1){
        //fail 2
        stringstream ss2;
        ss2<<user_d[current_user[sfd]].room_id;
        string num;
        ss2>>num;
        send_back = "You are already in game room "+ num +", please leave game room";
    }
    else{
        send_back = "Goodbye, "+current_user[sfd];
        user_d[current_user[sfd]].login = 0;
        user_d[current_user[sfd]].sfd = -1;
        current_user[sfd] = "";
        user_amount--;
        build_file();
        upload_file();
    }
    send_back+="\n";
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int start_game(string message, struct sockaddr_in client_addr, int sfd){
    string arg[4];
    int clen = sizeof(client_addr);
    int arg_count=0;
    
    arg[3] = "";
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
    
    string user_name = current_user[sfd];
    int id = user_d[user_name].room_id;;
    if(user_name==""){//fail 1: not log in
        send_back = "You are not logged in\n";
    }
    else if(user_d[user_name].room_id==-1){//fail 2: not in room
        send_back = "You did not join any game room\n";
    }
    else if(room_d[id].host!=user_name){ //fail 3:
        send_back = "You are not game room manager, you can't start game\n";
    }
    else if(room_d[id].status==2){ //fail 4:
        send_back = "Game has started, you can't start again\n";
    }
    else if(arg_count==4&&(!is_num(arg[3])||arg[3].length()!=4)){ //fail 5
        send_back = "Please enter 4 digit number with leading zero\n";
    }
    else{
        //success
        struct room r = room_d[id];
        for(int i=0; i<r.users_sfd.size(); i++){
            if(r.users_sfd[i]!=-1){
                send_back = "Game start! Current player is " + current_user[r.users_sfd[0]]+"\n";
                send(r.users_sfd[i], send_back.c_str(), strlen(send_back.c_str()), 0);
            }
        }
        //room_d[id].turn++;
        //change status
        room_d[id].status = 2;
        //round
        stringstream ss(arg[2]);
        int round;
        ss>>round;
        room_d[id].round = round;
        ss.clear();
        //answer
        if(arg[3]==""){
            unsigned seed = (unsigned)time(NULL);
            srand(seed);
            int answer = rand()% 8889 + 1111;
            stringstream ss;
            string ans_s;
            ss<<answer;
            ss>>ans_s;
            room_d[id].answer = ans_s;
        }
        else room_d[id].answer = arg[3];
        return 0;
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int guess(string message, struct sockaddr_in client_addr, int sfd){
    string arg[2];
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

    string guess=arg[1];
    string user_name = current_user[sfd];
    int room_id = user_d[user_name].room_id;
    stringstream ss1;
    if(user_name==""){ //fail 1
        send_back = "You are not logged in\n";
    }
    else if(room_id==-1){ //fail 2
        send_back = "You did not join any game room\n";
    }
    else if(room_d[room_id].status==1){ //fail 3
        if(room_d[room_id].host==user_name) send_back = "You are game room manager, please start game first\n";
        else send_back = "Game has not started yet\n";
    }
    else if(user_d[user_name].sfd!=room_d[room_id].users_sfd[room_d[room_id].turn]){ //fail 4
        send_back = "Please wait..., current player is "+current_user[room_d[room_id].users_sfd[room_d[room_id].turn]]+"\n";
    }
    else if(!is_num(guess)||guess.length()!=4){ //fail 5
        send_back = "Please enter 4 digit number with leading zero\n";
    }
    else{
        //guess
        int a=0;
        int b=0;
        string ans=room_d[room_id].answer;
        int opened_ans[4]={1, 1, 1, 1};
        int opened_guess[4]={1, 1, 1, 1};
        for(int i=0; i<4; i++){
            if(guess[i]==ans[i]){
                a++;
                opened_ans[i]=0;
                opened_guess[i]=0;
            }
        }
        for(int i=0; i<4; i++){
            for(int j=0; j<4; j++){
                if(guess[i]==ans[j]&&opened_ans[j]&&opened_guess[i]){
                    b++;
                    opened_ans[j]=0;
                    opened_guess[i]=0;
                    break;
                }
            }
        }
        stringstream ss, ss2;
        string a_s, b_s;
        ss<<a;
        ss>>a_s;
        ss2<<b;
        ss2>>b_s;
        vector<int> sfd_v = room_d[room_id].users_sfd;
        string current_player = current_user[room_d[room_id].users_sfd[room_d[room_id].turn]];
        if(a==4&&b==0){ //success: bingo
            send_back = current_player+" guess '"+guess+"' and got Bingo!!! "+current_player+" wins the game, game ends\n";
            room_d[room_id].status=1;
        }
        else{
            send_back = current_player+" guess '"+guess+"' and got '" +a_s+"A"+b_s+"B'";
            room_d[room_id].user_count++;
            //next person
            room_d[room_id].turn++;
            while(room_d[room_id].users_sfd[room_d[room_id].turn]==-1&&room_d[room_id].turn!=room_d[room_id].users_sfd.size()){
                room_d[room_id].turn++;
            }
            //next round
            if(room_d[room_id].user_count==room_d[room_id].user_num){
                room_d[room_id].round--;
                room_d[room_id].user_count=0;
                room_d[room_id].turn=0;
                while(room_d[room_id].users_sfd[room_d[room_id].turn]==-1&&room_d[room_id].turn!=room_d[room_id].users_sfd.size()){
                    room_d[room_id].turn++;
                }
            }

            if(room_d[room_id].round==0){ //success: No chances
                send_back += "\nGame ends, no one wins\n";
                room_d[room_id].status=1;
            }
            else{ //success: not bingo, keep guessing
                send_back += "\n";
            }
        }
        for(int i=0; i<sfd_v.size(); i++){
            if(sfd_v[i]!=-1) send(sfd_v[i], send_back.c_str(), strlen(send_back.c_str()), 0);
        }
        return 0;
    }
    send(sfd, send_back.c_str(), strlen(send_back.c_str()), 0);
    return 0;
}

int exit(string message, struct sockaddr_in client_addr, int sfd){
    string user_name = current_user[sfd];
    if(user_name!=""){
        user_d[user_name].login = 0;
        current_user[sfd] = "";
        if(user_d[user_name].room_id!=-1){ 
            int id = user_d[user_name].room_id;
            user_d[user_name].room_id = -1;
            stringstream ss;
            ss<<id;
            string ids;
            ss>>ids;
            if(room_d[id].host==user_name){ //success 1: is room manager
                struct room r=room_d[id];
                vector<int> sfd_v = r.users_sfd;
                //other
                int v_size = sfd_v.size();
                int i=0;
                while(1){
                    if(v_size==i) break;
                    if(sfd_v[i]!=-1&&sfd_v[i]!=sfd){
                        string other_user = current_user[sfd_v[i]];
                        user_d[other_user].room_id = -1;
                    }
                    i++;
                }
                //delete room
                int n = room_d.erase(id);
                //delete invitation in all user
                for(map<string, struct user>::iterator iter=user_d.begin(); iter!=user_d.end(); iter++){
                    int n = iter->second.invite_m.erase(id);
                }
            }
            else if(room_d[id].status==2){ //success 2: game started
                struct room r=room_d[id];
                vector<int> sfd_v = r.users_sfd;
                room_d[id].status=1;
                //other
                //user
                for(int i=0; i<sfd_v.size(); i++){
                    if(sfd_v[i]==sfd){
                        room_d[id].users_sfd[i]=-1;
                        room_d[id].user_num--;
                    }
                }
            }
            else{ //success 3
                struct room r=room_d[id];
                vector<int> sfd_v = r.users_sfd;
                //other
                //user
                for(int i=0; i<sfd_v.size(); i++){
                    if(sfd_v[i]==sfd) {
                        room_d[id].users_sfd[i]=-1;
                        room_d[id].user_num--;
                    }
                }
            }
        }
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
        if(info=="") break;
        arg_count++;
        if(arg_count==2){
            //name
            iter = user_d.find(info);
            if(iter==user_d.end()){
                p.name = info;
            }
            else{
                //fail 1
                send_back = "Username or Email is already used";
                flag=0;
            }
        }
        else if(arg_count==3){
            //email
            iter_v = find(mail_v.begin(), mail_v.end(), info);
            if(iter_v==mail_v.end()){
                p.email = info;
            }
            else{
                //fail 2
                send_back = "Username or Email is already used";
                flag=0;
            }
        }
        else if(arg_count==4){
            //password
            p.password=info;
        }
    }
    if(arg_count!=4){
        //command format
        send_back = "Usage: register <username> <email> <password>";
    }
    else if(flag){
        map<int, string> m;
        p.login=0;
        p.room_id=-1;
        p.sfd = udp_socketfd;
        p.invite_m = m;
        mail_v.push_back(p.email);
        mail_d[p.email] = p.name;
        user_d[p.name] = p;
        send_back = "Register Successfully";
    }
    send_back+="\n";
    sendto(udp_socketfd, send_back.c_str(), strlen(send_back.c_str()), 0, (struct sockaddr*) &client_addr, (socklen_t) clen);
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


