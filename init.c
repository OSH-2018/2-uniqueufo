#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MAX 256
#define LEN 256
struct args_list      //shell指令单个管道结构体
{
    int argc;   //单个管道参数个数
    char *argv[MAX];
}*args[MAX];//shell指令
int pipe_num;//shell管道个数
int backstage;//是否为后台处理命令标记

void split_args(char *line)  //切分管道
{
    char *store;
    char * shell_cmd = strtok_r(line, "|", &store);
    while (shell_cmd)
    {
        struct args_list * cmd = (struct args_list *)malloc(sizeof(struct args_list));
        args[pipe_num++] = cmd;
        cmd->argc = 0;
        char *save;
        char *arg = strtok_r(shell_cmd, " \t", &save);//处理空格
        while (arg)
        {
            cmd->argv[cmd->argc] = arg;
            arg = strtok_r(NULL, " \t", &save);
            cmd->argc++;
        }
        cmd->argv[cmd->argc] = NULL;
        shell_cmd = strtok_r(NULL, "|", &store);//继续切分
    }
}


int built_in(char *line)  //执行内部指令
{
    char *save,*tmp[MAX];
    char t[LEN],p[LEN];
    strcpy(t,line);
    char *arg = strtok_r(line, " \t", &save);//切分空格
    int i=0;
    while (arg)
    {
        tmp[i] = arg;
        i++;//记录命令个数
        arg = strtok_r(NULL, " \t", &save);
    }
    tmp[i] = NULL;
    if (strcmp(tmp[i-1],"&")==0)//判断是否为后台处理命令
    {
        backstage=1;
        i--;
    }
    else if (strcmp(tmp[0],"exit")==0)//exit
    {
        exit(0);
        return 1;
    }
    else if (strcmp(tmp[0],"pwd")==0)//pwd
    {
        char buf[LEN];
        getcwd(buf,sizeof(buf));//得到当前路径
        printf("%s\n",buf);
        return 1;
    }
    else if (strcmp(tmp[0],"cd")==0)//cd
    {
        char buf[LEN];
        if(tmp[1]==NULL)return 1;
        if (chdir(tmp[1])>=0);
        else
        {
            printf("Error path!\n");
        }
        return 1;
    }
    else if(strcmp(tmp[0],"export")==0)//export改变环境变量
    {
        char buf[LEN];char *result,*value = NULL;
        char *character = strchr(tmp[1],61);
        int set_;
        if(character)//参数中含有赋值号
        {
            result = strtok(tmp[1],"=");
            value = strtok(NULL,"=");
            set_ = setenv(result,value,1);
            if(set_== -1)printf("export failure\n");
        }
        else printf("export failure\n");
        return 1;
    }

    else return 0;
}


void execute(char *argv[])  //执行外部命令列表
{
    int error;
    error=execvp(argv[0],argv);
    if (error==-1)  printf("failed!\n");
    exit(1);
}
void pipe_execute(int index)  //执行管道命令
{
    if (index == pipe_num - 1)//如果没有管道，直接执行外部命令
    {
        execute(args[index]->argv);
    }
    int pipe_fd[2];
    pipe(pipe_fd);//创建管道
    int cpid = fork();
    if(cpid == -1)
    {
        perror("fork error");
        exit(0);
    }
    if (cpid == 0)//fork成功
    {
        dup2(pipe_fd[1], 1);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execute(args[index]->argv);//执行外部命令
    }
    dup2(pipe_fd[0], 0);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    pipe_execute(index + 1);//迭代执行pipe命令
}

int main()
{
    int pid;
    char buf[LEN],p[LEN];
    char buffer[LEN];
    while (1)
    {
        getcwd(buffer,sizeof(buffer));
        printf("myshell:(%s)$",buffer);
        fgets(buf,LEN,stdin);//读入shell指令
        if (buf[0]=='\n')
            continue;
        buf[strlen(buf)-1]='\0';
        strcpy(p,buf);
        int built_in_flag;
        built_in_flag=built_in(buf);//内置指令执行
        if (built_in_flag==0)
        {
            pid=fork();//建立新的进程
            if (pid==0)
            {
                split_args(p);//切割管道
                if (strcmp(args[pipe_num-1]->argv[args[pipe_num-1]->argc-1],"&")==0)         //如果是后台处理命令将&符号删除
                {
                    args[pipe_num-1]->argc--;
                    args[pipe_num-1]->argv[args[pipe_num-1]->argc]=NULL;
                }
                if (args[0]->argv[args[0]->argc-1]!=NULL)   //输入保证第一个管道不为空
                {
                    char q[LEN];
                    if(args[0]->argc>3)
                    {
                        if(strcmp(args[0]->argv[args[0]->argc-2],"<")==0)//输入重定向
                        {
                            strcpy(q,args[0]->argv[args[0]->argc-1]);
                            int pipe_fd;pipe_fd=open(q,O_RDONLY);
                            args[0]->argv[args[0]->argc-2]=NULL;  //默认重定向为参数的最后一个
                            args[0]->argc-=2;
                            if (pipe_fd==-1)
                            {
                                printf("file open failed\n");
                            }
                            dup2(pipe_fd,0);
                            close(pipe_fd);
                        }
                    }
                }
                if (args[pipe_num-1]->argv[args[pipe_num-1]->argc-1]!=NULL)//输出保证最后一个管道不为空
                {
                    char q[LEN];
                    if(args[pipe_num-1]->argc>2)//参数个数大于两个，可能为重定向命令
                    {
                        if(strcmp(args[pipe_num-1]->argv[args[pipe_num-1]->argc-2],">")==0)//输出重定向（到文件）
                        {
                            strcpy(q,args[pipe_num-1]->argv[args[pipe_num-1]->argc-1]);
                            int pipe_fd;
                            args[pipe_num-1]->argv[args[pipe_num-1]->argc-2]=NULL;
                            args[pipe_num-1]->argc-=2;
                            pipe_fd=open(q,O_CREAT|O_RDWR,666);
                            if (pipe_fd==-1)
                            {
                                printf("file open failed\n");
                            }
                            dup2(pipe_fd,1);
                            close(pipe_fd);
                        }
                        else if(strcmp(args[pipe_num-1]->argv[args[pipe_num-1]->argc-2],">>")==0)//输出重定向（到文件末尾）
                        {
                            strcpy(q,args[pipe_num-1]->argv[args[pipe_num-1]->argc-1]);
                            int pipe_fd;
                            args[pipe_num-1]->argv[args[pipe_num-1]->argc-2]=NULL;
                            args[pipe_num-1]->argc-=2;
                            pipe_fd=open(q,O_CREAT|O_RDWR|O_APPEND,666);
                            if (pipe_fd==-1)
                            {
                                printf("file open failed\n");
                            }
                            dup2(pipe_fd,1);
                            close(pipe_fd);
                        }
                    }
                }
                pipe_execute(0);
                exit(0);
            }
            if (backstage==0)//等待子进程处理
                waitpid(pid,NULL,0);
        }
    }
}

