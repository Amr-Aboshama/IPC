#include <bits/stdc++.h>
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
#include <mqueue.h>
#include <sys/wait.h>

using namespace std;

#define PROCESSES_COUNT 3
#define RCV_ERROR -1
#define MAX_MSG_SIZE 64
#define DISK_PID pid

int CLK, killedChildren;

struct msgbuff
{
    long mtype;
    string mtext;
};

void sigChildHandler(int signum)
{
    int pid, status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        ++killedChildren;
}

void alarmHandler(int signum)
{
    ++CLK;
    killpg(getpgrp(), SIGUSR2);
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
    upProcessMsgqID = msgget(IPC_PRIVATE, 0644);
    // downProcessMsgqID=msgget(IPC_PRIVATE,0644);
    upDiskMsgqID = msgget(IPC_PRIVATE, 0644);
    downDiskMsgqID = msgget(IPC_PRIVATE, 0644);

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
            execl("Process", "Process", to_string(upProcessMsgqID), "p" + to_string(i) + ".txt", (char *)0);
    }

    //Forking Disk
    pid = fork();
    if (pid == -1)
        perror("Error in forking disk!");
    else if (pid == 0)
        execl("disk", "disk", to_string(upDiskMsgqID), to_string(downDiskMsgqID), (char *)0);

    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGALRM, alarmHandler);
    signal(SIGCHLD, sigChildHandler);
    freopen("kernal.log", "w", stdout);
    alarm(1);
    while (1)
    {
        msgbuff rcvProcessMsg, rcvDiskMsg;
        int rcvUpProcess = msgrcv(upProcessMsgqID, &rcvProcessMsg, MAX_MSG_SIZE, 0, IPC_NOWAIT);
        if (rcvUpProcess != RCV_ERROR)
        {
            printf("Time %d:\tProcess with id: %ld requested to ", CLK, rcvProcessMsg.mtype);

            if (rcvProcessMsg.mtext[0] == 'A')
            {
                string tmp = rcvProcessMsg.mtext;
                tmp.erase(0, 1);
                printf("(ADD) message \"%s\".\n", (tmp).c_str());
            }
            else
                printf("(DELETE) message at slot %c.\n", (rcvProcessMsg.mtext[1]));

            printf("Time %d:\t KERNAL requested the Disk to process pid |%ld|", CLK, rcvProcessMsg.mtype);

            if (rcvProcessMsg.mtext[0] == 'A')
            {
                string tmp = rcvProcessMsg.mtext;
                tmp.erase(0, 1);
                printf("(ADD) message \"%s\".\n", (tmp).c_str());
            }
            else
                printf("(DELETE) message at slot %c.\n", (rcvProcessMsg.mtext[1]));

            // so disk would send status msg
            kill(DISK_PID, SIGUSR1);
            msgbuff msg;
            int bytes = msgrcv(upDiskMsgqID, &msg, MAX_MSG_SIZE, 1, !IPC_NOWAIT);
            if (bytes < 1)
                printf("Error in recieving status msg from Disk\n");
            printf("No. of free slots in Disk at time %d is %c \n", CLK, (msg.mtext[1]));
            int sendVal = msgsnd(downDiskMsgqID, &rcvProcessMsg, MAX_MSG_SIZE, !IPC_NOWAIT);
            if (sendVal == -1)
                perror("Error in sending message from kernal to disk!");
        }
        int rcvUpDisk = msgrcv(upDiskMsgqID, &rcvDiskMsg, MAX_MSG_SIZE, 0, IPC_NOWAIT);
        if (rcvUpProcess != RCV_ERROR && rcvDiskMsg.mtext[0] == 'R')
        {
            int processResponse = rcvDiskMsg.mtext[1] - '0';
            string responseAsTxt;
            switch (processResponse)
            {
            case 0:
                responseAsTxt = "SUCCESSFUL ADD";
                break;
            case 1:
                responseAsTxt = "SUCCESSFUL DELETE";
                break;
            case 2:
                responseAsTxt = "UNABLE TO ADD";
                break;
            case 3:
                responseAsTxt = "UNABLE TO DELETE";
                break;
            default:
                //not needed
                responseAsTxt = "UNKOWN RESPONSE!!";
                break;
            }
            printf("Time %d:\tDisk Response => process PID |%ld| %s\n", CLK, rcvDiskMsg.mtype, responseAsTxt.c_str());

            if (isFinished(upProcessMsgqID, upDiskMsgqID, downDiskMsgqID))
                break;
        }
    }
    kill(DISK_PID, SIGKILL);
    return 0;
}
