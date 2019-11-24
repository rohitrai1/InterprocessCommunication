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
#include <queue>

using namespace std::chrono;
using namespace std;

void *handleTransaction(void *);

/*************************************************************/
/* Global Variable declaration to store records in server    */
/* server state after boot up                                */
/*************************************************************/

char op[2];
;
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
    queue<int> myqueue;
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

    if (processId == 1)
    {
        /*************************************************************/
        /* Enters this section when it is a coordinator process      */
        /*************************************************************/

        /*************************************************************/
        /* Setting up the locking variables                          */
        /* 0 is usable                                               */
        /* 1 is not usable currently                                 */
        /*************************************************************/
        int resourceState = 0;
        int masterSocket = -1;
        int sockSeq[3] = {0, 0, 0};

        /*************************************************************/
        /* The coordinator loops through all the slaves              */
        /* and informs them of its availibility                      */
        /*************************************************************/
        for (int i = 0; i < 3; i++)
        {
            if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("\n Error : Could not create socket \n");
            }

            sockSeq[i] = masterSocket;

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
            cout << "connected port no" << connectedPorts[i] << endl;

            /*************************************************************/
            /* The coordinator tells every node that it is up            */
            /*************************************************************/
            int n = write(masterSocket, "I am up", 50);
            if (n < 0)

            {
                perror("write error");
            }
        }

        for (int i = 0; i < 3; i++)
        {
            /*************************************************************/
            /* The slaves request for the resource                       */
            /*************************************************************/
            char *slaveData = (char *)malloc(256);
            int x = read(sockSeq[i], slaveData, 256);
            if (x < 0)
            {
                perror("error");
            }
            char *token = strtok(slaveData, "-");
            token = strtok(NULL, "-");
            cout << token << endl;

            /*************************************************************/
            /* The coordinator checks for the queue and accordingly      */
            /* decides weather to grant the resource or not              */
            /*************************************************************/
            if (myqueue.empty() && resourceState == 0)
            {
                cout << "Giving acess to process-" << token << endl;
                resourceState == 1;

                /*************************************************************/
                /* The coordinator pushes the slave pid in the queue         */
                /*************************************************************/
                myqueue.push(atoi(token));

                /*************************************************************/
                /* The coordinator sends out an "OK" if the resource         */
                /* has to be granted                                         */
                /*************************************************************/
                int n = write(sockSeq[i], "OK", 50);
                if (n < 0)
                {
                    perror("write error");
                }
            }
            else
            {
                myqueue.push(atoi(token));
            }

            /*************************************************************/
            /* The slave response is received by the coordinator         */
            /* they respond as DONE when they done using the resource    */
            /*************************************************************/
            char *slaveData1 = (char *)malloc(256);
            sleep(2);
            int ny1 = read(sockSeq[i], slaveData1, 256);
            cout << slaveData1 << endl;
            if (ny1 < 0)
            {
                perror("error receiving data");
                exit(1);
            }
            if (strcmp("DONE-2", slaveData1) == 0)
            {
                cout << "Prcess 2 releasing the resource" << endl;
                myqueue.pop();
            }
            else if (strcmp("DONE-3", slaveData1) == 0)
            {
                cout << "Prcess 3 releasing the resource" << endl;
                myqueue.pop();
            }
            else if (strcmp("DONE-4", slaveData1) == 0)
            {
                cout << "Prcess 4 releasing the resource" << endl;
                myqueue.pop();
            }
        }
    }
    else
    {
        /*************************************************************/
        /* Enters this section when it is a slave process            */
        /*************************************************************/
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
        /* Socket initialization for the slave                       */
        /*************************************************************/
        if (argv[1] == NULL)
        {
            cout << "No Port number specified" << endl;
            cout << "Please provide a port number for example :: ./main 54002" << endl;
            exit(1);
        }
        cout << "Slave process: " << processId << " connected on port " << argv[1] << endl;
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
        /* Socket waiting for the coordinator to get up              */
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

                /*************************************************************/
                /* Slaves receive data and now they know coordinator is up   */
                /*************************************************************/
                char *clientData = (char *)malloc(256);
                int n = read(connectResult, clientData, 256);
                if (n < 0)
                {
                    perror("error receiving data");
                    exit(1);
                }
                cout << "Data Received:" << clientData << endl;

                int compareResult = strcmp("I am up", clientData);
                if (compareResult == 0)
                {

                    sleep(5);
                    /*************************************************************/
                    /* Slaves start to request for the resource, the counter file*/
                    /*************************************************************/
                    if (atoi(argv[2]) == 2)
                    {

                        int n = write(connectResult, "Request for resource from process-2", 40);
                    }
                    else if (atoi(argv[2]) == 3)
                    {
                        int n = write(connectResult, "Request for resource from process-3", 40);
                    }
                    else if (atoi(argv[2]) == 4)
                    {
                        int n = write(connectResult, "Request for resource from process-4", 40);
                    }

                    if (n < 0)
                    {
                        perror("Error sending response.");
                    }

                    char *clientData = (char *)malloc(256);
                    int n = read(connectResult, clientData, 256);
                    if (n < 0)
                    {
                        perror("error receiving data");
                        exit(1);
                    }
                    cout << clientData << endl;
                    int compareResult = strcmp("OK", clientData);

                    /*************************************************************/
                    /* If the coordinator gets an "OK" the slave utilizes resourc*/
                    /*************************************************************/
                    if (compareResult == 0)
                    {
                        cout << "got okay from the coordinator proces to read" << endl;

                        /*************************************************************/
                        /* Give access to file to increment the counter              */
                        /*************************************************************/
                        /*************************************************************/
                        /* Read the records file and store it to the server state    */
                        /*************************************************************/
                        ifstream theFile("Counter.txt");

                        if (theFile.fail())
                        {
                            cerr << "Error reading the record file" << endl;
                            exit(1);
                        }
                        int count = 0;
                        while (theFile >> count)
                        {
                            cout << "count value " << count << endl;
                            count++;
                        }

                        ofstream outFile("Counter.txt");
                        if (outFile.fail())
                        {
                            cerr << "Error reading the record file" << endl;
                            exit(1);
                        }

                        outFile << count;

                        /*************************************************************/
                        /* Slaves send back done when done using the resource        */
                        /*************************************************************/
                        if (atoi(argv[2]) == 2)
                        {

                            int n = write(connectResult, "DONE-2", 40);
                        }
                        else if (atoi(argv[2]) == 3)
                        {
                            int n = write(connectResult, "DONE-3", 40);
                        }
                        else if (atoi(argv[2]) == 4)
                        {
                            int n = write(connectResult, "DONE-4", 40);
                        }
                    }
                }
            }
        }
    }

    return 0;
}