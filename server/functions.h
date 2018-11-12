#pragma once

vector<string> readfile(char [],unsigned int);
int authorize();
unsigned int cstrtoint(char*);
int serversock();
int Send(SOCKET,char[],int,int);
int Recv(SOCKET,char[],int,int);
int lstnacpt();
int sendfile(char[]);
int recvfile();
void closeserver();
void closeclient();
int rsconsole();
int recvcommand();
