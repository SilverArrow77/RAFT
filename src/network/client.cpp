#include <iostream>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

void readBuffer(int sock){
    char buffer[1024];
    int bytes = read(sock, buffer, sizeof(buffer));
    if(bytes > 0){
        buffer[bytes] = '\0';
        cout << buffer << endl;
    }
    else {
        cout << bytes << endl;
    }
}

void sendData(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in clientAddr = {0};
    clientAddr.sin_port = htons(5000);
    clientAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &clientAddr.sin_addr);
    int res = connect(sock, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    const char *msg = "SET name a";
    write(sock, msg, strlen(msg));
    cout << "Works" << endl;
    readBuffer(sock);
    
}

// int main(){
//     cout << "Works" << endl;
//     sendData();
//     return 0;
// }