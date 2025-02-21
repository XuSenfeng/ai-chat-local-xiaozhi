#ifndef CHAT_AI_H
#define CHAT_AI_H
#include "display.h"

typedef struct {
    const char *text;
    Display* display;
}message_t;

class Chat_ai {
public:
    Chat_ai();
    ~Chat_ai();
    void Chat_ai_Comunicate(message_t *msg);
    int voice;
private:
    // int user_id;
};



#define MAX_CHAT_BUFFER (2048)


#endif