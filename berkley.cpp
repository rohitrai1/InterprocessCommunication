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
#include <array>
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
int portNo;
uint32_t connectedPorts[3] = {54002, 54003, 54004};
int slaveTime[3];
int processId;
int transacAmout;
int sumOfSlaveTime;

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

string inputFileRecords;
string name[100];
u_int optname = 1;

int main(int argc, char **argv)
{
    time_t timer;
    srand((unsigned)time(&timer));
    int logical_clock = rand() % 100;
    if (argv[1] == NULL)
    {
        cout << "No Port number specified" << endl;
        cout << "Please provide a port number for example :: ./main 54002" << endl;
        exit(1);
    }
    portNo = atoi(argv[1]);
    processId = atoi(argv[2]);
    sockaddr_in client;
    socklen_t clientSize;

    /*************************************************************/
    /* Loop waiting for incoming connects or for incoming data   */
    /* on any of the connected sockets.                          */
    /*************************************************************/
    /*************************************************************/
    /* Accept the waiting connections                            */
    /*************************************************************/
    if (processId == 1)
    {
        /*************************************************************/
        /* If the processId is 1 it is a master process              */
        /*************************************************************/

        /*************************************************************/
        /* Looping through all the slaves                            */
        /*************************************************************/
        for (int i = 0; i < 3; i++)
        {
            /*************************************************************/
            /* Initializing the master socket                            */
            /* And connecting it to all the slaves                       */
            /*************************************************************/

            int masterSocket = -1;
            if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("\n Error : Could not create socket \n");
            }

            struct sockaddr_in serv_addr;
            memset(&serv_addr, '\0', sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            string ipAddress = "127.0.0.1";
            serv_addr.sin_port = htons(connectedPorts[i]);
            if (inet_pton(AF_INET, ipAddress.c_str(), &serv_addr.sin_addr) <= 0)
            {
                perror("invalid address");
            }

            int connectResult = connect(masterSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            if (connectResult == -1)
            {
                perror("can't connect");
            }
            if (i == 0)
            {
                cout << "Master Process Connected on Port " << portNo << endl;
                cout << "Master Clock says: " << logical_clock << endl;
            }
            cout << endl
                 << endl;

            /*************************************************************/
            /* Requesting time from all the slave processes              */
            /*************************************************************/
            cout << "Request clock time from Slave " << i + 1 << endl;

            int n = write(masterSocket, "Request for clock", 50);
            if (n < 0)
            {
                perror("write error");
            }

            /*************************************************************/
            /* Reading the slave response (time)                         */
            /*************************************************************/
            char *slaveData = (char *)malloc(256);
            int ny = read(masterSocket, slaveData, 256);
            if (ny < 0)
            {
                perror("error in read");
            }
            cout << "Response from the slave " << i + 1 << ": " << slaveData << endl;
            slaveTime[i] = atoi(slaveData);
            cout << "Time on slave " << i + 1 << ": " << slaveTime[i] << endl;
            sumOfSlaveTime = sumOfSlaveTime + slaveTime[i];
            cout << endl;
            cout << endl;
        }

        sumOfSlaveTime = sumOfSlaveTime + logical_clock;
        /*************************************************************/
        /* Calculating the average as per slave response             */
        /*************************************************************/
        int average = (sumOfSlaveTime / 3);
        cout << "The average on the values received from all slaves: " << average << endl;
        cout << endl;
        cout << endl;

        /*************************************************************/
        /* Adjusting the master clock                                */
        /*************************************************************/
        int masterOffset = average - logical_clock;
        logical_clock = logical_clock + masterOffset;
        cout << "Adjusting the master clock to " << logical_clock << endl;
        cout << endl;
        /*************************************************************/
        /* Looping through the slaves to send the clock drift        */
        /*************************************************************/
        for (int i = 0; i < 3; i++)
        {
            int masterSocket = -1;
            if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("\n Error : Could not create socket \n");
            }

            struct sockaddr_in serv_addr;
            memset(&serv_addr, '\0', sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            string ipAddress = "127.0.0.1";
            serv_addr.sin_port = htons(connectedPorts[i]);
            if (inet_pton(AF_INET, ipAddress.c_str(), &serv_addr.sin_addr) <= 0)
            {
                perror("invalid address");
            }

            int connectResult = connect(masterSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            if (connectResult == -1)
            {
                perror("can't connect");
            }

            /*************************************************************/
            /* Calculating clock drift for each slave                    */
            /*************************************************************/
            int drift = average - slaveTime[i];
            if (i == 0)
            {
                cout << "The drift on slave 1: " << drift << endl;
                cout << "Requesting slave 1 to update clock based on the drift: " << drift << endl;
            }
            else if (i == 1)
            {
                cout << "The drift on slave 2: " << drift << endl;
                cout << "Requesting slave 2 to update clock based on the drift: " << drift << endl;
            }
            else if (i == 2)
            {
                cout << "The drift on slave 3: " << drift << endl;
                cout << "Requesting slave 3 to update clock based on the drift: " << drift << endl;
            }
            char sendBuffer[256];
            memset(sendBuffer, '\0', sizeof(sendBuffer)); // clear send buffer before sending response data
            sprintf(sendBuffer, "%d", drift);

            /*************************************************************/
            /* Sending the drift to each slave                           */
            /*************************************************************/
            int n = write(masterSocket, sendBuffer, strlen(sendBuffer));

            if (n < 0)
            {
                perror("write error");
            }

            char *slaveData = (char *)malloc(256);
            int ny = read(masterSocket, slaveData, 256);
            if (ny < 0)
            {
                perror("error in read");
            }
            cout << "Updated response from the slave " << i + 1 << ":" << slaveData << endl;
            cout << endl;
        }
    }
    else
    {
        /*************************************************************/
        /* Enters this section when it is a slave process            */
        /*************************************************************/
        if (processId == 2)
        {
            cout << "Slave 1 connected on Port " << argv[1] << endl;
        }
        else if (processId == 3)
        {
            cout << "Slave 2 connected on Port  " << argv[1] << endl;
        }
        else if (processId == 4)
        {
            cout << "Slave 3 connected on Port " << argv[1] << endl;
        }

        int slave_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (slave_socket == -1)
        {
            perror("connection");
        }
        int opt = 1;
        if (setsockopt(slave_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
        {
            perror("setsockopt");
        }

        /*************************************************************/
        /* Socket initialization for the slaves                      */
        /*************************************************************/
        if (argv[1] == NULL)
        {
            cout << "No Port number specified" << endl;
            cout << "Please provide a port number for example :: ./main 54002" << endl;
            exit(1);
        }
        portNo = atoi(argv[1]);
        sockaddr_in sock_detail;
        sock_detail.sin_family = AF_INET;
        sock_detail.sin_port = htons(portNo);

        inet_pton(AF_INET, "0.0.0.0", &sock_detail.sin_addr); //character string src to a network address strcuture
        if (bind(slave_socket, (struct sockaddr *)&sock_detail, sizeof(sockaddr_in)))
        {
            cout << "Unable to bind the socket to IP and Port" << endl;
            perror("unable to bind");
        }

        if (listen(slave_socket, SOMAXCONN) == -1)
        {
            perror("listen");
        }
        /*************************************************************/
        /* Slaves are waiting to listen from the master              */
        /*************************************************************/
        while (true)
        {

            int connectResult = accept(slave_socket, (struct sockaddr *)NULL, NULL);
            if (connectResult < 0)
            {
                perror("error in connect");
            }
            else
            {
                char host[NI_MAXHOST];
                memset(host, 0, NI_MAXHOST);
                inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
                cout << "---------------------------------------------\nNew Connection established with " << host << "\n---------------------------------------------" << endl;

                char *clientData = (char *)malloc(256);
                int n = read(connectResult, clientData, 256);
                if (n < 0)
                {
                    perror("error receiving data");
                    exit(1);
                }
                cout << "Data received from the master: " << clientData << endl;

                /*************************************************************/
                /* Code to receive and parse request for clock from master   */
                /*************************************************************/
                int compareResult = strcmp("Request for clock", clientData);
                if (compareResult == 0)
                {
                    char sendBuffer[256];
                    memset(sendBuffer, '\0', sizeof(sendBuffer));
                    sprintf(sendBuffer, "%d", logical_clock);

                    sleep(5);

                    /*************************************************************/
                    /* Slave sends its logical time back to master after request */
                    /*************************************************************/
                    int n = write(connectResult, sendBuffer, strlen(sendBuffer));

                    if (n < 0)
                    {
                        perror("Error sending response.");
                    }

                    cout << "My logical Clock says " << logical_clock << endl;
                    cout << "Sending my clock value to the master " << logical_clock << endl;
                }
                else
                {
                    /*************************************************************/
                    /* Slave receives the drift value from the master            */
                    /*************************************************************/
                    cout << "The drift as calculated by master is " << clientData << endl;
                    int drift = atoi(clientData);
                    logical_clock = logical_clock + drift;
                    cout << "My Updated logical time is " << logical_clock << endl;
                    char sendBuffer[256];
                    memset(sendBuffer, '\0', sizeof(sendBuffer));
                    sprintf(sendBuffer, "%d", logical_clock);

                    sleep(5);
                    int n = write(connectResult, sendBuffer, strlen(sendBuffer));
                }
            }
        }
    }

    return 0;
}