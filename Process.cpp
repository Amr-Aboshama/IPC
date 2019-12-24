#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

#define MAX_MSG_SIZE 64

struct msgbuf
{
  long m_type;
  string msg;
};

int CLK;

void handler(int signum)
{
  ++CLK;
}

int StoI(char* num)
{
  int ret=0;
  int n=strlen(num);
  for(int i=0 ; i<n ; ++i)
  {
    ret*=10;
    ret+=(num[i]-'0');
  }
  return ret;
}

// argv[0] = Process.exe
// argv[1] = up_queue_id
// argv[2] = down_queue_id
// argv[3] = text file
int main(int argc ,char** argv)
{
  signal(SIGUSR2,handler);

  ifstream In(argv[3]);
  string line;
  vector<pair<int,string>>operations;
  int upMsgqID=StoI(argv[1]),downMsgqID=StoI(argv[2]);
  int pid=getpid();
  
  
  
  while(getline(In,line))
  {
    stringstream ss;
    int arrival;
    string op;
    string msg,tmp;
    ss << line;
    // line = int string string
    ss >> arrival;
    ss >> op;
    ss >> msg;
    if(op=="ADD") msg=op[0]+msg.substr(1,msg.size()-2);
    else msg=op[0]+msg;

    operations.push_back({arrival,msg});
  }
  sort(operations.begin(),operations.end());
  reverse(operations.begin(),operations.end());

  while(!operations.empty())
  {
    if(CLK==operations.back().first)
    {
      msgbuf smessage,rmessage;
      smessage.m_type=pid;
      smessage.msg=operations.back().second;
      smessage.msg.resize(MAX_MSG_SIZE);
      operations.pop_back();
      int send_response=msgsnd(upMsgqID,&smessage,64,!IPC_NOWAIT);
      if(!send_response)
        int rcv_response=msgrcv(downMsgqID,&rmessage,64,pid,!IPC_NOWAIT);
      else
        perror("Error in sending message!");
    }
  }
}
