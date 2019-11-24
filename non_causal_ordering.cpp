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
#include <array>
#include <stdlib.h>
#include <pthread.h>
#define MAXRECORDSIZE 200;
#include <chrono>
#define PORT 12345
#define IP "225.0.0.37"
using namespace std::chrono;
using namespace std;

void *sender(void *);
void *receiver(void *);

/*************************************************************/
/* Global Variable declaration to store records in server    */
/* server state after boot up                                */
/*************************************************************/

int portNo;
int processId;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

string inputFileRecords;
string name[100];
int m_sock;
sockaddr_in m_addr;
sockaddr_in m_r_addr;
struct ip_mreq mr;
int vectorClock[3] = {0, 0, 0};
int delivered_msg[3] = {0, 0, 0};
int main(int argc, char **argv)
{
    u_int on = 1;
    /*************************************************************/
    /* Create a AF_INET IPv4 stream socket to receive incoming   */
    /* requests                                                  */
    /*************************************************************/

    /*************************************************************/
    /* Setting up the sender sockets                             */
    /*************************************************************/
    m_sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset((char *)&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(IP);
    m_addr.sin_port = htons(PORT);

    /*************************************************************/
    /* Setting up the receiver sockets                           */
    /*************************************************************/
    memset((char *)&m_r_addr, 0, sizeof(m_r_addr));
    m_r_addr.sin_family = AF_INET;
    m_r_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_r_addr.sin_port = htons(PORT);

    if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        cout << "Unable to allow multiple sockets to use the same PORT number" << endl;
        return 0;
    }

    /*************************************************************/
    /* Binding the socket to send and receiver                   */
    /*************************************************************/
    int m = bind(m_sock, (struct sockaddr *)&m_r_addr, sizeof(m_r_addr));
    if (m < 0)
    {
        cout << "Cannot bind" << endl;
        return 0;
    }

    /*************************************************************/
    /* Enable the multicast group                                */
    /*************************************************************/
    mr.imr_multiaddr.s_addr = inet_addr(IP);
    mr.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0)
    {
        cout << "Error in setsockopt" << endl;
        return 0;
    }

    pthread_t sender_tid;
    pthread_t receiver_tid;
    int processId = atoi(argv[1]);

    /*************************************************************/
    /* Spawn the sender thread for the given process             */
    /*************************************************************/
    if (pthread_create(&sender_tid, NULL, sender, &processId) < 0)
    {
        cout << "Cannot create a thread for sender" << endl;
        return 1;
    }

    /*************************************************************/
    /* Spawn the receiver thread for the given process           */
    /*************************************************************/
    if (pthread_create(&receiver_tid, NULL, receiver, &processId) < 0)
    {
        cout << "Cannot create a thread for receiver" << endl;
        return 1;
    }
    pthread_join(sender_tid, NULL);

    pthread_join(receiver_tid, NULL);

    return 0;
}

/*************************************************************/
/* Sender thread spawn from the process                      */
/*************************************************************/
void *sender(void *processId)
{

    int pId = *((int *)processId);
    cout << "Sender Thread Spawned for Process: " << pId << endl;

    /*************************************************************/
    /* Sender thread spawned from the process                    */
    /*************************************************************/
    while (true)
    {
        /*************************************************************/
        /* Sender process increments its vector value before         */
        /* sending the message to the receiver thread                */
        /*************************************************************/
        for (int i = 1; i < 4; i++)
        {
            if (pId == i)
            {
                {
                    if (1 == pId)
                    {
                        vectorClock[0]++;
                    }
                    else if (2 == pId)
                    {
                        vectorClock[1]++;
                    }
                    else if (3 == pId)

                    {
                        vectorClock[2]++;
                    }
                }
            }
        }
        string messageVector = to_string(vectorClock[0]) + ',' + to_string(vectorClock[1]) + ',' + to_string(vectorClock[2]);
        string sendString = to_string(pId) + ',' + to_string(vectorClock[0]) + ',' + to_string(vectorClock[1]) + ',' + to_string(vectorClock[2]);
        char ctr[256];
        strcpy(ctr, sendString.c_str());

        /*************************************************************/
        /* Send the message to the socket                            */
        /*************************************************************/
        int send1 = sendto(m_sock, ctr, sizeof(ctr), 0, (struct sockaddr *)&m_addr, sizeof(m_addr));
        if (send1 < 0)
        {
            perror("error");
        }
        cout << "Message sent from process " << pId << " :" << messageVector << endl;
        sleep(10);
    }
}

/*************************************************************/
/* Receiver thread spawn from the process                    */
/*************************************************************/
void *receiver(void *processId)
{
    int pId = *((int *)processId);
    cout << "Reciever Thread Spawned for Process: " << pId << endl;
    char buffer2[256];
    socklen_t addr = sizeof(m_r_addr);
    int vector_first[3] = {0,
                           0,
                           0};

    int buffered_msg[3] = {0, 0, 0};

    /*************************************************************/
    /* Receiver keeps receiving on the socket in the loop        */
    /*************************************************************/
    while (recvfrom(m_sock, buffer2, sizeof(buffer2), 0, (struct sockaddr *)&m_r_addr, &addr) >= 0)
    {

        /*************************************************************/
        /* Reciving the response from the sender in the form         */
        /* Below is the code to parse the incomming message          */
        /*************************************************************/
        char *token;
        token = strtok(buffer2, ",");
        if (token)
        {
            vector_first[0] = atoi(token);

            token = strtok(NULL, ",");
            vector_first[1] = atoi(token);

            token = strtok(NULL, ",");
            vector_first[2] = atoi(token);

            token = strtok(NULL, ",");
            vector_first[3] = atoi(token);
        }

        cout << endl;
        cout << "Receiverd message: " << vector_first[1] << " " << vector_first[2] << " " << vector_first[3] << endl;
        cout << "Message received from Process: " << vector_first[0] << endl;

        if (pId == 1)
        {
            /*************************************************************/
            /* Enters here when the receiving process is 1               */
            /*************************************************************/
            if (vector_first[0] == 2)
            {
                /*************************************************************/
                /* Updating the vector value when the sender process is 2    */
                /*************************************************************/
                if ((delivered_msg[1] + 1 == vector_first[2]))
                {
                    // delivered_msg[0] = vector_first[1];
                    delivered_msg[1] = vector_first[2];
                    // delivered_msg[2] = vector_first[3];

                    cout << "Message Delivered at 1 from 2: " << delivered_msg[0] << "," << delivered_msg[1] << "," << delivered_msg[2] << endl;
                }
                else
                {
                    /*************************************************************/
                    /* Buffering the vector value when the sender process is 2   */
                    /* And the deliveriy criteria is not satisfied               */
                    /*************************************************************/
                    buffered_msg[0] = vector_first[1];
                    buffered_msg[1] = vector_first[2];
                    buffered_msg[2] = vector_first[3];

                    cout << "Message Buffered at 1 from 2: " << buffered_msg[0] << "," << buffered_msg[1] << "," << buffered_msg[2] << endl;
                }
            }
            else if (vector_first[0] == 3)
            {
                /*************************************************************/
                /* Updating the vector value when the sender process is 3    */
                /*************************************************************/
                if ((delivered_msg[2] + 1 == vector_first[3]))
                {

                    // delivered_msg[0] = vector_first[1];
                    // delivered_msg[1] = vector_first[2];
                    delivered_msg[2] = vector_first[3];

                    cout << "Message Delivered at 1 from 3: " << delivered_msg[0] << "," << delivered_msg[1] << "," << delivered_msg[2] << endl;
                }
                else
                {
                    /*************************************************************/
                    /* Buffering the vector value when the sender process is 3   */
                    /* And the deliveriy criteria is not satisfied               */
                    /*************************************************************/
                    buffered_msg[0] = vector_first[1];
                    buffered_msg[1] = vector_first[2];
                    buffered_msg[2] = vector_first[3];

                    cout << "Message Buffered at 1 from 3: " << buffered_msg[0] << "," << buffered_msg[1] << "," << buffered_msg[2] << endl;
                }
            }
            else if (vector_first[0] == 1)
            {
                /*************************************************************/
                /* Updating the vector when process 1 sends message to itself*/
                /*************************************************************/
                delivered_msg[0] = vector_first[1];
                // delivered_msg[1] = vector_first[2];
                // delivered_msg[2] = vector_first[3];

                cout << "Message Delivered at 1 from 1: " << delivered_msg[0] << "," << delivered_msg[1] << "," << delivered_msg[2] << endl;
            }
        }
        else if (pId == 2)
        {
            /*************************************************************/
            /* Enters here when the receiving process is 2               */
            /*************************************************************/
            if (vector_first[0] == 1)
            {
                /*************************************************************/
                /* Updating the vector value when the sender process is 1    */
                /*************************************************************/
                if (delivered_msg[0] + 1 == vector_first[1])
                {

                    delivered_msg[0] = vector_first[1];
                    // delivered_msg[1] = vector_first[2];
                    // delivered_msg[2] = vector_first[3];

                    cout << "Message Delivered at 2 from 1: " << delivered_msg[0] << "," << delivered_msg[1] << "," << delivered_msg[2] << endl;
                }
                else
                {
                    /*************************************************************/
                    /* Buffering the vector value when the sender process is 1   */
                    /* And the deliveriy criteria is not satisfied               */
                    /*************************************************************/
                    buffered_msg[0] = vector_first[1];
                    buffered_msg[1] = vector_first[2];
                    buffered_msg[2] = vector_first[3];

                    cout << "Message Buffered at 2 from 1: " << buffered_msg[0] << "," << buffered_msg[1] << "," << buffered_msg[2] << endl;
                }
            }
            else if (vector_first[0] == 3)
            {
                /*************************************************************/
                /* Updating the vector value when the sender process is 3    */
                /*************************************************************/
                if (delivered_msg[2] + 1 == vector_first[3])
                {

                    // delivered_msg[0] = vector_first[1];
                    // delivered_msg[1] = vector_first[2];
                    delivered_msg[2] = vector_first[3];

                    cout << "Message Delivered at 2 from 3: " << delivered_msg[0] << "," << delivered_msg[1] << "," << delivered_msg[2] << endl;
                }
                else
                {
                    /*************************************************************/
                    /* Buffering the vector value when the sender process is 3   */
                    /* And the deliveriy criteria is not satisfied               */
                    /*************************************************************/
                    buffered_msg[0] = vector_first[1];
                    buffered_msg[1] = vector_first[2];
                    buffered_msg[2] = vector_first[3];

                    cout << "Message Buffered at 2 from 3: " << buffered_msg[0] << "," << buffered_msg[1] << "," << buffered_msg[2] << endl;
                }
            }
            else if (vector_first[0] == 2)
            {
                /*************************************************************/
                /* Updating the vector value when process 2 sends to itself  */
                /*************************************************************/
                // delivered_msg[0] = vector_first[1];
                delivered_msg[1] = vector_first[2];
                // delivered_msg[2] = vector_first[3];

                cout << "Message Delivered at 2 from 2: " << endl;
            }
        }
        else if (pId == 3)
        {
            /*************************************************************/
            /* Enters here when the receiving process is 3               */
            /*************************************************************/

            if (vector_first[0] == 1)
            {
                /*************************************************************/
                /* Updating the vector value when the sender process is 1    */
                /*************************************************************/
                if (delivered_msg[0] + 1 == vector_first[1])
                {

                    delivered_msg[0] = vector_first[1];
                    // delivered_msg[1] = vector_first[2];
                    // delivered_msg[2] = vector_first[3];

                    cout << "Message Delivered at 3 from 1: " << delivered_msg[0] << "," << delivered_msg[1] << "," << delivered_msg[2] << endl;
                }
                else
                {
                    /*************************************************************/
                    /* Buffering the vector value when the sender process is 1   */
                    /* And the deliveriy criteria is not satisfied               */
                    /*************************************************************/
                    buffered_msg[0] = vector_first[1];
                    buffered_msg[1] = vector_first[2];
                    buffered_msg[2] = vector_first[3];

                    cout << "Message Buffered at 3 from 1: " << endl;
                }
            }
            else if (vector_first[0] == 2)
            {
                /*************************************************************/
                /* Updating the vector value when the sender process is 2    */
                /*************************************************************/
                if (delivered_msg[1] + 1 == vector_first[2])
                {
                    // delivered_msg[0] = vector_first[1];
                    delivered_msg[1] = vector_first[2];
                    // delivered_msg[2] = vector_first[3];

                    cout << "Message Delivered at 3 from 2: " << delivered_msg[0] << "," << delivered_msg[1] << "," << delivered_msg[2] << endl;
                }
                else
                {
                    /*************************************************************/
                    /* Buffering the vector value when the sender process is 1   */
                    /* And the deliveriy criteria is not satisfied               */
                    /*************************************************************/
                    buffered_msg[0] = vector_first[1];
                    buffered_msg[1] = vector_first[2];
                    buffered_msg[2] = vector_first[3];

                    cout << "Message Buffered at 3 from 1: " << buffered_msg[0] << "," << buffered_msg[1] << "," << buffered_msg[2] << endl;
                }
            }
            else if (vector_first[0] == 3)
            {
                /*************************************************************/
                /* Updating the vector value when process 2 sends to itself  */
                /*************************************************************/
                // delivered_msg[0] = vector_first[1];
                // delivered_msg[1] = vector_first[2];
                delivered_msg[2] = vector_first[3];

                cout << endl;
                cout << "Message Delivered" << endl;
            }
        }
        sleep(10);
    }
}
