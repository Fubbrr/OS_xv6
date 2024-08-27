#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//在ls.c基础上修改
void find(char *path, char *file)
{   
    //文件名缓冲区
    char buf[512], *p;
    //文件描述符
    int fd;
   
    struct dirent de;
    struct stat st;

    //open() 函数打开路径，返回一个文件描述符
    //错误返回 -1
    if ((fd = open(path, 0)) < 0)
    {
        //无法打开此路径
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    //fstat()返回文件的结点中的所有信息,获得一个已存在文件的模式，并将此模式赋值给它的副本
    //错误返回 -1
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        //关闭文件描述符 fd
        close(fd);
        return;
    }
    
    switch(st.type)
    {
        //目录类型不对
        case T_FILE:
            fprintf(2, "find: %s is not a directory\n", path);
            break;
        //目录类型正确
        case T_DIR:
            //路径过长放不入缓冲区
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
            {
                fprintf(2, "find: path too long\n");
                break;
            }
            //将path指向的字符串即绝对路径复制到buf
            strcpy(buf, path);
            //加 "/" 前缀
            p = buf + strlen(buf);
            *p++ = '/';
            //读取fd
            //判断read返回字节数与de长度相等
            while(read(fd, &de, sizeof(de)) == sizeof(de))
            {
                if(de.inum == 0)
                    continue;
                //不要递归为 “.” 和 “..”
                //字符串比较函数
                if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
                    continue;
                //把文件名信息复制p
                memmove(p, de.name, DIRSIZ);
                //设置文件名结束符
                p[DIRSIZ] = 0;
                // stat 以文件名作为参数，返回一个已存在文件的模式，并将此模式赋值给它的副本
                //出错，则返回 -1
                if(stat(buf, &st) < 0)
                {
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                //目录类型
                if (st.type == T_DIR)
                {
                    //递归查找
                    find(buf, file);
                }
                //文件类型，查找成功
                else if (st.type == T_FILE && !strcmp(de.name, file))
                {
                    //缓冲区存放的路径
                    printf("%s\n", buf);
                } 
            }
        break;
    }
    //关闭文件描述符
    close(fd);
    return;
}


int main(int argc, char *argv[])
{
  //如果参数不是3个
  if(argc != 3)
  {
    write(2, "Incorrect number of characters\n", strlen("Incorrect number of characters\n"));
    exit(1);
  }
  else
  {
    //寻找函数
    find(argv[1], argv[2]);
    exit(0);
  }
}

