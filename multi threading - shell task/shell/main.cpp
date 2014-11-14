#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <fcntl.h>
#include <memory>
#include <signal.h>

using namespace std;

struct MemoryKeeper {
    const char **args;
    MemoryKeeper(const char **a) : args(a) { }
    ~MemoryKeeper() { free(args); }
};

vector<string> parse_string(const string &s)
{
    vector<string> result;
    string cur;
    for (size_t i = 0; i < s.length(); ++i) {
        if ((s[i] == ' ') && (!(cur.empty()))) {
            result.push_back(cur);
            cur.clear();
        } else {
            cur += s[i];
        }
    }
    if (!cur.empty()) result.push_back(cur);
    return result;
}

enum Flags {
    PARALL = 1, REDIR = 2, PIPE = 4
};

int where_cut(vector<string> &parameters)
{
    int cut_to = parameters.size();
    for (size_t i = 0; i < parameters.size(); ++i) {
        if ((parameters[i] == "&") || (parameters[i] == ">") || (parameters[i] == "|")) {
            cut_to = i;
            break;
        }
    }
    return cut_to;
}

int shell_flags(vector<string> &parameters)
{
    int result = 0;
    int cut_to = where_cut(parameters);
    for (size_t i = cut_to; i < parameters.size(); ++i) {
        if (parameters[i] == "&") result |= PARALL;
        else if (parameters[i] == "|") result |= PIPE;
        else if (parameters[i] == ">") result |= REDIR;
    }
    return result;
}

//Returns vector of commands to pipe
vector<vector<string> > parse_pipes(const vector<string> &parameters)
{
    vector<vector<string> > res;
    vector<string> buffer;
    for (size_t i = 0; i < parameters.size(); ++i) {
        if ((parameters[i] == "|") && (!buffer.empty())) {
            res.push_back(buffer);
            buffer.clear();
        } else {
            buffer.push_back(parameters[i]);
        }
    }
    if (!buffer.empty()) {
        res.push_back(buffer);
    }
    return res;
}

vector<int> pids;
int main_pid = getpid();

void handler(int s) {
    signal(s, handler);
    if (getpid() != main_pid) return;
    cout << "In handler" << endl;
    if (s == SIGCHLD) {
        cout << "SIGCHLD received" << endl;
        int res;
        cout << "PID = " << wait(&res) << ", exit = ";
        cout << res << endl;
    } else if (s == SIGINT) {
        cout << "SIGINT received" << endl;
        for (size_t i = 0; i < pids.size(); ++i) {
            kill(pids[i], SIGKILL);
        }
        raise(SIGKILL);
    }
}

int main()
{
    signal(SIGCHLD, handler);
    signal(SIGINT, handler);
    string command_line;
    while (getline(cin, command_line)) {
        auto commands = parse_pipes(parse_string(command_line));
        if (!(shell_flags(commands.back()) & PARALL)) signal(SIGCHLD, SIG_DFL);
        else signal(SIGCHLD, handler);
        int prev[2], next[2];
        if (commands.size() > 1) {
            pipe(prev);
            pipe(next);
        }
        pids.clear();
        for (size_t i = 0; i < commands.size(); ++i) {
            int pid;
            if (commands.size() > 1) {
                if (i == 0) {
                    close(prev[0]);
                    close(prev[1]);
                    prev[0] = -1;
                    prev[1] = -1;
                } else if (i == commands.size() - 1) {
                    if (prev[0] != -1) close(prev[0]);
                    if (prev[1] != -1) close(prev[1]);
                    prev[0] = next[0];
                    prev[1] = next[1];
                    next[0] = -1;
                    next[1] = -1;
                } else {
                    if (prev[0] != -1) close(prev[0]);
                    if (prev[1] != -1) close(prev[1]);
                    prev[0] = next[0];
                    prev[1] = next[1];
                    pipe(next);
                }
            }
            int flags = shell_flags(commands[i]);
            if ((pid = fork()) == 0) {
                cout << "PID = " << getpid() << endl;
                if (commands.size() > 1) {
                    if (i == 0) {
                        close(1);
                        dup2(next[1], 1);
                        close(next[0]);
                        close(next[1]);
                    } else if (i == commands.size() - 1) {
                        close(0);
                        dup2(prev[0], 0);
                        close(prev[1]);
                        close(prev[0]);
                    } else {
                        close(1);
                        close(0);
                        dup2(prev[0], 0);
                        dup2(next[1], 1);
                        close(prev[1]);
                        close(next[0]);
                        close(prev[0]);
                        close(next[1]);
                    }
                }
                if (i == commands.size() - 1) {
                    if (flags & REDIR) {
                        string path = commands[i].back();
                        int fd = open(path.c_str(), O_WRONLY | O_APPEND);
                        close(1);
                        dup2(fd, 1);
                        commands[i].erase(commands[i].end() - 2, commands[i].end());
                    }
                }
                if (flags & PARALL) {
                    commands[i].erase(commands[i].end() - 1);
                }
                const char **args = (const char **)malloc(commands[i].size() + 1);
                MemoryKeeper keeper(args);
                for (size_t j = 0; j < commands[i].size(); ++j) {
                    args[j] = commands[i][j].c_str();
                }
                args[commands[i].size()] = NULL;

                return execvp(args[0], (char * const *)args);
            }
            pids.push_back(pid);
            if ((i == commands.size() - 1) && (commands.size() > 1)) {
                close(prev[0]);
                close(prev[1]);
            }
        }
        if (!(shell_flags(commands.back()) & PARALL)) {
            for (size_t i = 0; i < commands.size(); ++i) {
                int res = 0;
                cout << "PID = " << wait(&res) << ", exit = " << res << endl;
            }
        }
    }
    return 0;
}

