#include <iostream>
#include <algorithm>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <sys/epoll.h>

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <set>

#define MAX_EVENTS 32

using namespace std;

struct MyFriend {
    string ip;
    string buffer;
    int fd;
    MyFriend(int f) : ip(), buffer(), fd(f) { }
    bool operator==(const MyFriend &f) { return f.fd == fd; }
    bool operator!=(const MyFriend &f) { return f.fd != fd; }
};

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

int main()
{
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(MasterSocket == -1)
    {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int Result = bind(MasterSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr));

    if(Result == -1)
    {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    set_nonblock(MasterSocket);

    Result = listen(MasterSocket, SOMAXCONN);

    if(Result == -1)
    {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    struct epoll_event Event;
    Event.data.fd = MasterSocket;
    Event.events = EPOLLIN | EPOLLET;

    struct epoll_event * Events;
    Events = (struct epoll_event *) calloc(MAX_EVENTS, sizeof(struct epoll_event));

    int EPoll = epoll_create1(0);
    epoll_ctl(EPoll, EPOLL_CTL_ADD, MasterSocket, &Event);

    vector<MyFriend> friends;
    string buff;

    while(true)
    {
        int N = epoll_wait(EPoll, Events, MAX_EVENTS, -1);
        for(unsigned int i = 0; i < N; i++)
        {
            if((Events[i].events & EPOLLERR)||(Events[i].events & EPOLLHUP))
            {
                shutdown(Events[i].data.fd, SHUT_RDWR);
                //friends.erase(friends.find(Events[i].data.fd));
                close(Events[i].data.fd);
            }
            else if(Events[i].data.fd == MasterSocket)
            {
                cout << "In accept" << endl;
                sockaddr_in client;
                socklen_t len = sizeof(sockaddr_in);
                int SlaveSocket = accept(MasterSocket, (sockaddr*)(&client), &len);
                set_nonblock(SlaveSocket);

                struct epoll_event Event;
                Event.data.fd = SlaveSocket;
                Event.events = EPOLLIN | EPOLLET;

                epoll_ctl(EPoll, EPOLL_CTL_ADD, SlaveSocket, &Event);
                MyFriend newfriend(SlaveSocket);
                newfriend.ip = string(inet_ntoa(client.sin_addr));
                friends.push_back(newfriend);
            }
            else
            {
                static char Buffer[1024];
                int RecvSize = recv(Events[i].data.fd, Buffer, 1024, MSG_NOSIGNAL);
                auto cur_frnd = find(friends.begin(), friends.end(), Events[i].data.fd);
                Buffer[RecvSize] = 0;
                string mes = (*cur_frnd).ip + ": " + string(Buffer);
                buff += mes;
                fflush(stdout);

                /* It is for simple chat without clients-----------------------------------
                 *
                 * (*cur_frnd).buffer += mes;
                if (find(mes.begin(), mes.end(), '\n') != mes.end()) {
                    for (auto it = friends.begin(); it != friends.end(); ++it) {
                        if (it != cur_frnd) {
                            send((*it).fd, (*cur_frnd).buffer.c_str(), RecvSize, MSG_NOSIGNAL);
                        }
                    }
                    (*cur_frnd).buffer.clear();
                }--------------------------------------------------------------------------*/
                for (auto it = friends.begin(); it != friends.end(); ++it) {
                    send((*it).fd, buff.c_str(), strlen(buff.c_str()), MSG_NOSIGNAL);
                }
            }
        }
    }

    return 0;
}

