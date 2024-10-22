#include <iostream>
#include <sys/wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#define BUFFER_RECV 512

enum class PIPE_TYPE
{
    READ = 0,
    WRITE = 1,
    NUMS = 2,
};


void dupPipesBashProc(int* p2C, int* c2P);
void closeFdForC(int* p2C, int* c2P);
void closeFdForP(int* p2C, int* c2P);
void shellLoop(int* p2C, int* c2P);

int main(int argc, char* argv[], char* envp[])
{
    // Create 2 pipe for parents and child
    /*
        ---------            ------
        |Parents| ---pipe--> |Bash|
        ---------            ------

        ---------            ------
        |Parents| <--pipe--- |Bash|
        ---------            ------
    */
    // set 2 pipes for use
    int parentToChild[(int)PIPE_TYPE::NUMS];
    int childToParent[(int)PIPE_TYPE::NUMS];

    pid_t pid;

    // create pipes 
    int nCreateP2C = pipe(parentToChild);
    int nCreateC2P = pipe(childToParent);
    if (nCreateC2P == -1)
    {
        perror("Create C2P error!");
        return 0;
    }

    if (nCreateP2C == -1)
    {
         perror("Create P2C error!");
        return 0;
    }



    pid = fork();
    
    if (pid == -1)
    {
        perror("fork error");
        return 1;
    }
    
    if (pid == 0)
    {
        // chiled proc  
        closeFdForC(parentToChild, childToParent);
        
        dupPipesBashProc(parentToChild, childToParent);
     
        execl("/bin/bash", "bash", NULL);
        
        perror("execl Error");
        return 1;
    }
    else
    {
        
        // parents proc
        closeFdForP(parentToChild, childToParent);
        // shellLoop
        shellLoop(parentToChild, childToParent);
        // kill sub proc
        kill(pid, SIGKILL);
    }
    return 0;
}


void dupPipesBashProc(int* p2C, int* c2P)
{

    // redirect to stderr stdin stdout
    dup2(p2C[(int)PIPE_TYPE::READ], STDIN_FILENO);
    dup2(c2P[(int)PIPE_TYPE::WRITE], STDOUT_FILENO);
    dup2(c2P[(int)PIPE_TYPE::WRITE], STDERR_FILENO);

    close(p2C[(int)PIPE_TYPE::READ]);
    close(c2P[(int)PIPE_TYPE::WRITE]);
}

void closeFdForC(int* p2C, int* c2P)
{
    // close un usage pipes
    close(p2C[(int)PIPE_TYPE::WRITE]);
    close(c2P[(int)PIPE_TYPE::READ]);
}

void closeFdForP(int* p2C, int* c2P)
{
    close(p2C[(int)PIPE_TYPE::READ]);
    close(c2P[(int)PIPE_TYPE::WRITE]);    
}

void setNoblock(int* c2P)
{
    int flags = fcntl(c2P[(int)PIPE_TYPE::READ], F_GETFL, 0);
    fcntl(c2P[(int)PIPE_TYPE::READ], F_SETFL, flags | O_NONBLOCK);
}

void shellLoop(int* p2C, int* c2P)
{
    char szBuffer[BUFFER_RECV]{ 0 };
    std::string strCmd;
    setNoblock(c2P);
    fd_set readFds;

    while (true)
    {
        // get cmd from user
        std::cout << "$>";
        std::fflush(stdin);
        std::getline(std::cin, strCmd);
    
        if (strCmd == "exit")
        {
            // exit the shell
            break;
        }

        // set enter 
        strCmd += "\n";

        // write cmd to child
        write(p2C[(int)PIPE_TYPE::WRITE], strCmd.c_str(), strCmd.size() + 1);

        // use fd to monitor the pipe result
        FD_ZERO(&readFds);
        FD_SET(c2P[(int)PIPE_TYPE::READ], &readFds);

        struct timeval timeout{0, 1};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int nRet = select(c2P[(int)PIPE_TYPE::READ] + 1, &readFds, NULL, NULL, &timeout);
        
        if (nRet > 0 && FD_ISSET(c2P[(int)PIPE_TYPE::READ], &readFds))
        {
            ssize_t nRead;
            while (true)
            {
                nRead = read(c2P[(int)PIPE_TYPE::READ], szBuffer, 511);
                if (nRead > 0)
                {
                    
                    szBuffer[nRead] = '\0';
                    std::cout << szBuffer;
                    std::fflush(stdin);
                }
                else
                {
                    break;
                }
                
            }
        }
        else if (nRet == 0)
        {
            //perror("shell error");
            continue;
        }

    }

    close(p2C[(int)PIPE_TYPE::WRITE]);
    close(c2P[(int)PIPE_TYPE::READ]);
}