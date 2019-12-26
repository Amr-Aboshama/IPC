#include <bits/stdc++.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
//#include <mqueue.h> // message queue
using namespace std;

#define MAX_DISK_SZ 10
#define MAX_MSG_SIZE 64
#define rep(i, a, b) for (int i = a; i < b; i++)
#define PB push_back
#define MP make_pair

vector<string> diskSlots(MAX_DISK_SZ, "");
vector<pair<pair<int, string>, int>> operations;
int upID, downID;

int CLK;

struct msgbuff
{
    long m_type;
    char msg[64];
};

//from kernel to Disk
void Down()
{
    struct msgbuff recvMsg;
    int status = msgrcv(downID, &recvMsg, MAX_MSG_SIZE, 0, IPC_NOWAIT);
	//printf("DISK RCV: %d\n",status);
    // I think it's better to check that it's not zero
    if (status != 0)
    {
        if (recvMsg.msg[0] == 'D')
            operations.PB(MP(MP(CLK + 1, recvMsg.msg), recvMsg.m_type));
        else
            operations.PB(MP(MP(CLK + 3, recvMsg.msg), recvMsg.m_type));
    }
}

vector<pair<int, int>> execute()
{
    // 0 success add
    // 1 success delete
    // 2 failed add
    // 3 dailed delete
    vector<pair<int, string>> toAdd;
    vector<pair<int, int>> response;
    for (int i = 0; i < operations.size(); i++)
    {
        if (operations[i].first.first == CLK)
        {
            if (operations[i].first.second[0] == 'D')
            {
                int slotNum = operations[i].first.second[1] - '0';
                if (diskSlots[slotNum] == "")
                {
                    response.PB(MP(operations[i].second, 3));
                }
                else
                {
                    diskSlots[slotNum] = "";
                    response.PB(MP(operations[i].second, 1));
                }
                operations.erase(operations.begin() + i);
                i--;
            }
            else if (operations[i].first.second[0] == 'A')
            {
                operations[i].first.second.erase(0, 1);
                toAdd.PB(MP(response.size(), operations[i].first.second));
                response.PB(MP(operations[i].second, -1));
                operations.erase(operations.begin() + i);
                i--;
            }
        }
    }
    int j = 0;
    for (int i = 0; i < diskSlots.size(); i++)
    {
        if (diskSlots[i] == "")
        {
            diskSlots[i] = toAdd[j].second[j];
            response[toAdd[j].first].second = 0;
            j++;
        }
    }
    while (j < toAdd.size())
    {
        response[toAdd[j].first].second = 2;
        j++;
    }
    return response;
}

//from Disk to kernel
void Up(string msg, int pid)
{
    if (msg.size() > MAX_MSG_SIZE)
    {
        perror("Message is larger than 64 letters");
        return;
    }

    struct msgbuff M;
    M.m_type = pid;
	strcpy(M.msg,msg.c_str());
    //M.msg = msg.c_str();

    // !IPC_NOWAIT will make the function waits till it sends the msg
    int send_val = msgsnd(upID, &M, MAX_MSG_SIZE, !IPC_NOWAIT);

    if (send_val == -1)
        perror("Error in sending msg from Disk to Kernel");
}

void handler2(int signum)
{
    CLK++;
    vector<pair<int, int>> responses = execute();
    for (auto r : responses)
    {
        string s = "R" + r.second;
        Up(s, r.first);
    }
}

void handler1(int signum)
{
	cout << "IM_HERE" << endl;
    int free_slots = 0;
    rep(i, 0, MAX_DISK_SZ) if (diskSlots[i] == "") free_slots++;
    string txt = "S" + to_string(free_slots);
    Up(txt, 1);
}

int StoI(char *num)
{
    int ret = 0;
    int n = strlen(num);
    for (int i = 0; i < n; ++i)
    {
        ret *= 10;
        ret += (num[i] - '0');
    }
    return ret;
}

// argv[0] = Process.exe
// argv[1] = up_queue_id
// argv[2] = down_queue_id
int main(int argc, char *argv[])
{
    signal(SIGUSR2, handler2);
    signal(SIGUSR1, handler1);
	printf("Disk in: %d\n",getpid());
	//printf("Disk GID: %d\n",getpgrp()); 
    CLK = 0;

    upID = StoI(argv[1]);
    downID = StoI(argv[2]);

    while (1)
        Down();
	printf("Disk died");
    return 0;
}
