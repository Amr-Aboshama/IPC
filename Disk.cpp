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

using namespace std;

#define MAX_DISK_SZ 10
#define MAX_MSG_SIZE 64 + 4
#define rep(i, a, b) for (int i = a; i < b; ++i)

int CLK;

vector<string> diskSlots(MAX_DISK_SZ, "");
int upID, downID;

struct msgbuffDisk
{
  long m_type;
  long pid;
  char msg[64];
};

//from Disk to kernel
void Up(string msg, int pid, long mtype)
{
  if (msg.size() > MAX_MSG_SIZE)
  {
    perror("Message is larger than 64 letters");
    return;
  }

  struct msgbuffDisk M;
  M.m_type = mtype;
  M.pid = pid;
  strcpy(M.msg, msg.c_str());
  //M.msg = msg.c_str();

  // !IPC_NOWAIT will make the function waits till it sends the msg
  int send_val = msgsnd(upID, &M, MAX_MSG_SIZE, !IPC_NOWAIT);
  // cout << msg << endl;
  if (send_val == -1)
    perror("Error in sending msg from Disk to Kernel");
}

void Down()
{
  struct msgbuffDisk recvMsg;
  int status = msgrcv(downID, &recvMsg, MAX_MSG_SIZE, 0, IPC_NOWAIT);
  //printf("DISK RCV: %d\n",status);
  // I think it's better to check that it's not zero
  if (status != -1)
  {
    // cout << "STATS: " << status << endl;
    struct msgbuffDisk MSG;
    if (recvMsg.msg[0] == 'D')
    {
      if (!diskSlots[recvMsg.msg[1] - '0'].empty())
        Up("R1", recvMsg.pid, 2);
      else
        Up("R3", recvMsg.pid, 2);
    }
    else
    {
      for (int i = 0; i < MAX_DISK_SZ; ++i)
        if (diskSlots[i].empty())
        {
          diskSlots[i] = recvMsg.msg + 1;
          Up("R0", recvMsg.pid, 2);
          return;
        }
      Up("R2", recvMsg.pid, 2);
    }
  }
}

void handler2(int signum)
{
  CLK++;
}

void handler1(int signum)
{
  int free_slots = 0;
  rep(i, 0, MAX_DISK_SZ)
  {
    if (diskSlots[i] == "")
      free_slots++;
  }
  string txt = "S" + to_string(free_slots);
  Up(txt, -1, 1);
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

int main(int argc, char **argv)
{
  upID = StoI(argv[1]);
  downID = StoI(argv[2]);
  signal(SIGUSR2, handler2);
  signal(SIGUSR1, handler1);

  while (1)
    Down();
}
