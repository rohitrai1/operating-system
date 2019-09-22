#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#define MAXRECORDSIZE 200

using namespace std;

void *handleTransaction(void *);

int account_id[100];
string name[100];
int account_balance[100];
string inputFileRecords;
int fileCount = 0;
char op[2];
int accountNo;
int transacAmout;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
int main(int argc, char **argv)
{
    ifstream theFile("Records.txt");
    // Reading the Records.txt and setting global variables

    if (theFile.fail())
    {
        cerr << "Error reading the record file" << endl;
        exit(1);
    }

    int i = 0;
    while (theFile >> account_id[i] >> name[i] >> account_balance[i])
    {
        cout << account_id[i] << name[i] << account_balance[i] << endl;
        i++;
    }
    fileCount = i;
    // create a socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET since considering ipv4 protocol for the project
    if (tcp_socket == -1)
    {
        perror("connection");
    }
    cout << "==here2===" << endl;
    // bind the socket to IP and port
    sockaddr_in sock_detail;
    cout << "==here2.1===" << endl;
    sock_detail.sin_family = AF_INET;
    cout << "==here2.2===" << endl;
    sock_detail.sin_port = htons(54002);
    cout << "==here2.3===" << endl;                       // little endian to network byte ordering since using TCP/IP
    inet_pton(AF_INET, "0.0.0.0", &sock_detail.sin_addr); //character string src to a network address strcuture
    cout << "==here2.4===" << endl;
    if (bind(tcp_socket, (struct sockaddr *)&sock_detail, sizeof(sockaddr_in)))
    {
        cout << "Unable to bind the socket to IP and Port" << endl;
    }
    cout << "==here==3=";
    // Mark the socket for listening in
    if (listen(tcp_socket, SOMAXCONN) == -1)
    {
        perror("listen");
    }
    cout << "==here===4";
    // Accept calls from multiple clients
    sockaddr_in client;
    socklen_t clientSize;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    int clientSocket;
    cout << "==here===5" << endl;
    pthread_mutex_init(&mutex2, NULL);
    while (true)
    {
        cout << "=======6=here==========" << endl;
        clientSocket = accept(tcp_socket, (sockaddr *)&client, &clientSize);
        cout << "accepted client: " << clientSocket << endl;
        if (clientSocket == -1)
        {
            perror("socket");
        }
        else
        {
            cout << "A Connection has been established" << endl;
        }

        pthread_t tid;
        //pthread_attr_t attr;
        //pthread_attr_init(&attr);
        cout << "=======7=here==========" << endl;
        // spawn a new thread for each conncetion
        if (pthread_create(&tid, NULL, &handleTransaction, &clientSocket) < 0)
        {
            perror("create thread error");
            cout << "can't spawn a thread" << endl;
        }
    }

    // close socket
    close(clientSocket);
    // close the listening calls
    // close(tcp_socket);
    return 0;
}

// handleTransaction will get invoked for all the customers with a new thread id
// will handle transaction for each entry in transaction file
void *handleTransaction(void *clientSocket)
{
    cout << " thread called" << endl;
    int newSocket = *((int *)clientSocket);
    char *clientData = (char *)malloc(256);
    //bzero(clientData, 256);
    int n = 0;
    while (n = read(newSocket, clientData, 256))
    {
        if (n < 0)
        {
            cout << "exit " << endl;
            exit(1);
        }

        cout << "Data Received:" << clientData << endl;

        //Spliting timestamp, account-no, name of customer, account-balance;

        // char transactionType;

        /* get the first token */
        char *token;
        token = strtok(clientData, " ");
        if (token)
        {
            accountNo = atoi(token);

            token = strtok(NULL, " ");
            strcpy(op, token);
            cout << "code==>" << token << endl;

            token = strtok(NULL, " ");
            transacAmout = atoi(token);
        }
        cout << "here op code" << op << endl;
        for (int i = 0; i < fileCount; i++)
        { // check if the account id exists in the db
            if (account_id[i] == accountNo)
            {
                pthread_mutex_init(&mutex2, NULL);
                cout << "match is there " << endl;
                cout << "here1" << endl;
                if (strcmp(op, "w") == 0)
                {
                    if ((account_balance[i] - transacAmout) > 0)
                    {
                        account_balance[i] = account_balance[i] - transacAmout;

                        ifstream inFile("Records.txt");
                        ofstream outFile("Records1.txt");

                        string strTemp;
                        string strTemp1;
                        string strTemp2;
                        string newx;
                        string replaceStr;
                        while (inFile >> strTemp >> strTemp1 >> strTemp2)
                        {
                            cout << "strTemp" << strTemp << endl;
                            if (strTemp == to_string(accountNo))
                            {
                                newx = to_string(account_id[i]) + " " + name[i] + " " + to_string(account_balance[i]) + "\n";
                                outFile << newx;
                            }
                            else
                            {
                                cout << "else part" << strTemp << endl;
                                outFile
                                    << strTemp << " " << strTemp1 << " " << strTemp2 << endl;
                            }

                            rename("Records1.txt", "Records.txt");
                        }
                    }
                    else
                    {
                        //balance insufficient to perform the withdrawal

                        // n = write(newSocket, "NACK: Insufficient balance\n", 50);
                    }
                }
                else if (strcmp(op, "d") == 0)
                {
                    account_balance[i] = account_balance[i] + transacAmout;
                    ifstream inFile("Records.txt");
                    ofstream outFile("Records1.txt");

                    string strTemp;
                    string strTemp1;
                    string strTemp2;
                    string newx;
                    string replaceStr;
                    while (inFile >> strTemp >> strTemp1 >> strTemp2)
                    {
                        cout << "strTemp" << strTemp << endl;
                        if (strTemp == to_string(accountNo))
                        {
                            newx = to_string(account_id[i]) + " " + name[i] + " " + to_string(account_balance[i]) + "\n";
                            outFile << newx;
                        }
                        else
                        {
                            cout << "else part" << strTemp << endl;
                            outFile
                                << strTemp << " " << strTemp1 << " " << strTemp2 << endl;
                        }

                        rename("Records1.txt", "Records.txt");
                    }
                }
                //release the lock
                pthread_mutex_unlock(&mutex2);
            }
        }
    }
}
