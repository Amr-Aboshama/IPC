#include <bits/stdc++.h>

using namespace std;

#define PROCESSES_COUNT 3


int main()
{
    int pid,upProcessMsgqID,upDiskMsgqID,downProcessMsgqID,downDiskMsgqID;

    upProcessMsgqID=msgget(IPC_PRIVATE,0644);
    downProcessMsgqID=msgget(IPC_PRIVATE,0644);
    upDiskMsgqID=msgget(IPC_PRIVATE,0644);
    downDiskMsgqID=msgget(IPC_PRIVATE,0644);

    if(min({upDiskMsgqID,upProcessMsgqID,downDiskMsgqID,downProcessMsgqID})==-1)  perror("Error in creating a message queue!");

    //Forking Disk
    pid=fork();
    if(pid==-1) perror("Error in forking disk!");
    else if(pid==0) execl("disk", "disk", atoi(upDiskMsgqID),atoi(downDiskMsgqID), (char*)0);

    // Forking Processes
    for (int i = 0; i < PROCESSES_COUNT; ++i)
    {
        pid=fork();
        if(pid==-1) perror("Error in forking process No."+atoi(i)+"!");
        else if(pid==0) execl("Process","Process",atoi(upProcessMsgqID),atoi(downProcessMsgqID),"p"+i+".txt",(char*)0);
    }
    signal(SIGUSR1,SIG_IGN);
    signal(SIGUSR2,SIG_IGN);
    freopen("kernal.log","w",stdout);
    
}