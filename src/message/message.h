#include <string>
using namespace std;

struct Message {
    string type;
    int senderId;
};

Message parseMessage(string &input, int senderId);