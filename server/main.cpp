#include "includes.h"

// Server 2.0

const char keychar = '#';
unsigned int serverstate = 0;

int main(int argc, char *argv[])
{
    unsigned int serverstate = 0;
    serverstate = (unsigned int)serversock();

    while (true)
    {
        serverstate = lstnacpt();
        while(serverstate == 0)
        {
            serverstate = recvcommand();
        }
        printf("\nClient disconnected.\n\n\n");
    }

    closeserver();

    return 0;
}
