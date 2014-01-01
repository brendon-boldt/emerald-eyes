#include "includes.h"

static WSADATA WSAData;
static char    serverip[32];
static char    serverport[8];
static char    pswrd[32];
static const int bufsize         = 1000;
static const int headsize        = 3;
const static unsigned long mode  = 2;
static const int namesize        = 50; // 'filenamesize'
static const int cmdsize         = 50;
static const int cnslsize        = 8192;
static int clientsockfd; // client socket file descriptor
static struct addrinfo hints, *res;

// Pulls data from an "=" .ini file
// Nota bene: the order of values matters, not the labels
vector<string> readfile(char file[], unsigned int count)
{
    vector<string> strres (3);
    ifstream reader(file,ios_base::in);
    string str;
    char x[32];

    unsigned int i,j,k;
    bool read = false;
    for(i=0;i<count;i++)
    {
        memset(x,0,sizeof(x));
        k = 0;
        read = false;
        getline(reader,str);
        for (j=0;j<str.length();j++)
        {
            if (read == true)
            {
                 x[k] = str[j];
                 k++;
            }
            if (str[j] == '=') {read = true;}
        }
        strres[i] = x;
    }
    return strres;
}

int authorize()
{
    char result[1];
    memset(result,0,sizeof(result));
    Send(clientsockfd,pswrd,cmdsize,0);
    if (Recv(clientsockfd,result,sizeof(result),MSG_WAITALL) == -1)
        {return 1;}
    if (result[0] == 0)
    {
        printf("%s\n","Authorization successful.");
    }
    else
    {
        printf("%s\n","Authorization failed.");
    }
    printf("\n");

    return 0;
}

// C-string to integer
int cstrtoint (char* a)
{
    unsigned int x = 0;
    x = (unsigned char)a[2]*256*256+(unsigned char)a[1]*256+(unsigned char)a[0];
    return x;
}

int clientsock()
{
    vector<string> dat (3);
    dat = readfile("config.ini",3);
    strcpy(serverip  ,dat[0].c_str());
    strcpy(serverport,dat[1].c_str());
    strcpy(pswrd     ,dat[2].c_str());

    if (WSAStartup(MAKEWORD(2,2),&WSAData))
    {
        printf("WSA startup failed.\n\n");
        return 1;
    }

    // Importanat socket type values
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if (getaddrinfo(serverip,serverport,&hints,&res) != 0)
    {
        printf("Failed to get server address information.\n\n");
        return 1;
    }

    //struct sockaddr_in *serverip = (struct sockaddr_in *)res->ai_addr;
    //printf("Host IP Address: %s\n\n", inet_ntoa(res->sin_addr));

    if ((clientsockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) == -1)
    {
        printf("Client socket creation failed.\n\n");
        return 1;
    }

    printf("Connecting to server...\n\n");

    if (connect(clientsockfd,res->ai_addr,res->ai_addrlen) != -1)
    {
        printf("Connected to server.\n\n");
    }
    else
    {
        printf("Unable to connect to server.\n\n");
        return 1;
    }
    return 0;
}

// "Send" (vs. "send") assures that all of the data is sent
int Send(SOCKET a, char b[], int c, int d)
{
    int sent=0, total=0;
    while (total<c)
    {
        sent = send(a,total+b,c-total,d);
        total+=sent;
        if (sent < 0) {return sent;}
    }
    return total;
}

// Assures all data is received
int Recv(SOCKET a, char b[], int c, int d)
{
    int recvd=0, total=0;
    while (total<c)
    {
        recvd = recv(a,total+b,c-total,d);
        total+=recvd;
        if (recvd < 0) {return recvd;}
    }
    return total;
}

int recvfile()
{
    char filename[namesize];
    memset(filename,0,sizeof(filename));
    Recv(clientsockfd,filename,namesize,MSG_WAITALL);

    // This will make the client socket a non-blocking socket for mode = 1.
    ///ioctlsocket(clientsockfd,FIONBIO,&mode);

    // Header indicates file size
    char header[headsize],nullarr[headsize];
    memset(nullarr,0,sizeof(nullarr));
    Recv(clientsockfd,header,headsize,MSG_WAITALL);
    // Non-existent files are transmitted as size 0
    if (!strcmp(header,nullarr)) // strcmp returns 0 for equal strings
    {
        printf("%s\n","File does not exist.\n\n");
    }
    else
    {
        ofstream trawrite(filename,ios_base::out|ios_base::binary);
        printf("Receiving: `%s'...",filename);
        unsigned int length = cstrtoint(header);

        char buf[bufsize];
        unsigned int i = 0;
        unsigned int bytesrecv = 1;
        unsigned int bytesrecvtotal=0;
        while (bytesrecvtotal<length)
        {
            memset(buf,0,sizeof(buf));
            bytesrecv = Recv(clientsockfd,buf,bufsize,0);
            trawrite.write(buf,bytesrecv);
            bytesrecvtotal+=bytesrecv;
            i++;
            if (!(i%100)) {printf("%.0f%% Complete\r",(100*(bytesrecvtotal))/(float)(length));}
        }
        printf("100%% Complete\n\n");
        trawrite.close();
        printf("File transmission complete...\n\n");
    }

    return 0;
}

int sendfile(char filename[namesize])
{
    Send(clientsockfd,filename,namesize,0);

    // Transmaission reader
    ifstream traread (filename,ios_base::in |ios_base::binary);
    traread.seekg(0,traread.end);
    char header[headsize];
    memset(header,0,sizeof(header));
    if (!traread.fail())
    {
        printf("Sending `%s'...\n\n",filename);
        unsigned int length = traread.tellg();
        // A base 256 header (char array) to integer
        {
            unsigned int x = length;
            header[2] = x/(256*256);
            x-=(int(header[2])*256*256);
            header[1] = x/256;
            x-=(int(header[1])*256);
            header[0] = x;
        }
        Send(clientsockfd,header,headsize,0);
        traread.seekg(0);

        int bytessent = 0;
        unsigned int bytessenttotal = 0;
        char tra[bufsize];
        unsigned long int i;
        while (bytessenttotal<length)
        {
            memset(tra,0,sizeof(tra));
            traread.read(tra,bufsize);
            bytessent = Send(clientsockfd,tra,bufsize,0);
            traread.seekg(bytessent-bufsize,traread.cur);
            bytessenttotal+=bytessent;
            i++;
            if (!(i%100)) {printf("%.0f%% Complete\r",(100*(bytessenttotal))/(float)(length));}
        }
        printf("100%% Complete\n\n");
        printf("File transmission complete...\n\n");
    }
    else
    {
        Send(clientsockfd,header,headsize,0);
        printf("%s\n","File does not exist.\n\n");
    }
    traread.close();
    return 0;
}

void closeclient()
{
    freeaddrinfo(res);
    shutdown(clientsockfd,2);
    WSACleanup();
}

int sendcommand()
{
    printf(">>");
    char cmd[cmdsize];
    memset(cmd,0,sizeof(cmd));
    while (strlen(cmd) == 0) {cin.getline(cmd,cmdsize);}
    if  (Send(clientsockfd,cmd,cmdsize,0) == -1) {return 1;}
    if (strchr(cmd,(unsigned int)keychar) == cmd)
    {
        switch ((unsigned int)*(cmd+0x1))
        {
            case 'd':
                recvfile();
                break;
            case 'u':
                sendfile(cmd+0x3);
                break;
            case 'c':
                break;
            case 'e':
                return 2;
            default:
                printf("Command sequence not recognized.\n");
                break;
        }
    }
    else
    {
        char cnsl[cnslsize];
        memset(cnsl,0,sizeof(cnsl));
        Recv(clientsockfd,cnsl,cnslsize,MSG_WAITALL);
        printf("%s",cnsl);
    }
    return 0;
}
