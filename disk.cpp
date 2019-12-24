// typedef struct my_mq_msg_sa
// {
//     int type;
//     my_data_t data;
// } my_mq_msg_t;
// typedef union my_data_s {
//     struct del
//     {
//         bool deleteMsg;
//         int slotNum;
//     };
//     struct add
//     {
//         bool deleteMsg;
//         string msg;
//     };
// } my_data_t;

#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <mqueue.h> // message queue
using namespace std;

#define MAX_DISK_SZ 10
#define MY_MQ_UP "/my_upMq"
#define MY_MQ_DOWN "/my_downMq"
#define DATA_STR_LEN 30
#define MAX_MSG_SIZE 64
#define rep(i, a, b) for (int i = a; i < b; i++)
#define PB push_back

vector<string> diskSlots(MAX_DISK_SZ, "");
int upID, downID;

static struct mq_attr mqDown;
static struct mq_attr mqUp;
static mqd_t myDownMq;
static mqd_t myUpMq;
long clk = 0;

struct up_msg
{
    int mtype;
    string mtext;
    bool isStatus; // true => status msg telling how many free slots is there
                    //false => response msg from the recieved msg from kernel
};

struct down_msg
{
    int mtype;
    string mtext;
    bool isAdd;
};

//from kernel to Disk
// 0==> succesful ADD
// 1 ==> successful DEL
// 2 ==> unable to ADD
// 3 ==> unable to DEL
int Down()
{
    struct down_msg recvMsg;

    int status = mq_receive(myDownMq, (char *)&recvMsg, sizeof(recvMsg), NULL);

    // I think it's better to check that it's not zero
    if (status != 0)
    {
        if (recvMsg.isAdd)
        {
            sleep(3000);
            bool added = false;
            for (auto e : diskSlots)
            {
                if (e == "")
                {
                    e = recvMsg.mtext;
                    added = true;
                    break;
                }
            }
            if (added) return 0;
            else return 2;
        }
        else
        {
            sleep(1000);
            bool isEmpty = diskSlots[stoi(recvMsg.mtext)] == "";
            diskSlots[stoi(recvMsg.mtext)] = "";
            if (isEmpty) // unable to delete
                return 3;
            else //deleted successfully
                return 1;
        }
    }
    else
    {
        perror("Error in recieving msg");
        return -1;
    }
}

//from Disk to kernel
void Up(string msg, char type)
{
    if (msg.size() > MAX_MSG_SIZE)
    {
        perror("Message is larger than 64 letters");
        return;
    }

    struct up_msg M;
    M.isStatus = (type == 'S');
    M.mtype = 1;
    M.mtext = msg;

    // !IPC_NOWAIT will make the function waits till it sends the msg
    int send_val = msgsnd(upID, &M, sizeof(up_msg), !IPC_NOWAIT);

    if (send_val == -1)
        perror("Error in sending msg from Disk to Kernel");
}

void handler2(int signum)
{
    clk++;
    
    int res = Down();
    Up(to_string(res), 'R');
}

void handler1(int signum)
{
    string msg = "Disk has ";

    int free_slots = 0;
    rep(i, 0, MAX_DISK_SZ) if (diskSlots[i] == "") free_slots++;

    msg += to_string(free_slots) + " free slots";

    Up(msg, 'S');
}

int main(int argc, char *argv[])
{
    signal(SIGUSR2, handler2);
    signal(SIGUSR1, handler1);

    mqDown.mq_maxmsg = 10;
    mqDown.mq_msgsize = sizeof(down_msg);

    // mqUp.mq_maxmsg = 10;
    // mqUp.mq_msgsize = sizeof(up_msg);

    // myUpMq = mq_open(MY_MQ_UP,
    //                  O_CREAT | O_RDWR | O_NONBLOCK,
    //                  0666,
    //                  &mqUp);

    // assert(myUpMq != -1);

    myDownMq = mq_open(MY_MQ_DOWN,
                       O_CREAT | O_RDWR | O_NONBLOCK,
                       0666,
                       &mqDown);

    assert(myDownMq != -1);

    upID = msgget(IPC_PRIVATE, 0644);
    if (upID == -1)
    {
        perror("Error in creating up_msg queue");
        exit(-1);
    }

    printf("msgqid = %d\n", upID);

    return 0;
}
