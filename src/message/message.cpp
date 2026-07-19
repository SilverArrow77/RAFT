#pragma once
#include "message.h"

using namespace std;

Message parseMessage(string &input, int senderId){
    Message message;
    if(input == "HEARTBEAT"){
        message.type = "Heartbeat";
    }
    else{
        message.type = "Unknown";
    }
    message.senderId = senderId;
    return message;
}