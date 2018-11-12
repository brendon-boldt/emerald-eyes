#pragma once

vector<string> readfile(char [],unsigned int);
int authorize();
int cstrtoint(char*);
int clientsock();
int Send(SOCKET,char[],int,int);
int Recv(SOCKET,char[],int,int);
int recvfile();
int sendfile(char[]);
void closeclient();
int sendcommand();
