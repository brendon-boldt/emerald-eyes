#include "includes.h"

static struct       addrinfo hints, *res;
static struct       sockaddr_storage clientstorage;
static struct       sockaddr_in *serverip;
static int          serversockfd; // Socket file descriptor
static int          connectionfd; // Likewise
static char         port[8];
static char         pswrd[32];
static WSADATA      WSAData;
static const int    backlog    = 5;
static const int    bufsize    = 1000;
static const int    headsize   = 3;
static const int    namesize   = 50;
static const int    cmdsize    = 50;
static const int    cnslsize   = 8192;


// Pulls data from an "=" .ini file
// Nota bene: the order of values matters, not the labels
vector<string> readfile(char file[], unsigned int count)
{
    vector<string> res (count);
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
        res[i] = x;
    }
    return res;
}

int authorize()
{

    {
        printf("Authorizing request...\n\n");
        char cauth[cmdsize];
        char result[1];
        memset(cauth,0,sizeof(cauth));
        Recv(connectionfd,cauth,cmdsize,MSG_WAITALL);
        if (strcmp(cauth,pswrd) == false)
        {
            result[0] = 0;
            Send(connectionfd,result,sizeof(result),0);
            return 0;
        }
        else
        {
            result[0] = 1;
            Send(connectionfd,result,sizeof(result),0);
        }
    }
    return 1;
}

// C-string to integer
unsigned int cstrtoint (char* a)
{
    unsigned int x = 0;
    x = (unsigned char)a[2]*256*256+(unsigned char)a[1]*256+(unsigned char)a[0];
    return x;
}

int serversock()
{
    vector<string> dat (2);
    dat   = readfile("config.ini",2);
    strcpy(port ,dat[0].c_str());
    strcpy(pswrd,dat[1].c_str());

    if (WSAStartup(MAKEWORD(2,2),&WSAData))
    {
        printf("WSA startup failed.\n\n");
        getchar();
       exit(1);
    }

    // Important socket type values
    memset(&hints, 0, sizeof hints);
    hints.ai_family =   AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags =    AI_PASSIVE;

    if (getaddrinfo(NULL,port,&hints,&res) != 0)
    {
        printf("Failed to get server address information.\n\n");
        getchar();
       exit(2);
    }

    serverip = (struct sockaddr_in *)res->ai_addr;

    //printf("IP Address: %s\n\n", inet_ntoa(serverip->sin_addr));

    if ((serversockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) == -1)
    {
        printf("Server socket creation failed.\n\n");
        getchar();
       exit(3);
    }

    if (bind(serversockfd,res->ai_addr,res->ai_addrlen) == -1)
    {
        printf("Server socket binding failed.\n\n");
        getchar();
       exit(4);
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

int lstnacpt()
{
    if (((listen(serversockfd,backlog))) != -1)
    {
        printf("Awaiting client connection...\n\n");
    }
    else
    {
        printf("Server socket unable to receive incoming request(s).\n\n");
        getchar();
        return 1;
    }


    int addrsize = sizeof clientstorage;
    if ((connectionfd = accept(serversockfd,(struct sockaddr *)&clientstorage, &addrsize)) != -1)
    {
        printf("Client request accpeted.\n\n");
    }
    else
    {
        printf("Unable to accept client request.\n\n");
        getchar();
        return 2;
    }

    if (authorize() != 0)
    {
        closeclient();
        return 3;
    }
    else
    {
        printf("Client authorized.\n\n");
    }

    return 0;
}

int sendfile(char filename[namesize])
{
    Send(connectionfd,filename,namesize,0);

    // Transmission reader
    ifstream traread (filename,ios_base::in |ios_base::binary);
    char header[headsize];
    memset(header,0,sizeof(header));
    if(!traread.fail())
    {
        printf("Sending `%s'...\n\n",filename);
        traread.seekg(0,traread.end);
        unsigned int length = traread.tellg();
         // A base 256 header (char array) to integer
        {
            unsigned int x = length;
            header[2] = x/(256*256);
            x-=((unsigned int)header[2]*256*256);
            header[1] = x/256;
            x-=((unsigned int)header[1]*256);
            header[0] = x;
        }

        Send(connectionfd,header,headsize,0);
        traread.seekg(0);

        int bytessent = 0;
        unsigned int bytessenttotal = 0;
        char tra[bufsize];
        while (bytessenttotal<length)
        {
            memset(tra,0,sizeof(tra));
            traread.read(tra,bufsize);
            bytessent = Send(connectionfd,tra,bufsize,0);
            traread.seekg(bytessent-bufsize,traread.cur);
            bytessenttotal+=bytessent;
        }

        printf("File transmission complete...\n\n");
    }
    else
    {
        Send(connectionfd,header,headsize,0);
        printf("%s\n","File does not exist.\n\n");
    }

    traread.close();
    return 0;
}

int recvfile()
{
    char filename[namesize];
    memset(filename,0,sizeof(filename));
    Recv(connectionfd,filename,namesize,MSG_WAITALL);

    // This will make the client socket a non-blocking socket (if mode = 1).
    // ioctlsocket(connectionfd,FIONBIO,&mode);

    char header[headsize],nullarr[headsize];
    memset(nullarr,0,sizeof(nullarr));
    Recv(connectionfd,header,headsize,MSG_WAITALL);
    if (!strcmp(header,nullarr))
    {
        printf("%s\n","File does not exist.\n\n");
    }
    else
    {
        ofstream trawrite(filename,ios_base::out|ios_base::binary);
        printf("Receiving `%s'...\n\n",filename);
        unsigned int length = cstrtoint(header);

        char buf[bufsize];
        unsigned int bytesrecv = 0;
        unsigned int bytesrecvtotal = 0;
        while (bytesrecvtotal<length)
        {
            memset(buf,0,sizeof(buf));
            bytesrecv = Recv(connectionfd,buf,bufsize,0);
            trawrite.write(buf,bytesrecv);
            bytesrecvtotal+=bytesrecv;
        }

        trawrite.close();
        printf("File transmission complete...\n\n");
    }

    return 0;
}

void closeserver()
{
    shutdown(connectionfd,SD_BOTH); // I had to custom define SD_BOTH = 2 in WinDef.h
    shutdown(serversockfd,SD_BOTH);
    freeaddrinfo(res);
    WSACleanup();
}

void closeclient()
{
    // I had to custom define SD_BOTH = 2 in WinDef.h
    shutdown(connectionfd,SD_BOTH);
}

int recvcommand()
{
    printf("%s",">>");
    char cmd[cmdsize];
    memset(cmd,0,sizeof(cmd));
    int status = 0;
    while (status == 0)
    {
        status = Recv(connectionfd,cmd,cmdsize,0);
        if (status == -1) {return 1;}
    }
    printf("%s\n",cmd);
    if (strchr(cmd,(unsigned int)keychar) == cmd)
    {
        switch ((unsigned int)*(cmd+0x1))
        {
            case 'd':
                sendfile(cmd+0x3);
                break;
            case 'u':
                recvfile();
                break;
            case 'c':
                SetCurrentDirectory(cmd+0x3);
                break;
            case 'e':
                return 1;
                break;
            default:
                printf("Command sequence not recognized.\n");
                break;
        }
        printf("\n");
    }
    else
    {
        FILE * f = popen(cmd, "rt");
        char temp[128];
        char cnsl[cnslsize];
        memset(cnsl,0,cnslsize);
        do
        {
            fgets(temp, sizeof(temp), f);
            if (feof(f)) {break;}
            strncat(cnsl,temp,sizeof(temp));
        } while (!(feof(f)));
        printf(cnsl);
        if (Send(connectionfd,cnsl,cnslsize,0) == -1) {return 1;}
    }
    return 0;
}


