#include "includes.h"

// Client 2.0

// No arguments for main() currently in use
int main(int argc, char *argv[])
{
    while (clientsock() != 0)
    {
        printf("Reattempting connection...\n\n\n");
    }

    int state = authorize();

    while (!state)
    {
        state = sendcommand();
    }

    switch (state)
    {
        case 1:
            printf("Server has closed.\n\n");
            break;
        case 2:
            printf("Successfully disconnected from server.\n\n");
            break;
        default:
            printf("An error occured.\n\n");
            break;
    }

    closeclient();
    getchar(); // Pause the program before it closes

    return 0;
}
