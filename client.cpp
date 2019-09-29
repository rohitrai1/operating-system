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
#include <ctime>
#include <chrono>

using namespace std::chrono;
using namespace std;

int main()
{
    /*************************************************************/
    /* Client socket initialization                              */
    /*************************************************************/
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int port = 54002;
    string ipAddress = "127.0.0.1";

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    /*************************************************************/
    /* Connect to the server socket                              */
    /*************************************************************/

    int connectResult = connect(sock, (sockaddr *)&hint, sizeof(sockaddr_in));
    if (connectResult == -1)
    {
        return 1;
    }

    string record;
    int timestamp;
    int account_no;
    char transactionType[2];
    int transaction_amout;

    /*************************************************************/
    /* Read transaction.txt file and store it to the server state*/
    /*************************************************************/
    ifstream theFile("Transactions.txt");

    int transactionCount = 0;
    /*************************************************************/
    /* Delay each transaction based on timestamp                 */
    /* And send it to the server one by one                      */
    /*************************************************************/
    while (theFile >> timestamp >> account_no >> transactionType >> transaction_amout)
    {
        auto now = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch());
        auto transactionTimestamp = std::chrono::milliseconds(timestamp);
        auto delay = now + transactionTimestamp;
        transactionCount++;
        if (delay.count() > duration_cast<milliseconds>(
                                system_clock::now().time_since_epoch())
                                .count())
        {

            sleep((delay.count() - duration_cast<milliseconds>(
                                       system_clock::now().time_since_epoch())
                                       .count()) /
                  1000);
        }

        record = to_string(timestamp) + " " + to_string(account_no) + " " + transactionType + " " + to_string(transaction_amout);
        cout << "Triggered transaction ----->" << record << endl;
        int resposne = write(sock, record.c_str(), 256);
        if (resposne < 0)
        {
            cout << "error";
        }

        /*************************************************************/
        /* Read response from the server (acknowledgment)            */
        /*************************************************************/
        char buffer[256];
        resposne = read(sock, buffer, 256);
        printf("Message Received : %s\n", buffer);
    }
}