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
#define MAXRECORDSIZE 200;
#include <chrono>

using namespace std::chrono;
using namespace std;

void *handleTransaction(void *);

/*************************************************************/
/* Global Variable declaration to store records in server    */
/* server state after boot up                                */
/*************************************************************/

char op[2];

int account_balance[100];
int account_id[100];
int accountNo;
int fileCount = 0;
int transacAmout;

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

string inputFileRecords;
string name[100];

int main(int argc, char **argv)
{
    /*************************************************************/
    /* Read the records file and store it to the server state    */
    /*************************************************************/
    ifstream theFile("Records.txt");

    if (theFile.fail())
    {
        cerr << "Error reading the record file" << endl;
        exit(1);
    }

    int i = 0;
    cout << "**************Records (Database)*************" << endl;
    while (theFile >> account_id[i] >> name[i] >> account_balance[i])
    {
        cout << account_id[i] << " " << name[i] << " " << account_balance[i] << endl;
        i++;
    }
    cout << "*********************************************" << endl;
    fileCount = i;

    /*************************************************************/
    /* Create a AF_INET IPv4 stream socket to receive incoming   */
    /* requests                                                  */
    /*************************************************************/
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1)
    {
        perror("connection");
    }

    /*************************************************************/
    /* Bind the socket                                           */
    /*************************************************************/
    int clientSocket;
    sockaddr_in sock_detail;
    sock_detail.sin_family = AF_INET;
    sock_detail.sin_port = htons(54002);
    sockaddr_in client;
    socklen_t clientSize;

    inet_pton(AF_INET, "0.0.0.0", &sock_detail.sin_addr); //character string src to a network address strcuture
    if (bind(tcp_socket, (struct sockaddr *)&sock_detail, sizeof(sockaddr_in)))
    {
        cout << "Unable to bind the socket to IP and Port" << endl;
        perror("unable to bind");
    }

    /*************************************************************/
    /* Mark the socket to listen back log for MAX connections    */
    /*************************************************************/
    if (listen(tcp_socket, SOMAXCONN) == -1)
    {
        perror("listen");
    }

    pthread_mutex_init(&mutex2, NULL);

    /*************************************************************/
    /* Loop waiting for incoming connects or for incoming data   */
    /* on any of the connected sockets.                          */
    /*************************************************************/
    while (true)
    {
        /*************************************************************/
        /* Accept the waiting connections                            */
        /*************************************************************/
        clientSocket = accept(tcp_socket, (sockaddr *)&client, &clientSize);
        if (clientSocket == -1)
        {
            perror("socket");
        }
        else
        {
            char host[NI_MAXHOST];
            memset(host, 0, NI_MAXHOST);
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            cout << "---------------------------------------------\nNew Connection established with " << host << "\n---------------------------------------------" << endl;
        }

        pthread_t tid;
        /*************************************************************/
        /* Spawn a new thread for each connection/client             */
        /*************************************************************/
        if (pthread_create(&tid, NULL, &handleTransaction, &clientSocket) < 0)
        {
            perror("create thread error");
            cout << "can't spawn the thread" << endl;
        }
    }

    return 0;
}

/*************************************************************/
/* handleTransaction will get invoked for all the customers  */
/* with a new thread id                                      */
/* will handle transaction for each entry in transaction file*/
/*************************************************************/
void *handleTransaction(void *clientSocket)
{
    cout << "*********************************************" << endl;
    int newSocket = *((int *)clientSocket);
    char *clientData = (char *)malloc(256);
    int n = 0;
    while (n = read(newSocket, clientData, 256))
    {
        if (n < 0)
        {
            cout << "exit " << endl;
            exit(1);
        }

        cout << "Data Received:" << clientData << endl;

        /*************************************************************/
        /* Spliting timestamp, account-no, name of                   */
        /* customer, account-balance                                 */
        /*************************************************************/
        char *token;
        int timestamp;
        token = strtok(clientData, " ");
        if (token)
        {
            timestamp = atoi(token);

            token = strtok(NULL, " ");
            accountNo = atoi(token);

            token = strtok(NULL, " ");
            strcpy(op, token);

            token = strtok(NULL, " ");
            transacAmout = atoi(token);
        }

        for (int i = 0; i < fileCount; i++)
        {
            /*************************************************************/
            /* Check if the account id exists in the db (Record.txt)     */
            /*************************************************************/
            if (account_id[i] == accountNo)
            {
                /*************************************************************/
                /* Lock as we enter the critical section                     */
                /*************************************************************/
                pthread_mutex_init(&mutex2, NULL);
                if (strcmp(op, "w") == 0)
                {
                    if ((account_balance[i] - transacAmout) > 0)
                    {
                        /*************************************************************/
                        /* Deducting the account balance                             */
                        /*************************************************************/
                        account_balance[i] = account_balance[i] - transacAmout;

                        /*************************************************************/
                        /* Logic to update Records file with the new values          */
                        /*************************************************************/
                        ifstream inFile("Records.txt");
                        ofstream outFile("Records1.txt");

                        string strTemp, strTemp1, strTemp2, newFileValue, replaceStr;
                        while (inFile >> strTemp >> strTemp1 >> strTemp2)
                        {
                            if (strTemp == to_string(accountNo))
                            {
                                newFileValue = to_string(account_id[i]) + " " + name[i] + " " + to_string(account_balance[i]) + "\n";
                                outFile << newFileValue;
                            }
                            else
                            {
                                outFile
                                    << strTemp << " " << strTemp1 << " " << strTemp2 << endl;
                            }

                            rename("Records1.txt", "Records.txt");
                        }
                        n = write(newSocket, "ACK: Amount Widrawn\n", 50);
                        cout << "Amount successfully windrawn" << endl;
                    }
                    else
                    {
                        /*************************************************************/
                        /* Account balance low can't perform widrawal                */
                        /*************************************************************/

                        n = write(newSocket, "ACK: Insufficient balance\n", 50);
                    }
                }
                else if (strcmp(op, "d") == 0)
                {
                    /*************************************************************/
                    /* Depositing the account balance                            */
                    /*************************************************************/
                    account_balance[i] = account_balance[i] + transacAmout;

                    /*************************************************************/
                    /* Logic to update Records file with the new values          */
                    /*************************************************************/
                    ifstream inFile("Records.txt");
                    ofstream outFile("Records1.txt");

                    string strTemp, strTemp1, strTemp2, newFileValue, replaceStr;
                    while (inFile >> strTemp >> strTemp1 >> strTemp2)
                    {
                        if (strTemp == to_string(accountNo))
                        {
                            newFileValue = to_string(account_id[i]) + " " + name[i] + " " + to_string(account_balance[i]) + "\n";
                            outFile << newFileValue;
                        }
                        else
                        {
                            outFile
                                << strTemp << " " << strTemp1 << " " << strTemp2 << endl;
                        }

                        rename("Records1.txt", "Records.txt");
                    }
                    n = write(newSocket, "ACK: Amount deposited\n", 50);
                    cout << "Amount successfully deposited" << endl;
                }

                /*************************************************************/
                /* Releasing the lock as we leave the critical section       */
                /*************************************************************/
                cout << "*********************************************" << endl;
                pthread_mutex_unlock(&mutex2);
            }
        }
    }
}
