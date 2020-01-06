#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

#define PROCESSES_COUNT 1
#define RCV_ERROR -1
#define MAX_MSG_SIZE 64
#define MAX_MSG_SIZE_DISK 64 + 4
#define DISK_PID pid

int CLK, killedChildren, busy;

struct msgbuff
{
    long m_type;
    char msg[64];
};

struct msgbuffDisk
{
    long m_type;
    long pid;
    char msg[64];
};

void sigChildHandler(int signum)
{
    int pid, status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        ++killedChildren;
    //printf("Process %d died and killedChildren = %d\n",pid,killedChildren);
}

void alarmHandler(int signum)
{
    ++CLK;
    killpg(getpgrp(), SIGUSR2);
    if (busy)
        --busy;
    alarm(1);
}

bool isFinished(int UP, int UD, int DD)
{
    if (killedChildren != PROCESSES_COUNT)
        return 0;
    bool empty = 1;
    msqid_ds got;
    msgctl(UP, IPC_STAT, &got);
    empty &= (got.msg_qnum == 0);
    msgctl(UD, IPC_STAT, &got);
    empty &= (got.msg_qnum == 0);
    msgctl(DD, IPC_STAT, &got);
    empty &= (got.msg_qnum == 0);

    return empty;
}

int main()
{
    int pid, upProcessMsgqID, upDiskMsgqID, downDiskMsgqID; //,downProcessMsgqID;
    upProcessMsgqID = msgget(IPC_PRIVATE, 0666);
    upDiskMsgqID = msgget(IPC_PRIVATE, 0666);
    downDiskMsgqID = msgget(IPC_PRIVATE, 0666);

    if (min({upDiskMsgqID, upProcessMsgqID, downDiskMsgqID /*,downProcessMsgqID*/}) < 0)
        perror("Error in creating a message queue!");

    // Forking Processes
    for (int i = 0; i < PROCESSES_COUNT; ++i)
    {
        pid = fork();
        if (pid == -1)
        {
            string tmp = "Error in forking process No. " + to_string(i) + "!";
            perror(tmp.c_str());
        }
        else if (pid == 0)
            execl("Process", "Process", to_string(upProcessMsgqID).c_str(), ("p" + to_string(i) + ".txt").c_str(), (char *)0);
    }

    //Forking Disk
    pid = fork();
    if (pid == -1)
        perror("Error in forking disk!");
    else if (pid == 0)
        execl("Disk", "Disk", to_string(upDiskMsgqID).c_str(), to_string(downDiskMsgqID).c_str(), (char *)0);
    //sleep(1);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGALRM, alarmHandler);
    signal(SIGCHLD, sigChildHandler);
    //freopen("kernal.log", "w", stdout);
    alarm(1);
    while (1)
    {
        //printf("%d==%d\n",killedChildren,PROCESSES_COUNT);
        msgbuff rcvProcessMsg;
        msgbuffDisk rcvDiskMsg;
        int rcvUpProcess = msgrcv(upProcessMsgqID, &rcvProcessMsg, MAX_MSG_SIZE, 0, IPC_NOWAIT);
        //printf("%d %d\n",rcvUpProcess,RCV_ERROR);
        //printf("CLK: %d\n", CLK);
        //printf("%d\n", rcvUpProcess);
        if (rcvUpProcess != RCV_ERROR)
        {
            printf("Time %d:\tProcess with id: %ld requested to ", CLK, rcvProcessMsg.m_type);
            if (rcvProcessMsg.msg[0] == 'A')
                printf("(ADD) message \"%s\".\n", rcvProcessMsg.msg + 1);
            else
                printf("(DELETE) message at slot %c.\n", (rcvProcessMsg.msg[1]));

            printf("Time %d:\t KERNAL requested the Disk to process pid |%ld|", CLK, rcvProcessMsg.m_type);

            if (rcvProcessMsg.msg[0] == 'A')
                printf("(ADD) message \"%s\".\n", rcvProcessMsg.msg + 1);
            else
                printf("(DELETE) message at slot %c.\n", (rcvProcessMsg.msg[1]));

            // so disk would send status msg
            // cout << "BBB: " << busy << endl;
            cout << busy << endl;
            if (!busy)
            {
                kill(DISK_PID, SIGUSR1);
                msgbuffDisk msg;
                int bytes = msgrcv(upDiskMsgqID, &msg, MAX_MSG_SIZE_DISK, 1, !IPC_NOWAIT);
                if (bytes == -1)
                    printf("Error in recieving status msg from Disk\n");
                else
                    printf("No. of free slots in Disk at time %d is %s \n", CLK, (msg.msg + 1));
                int sendVal = msgsnd(downDiskMsgqID, &rcvProcessMsg, MAX_MSG_SIZE, !IPC_NOWAIT);
                if (sendVal == -1)
                    perror("Error in sending message from kernal to disk!");
                else
                    busy = (rcvProcessMsg.msg[0] == 'A') ? 3 : 1;

                k Response = > process PID | % ld | % s\n ", CLK, rcvDiskMsg.pid, responseAsTxt.c_str());
            }
        }
    }
    //printf("KILLED: %d %d\n", killedChildren, PROCESSES_COUNT);
    if (isFinished(upProcessMsgqID, upDiskMsgqID, downDiskMsgqID))
    {
        printf("Killed: %d\n", killedChildren);
        kill(DISK_PID, SIGKILL);
        printf("KERNAL died\n");
        printf("Finish Time: %d\n", CLK);
        return 0;
    }
}
}
