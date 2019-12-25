#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

#define MAX_MSG_SIZE 64

struct msgbuff
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
// argv[2] = text file
// argv[3] = down_queue_id
int main(int argc ,char** argv)
{
  signal(SIGUSR2,handler);
  signal(SIGUSR1,SIG_IGN);

  ifstream In(argv[2]);
  string line;
  vector<pair<int,string>>operations;
  int upMsgqID=StoI(argv[1]);
  // int downMsgqID=StoI(argv[3]);
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
      msgbuff smessage;
      // msgbuff rmessage;
      smessage.m_type=pid;
      smessage.msg=operations.back().second;
      smessage.msg.resize(MAX_MSG_SIZE);
      operations.pop_back();
      int sendResponse=msgsnd(upMsgqID,&smessage,MAX_MSG_SIZE,!IPC_NOWAIT);
      //if(!send_response)
        //"!IPC_NOWAIT" can cause a dead lock if it waited 3 clock cycles or more to get response from kernel
        // time = last_time+3 at least and maybe arrival time of the first message in the process < last_time +3 so it will be in a desd lock 
        //so we made it with "IPC_NOWAIT"
        //int rcv_response=msgrcv(downMsgqID,&rmessage,MAX_MSG_SIZE,pid,IPC_NOWAIT);
      if(sendResponse == -1)
        perror("Error in sending message!");
    }
  }
  return 0;
}
