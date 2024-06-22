#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>

class Shell {
public:
    void run() {
        while (true) {
            std::cout << "Shell> ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "exit") {
                break;
            }
            if (!input.empty()) {
                executeCommand(input);
            }
        }
    }

    void executeCommand(const std::string& command) {
        // 将命令拆分为参数
        std::vector<std::string> args = splitCommand(command);
        if (args.empty()) {
            return;
        }

        // 内建命令处理（如 cd）
        if (args[0] == "cd") {
            if (args.size() < 2) {
                std::cerr << "cd: expected argument" << std::endl;
            } else {
                if (chdir(args[1].c_str()) != 0) { // 切换到cd后面所指的目录
                    perror("cd failed");
                }
            }
            return;
        }

        if (args[0] == "source") {
            if (args.size() < 2) {
                std::cerr << "source: expected filename" << std::endl;
            } else {
                executeScript(args[1]);
            }
            return;
        }

        // 处理管道
        std::vector<std::string> commands = splitPipeline(command);
        int numCommands = commands.size();

        if (numCommands > 1) {
            executePipeline(commands);
        } else {
            executeSingleCommand(command);
        }
    }

    std::vector<std::string> splitCommand(const std::string& command) {
        std::vector<std::string> args;
        std::istringstream iss(command);
        std::string token;
        while (iss >> token) {
            args.push_back(token);
        }
        return args;
    }

private:
    void executeSingleCommand(const std::string& command) {
        std::vector<std::string> args = splitCommand(command);
        char* argv[args.size() + 1];
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = const_cast<char*>(args[i].c_str());
        }
        argv[args.size()] = nullptr;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            // 子进程
            handleRedirection(args);
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else {
            // 父进程
            int status;
            waitpid(pid, &status, 0);
        }
    }

    void handleRedirection(std::vector<std::string>& args) {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == ">") {
                if (i + 1 < args.size()) {
                    int fd = open(args[i + 1].c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    if (fd < 0) {
                        perror("open failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    args.erase(args.begin() + i, args.begin() + i + 2);
                    --i;
                }
            } else if (args[i] == ">>") {
                if (i + 1 < args.size()) {
                    int fd = open(args[i + 1].c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
                    if (fd < 0) {
                        perror("open failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    args.erase(args.begin() + i, args.begin() + i + 2);
                    --i;
                }
            } else if (args[i] == "<") {
                if (i + 1 < args.size()) {
                    int fd = open(args[i + 1].c_str(), O_RDONLY);
                    if (fd < 0) {
                        perror("open failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                    args.erase(args.begin() + i, args.begin() + i + 2);
                    --i;
                }
            }
        }
    }

    std::vector<std::string> splitPipeline(const std::string& command) {
        std::vector<std::string> commands;
        std::istringstream iss(command);
        std::string token;
        while (std::getline(iss, token, '|')) {
            commands.push_back(token);
        }
        return commands;
    }
    void executePipeline(const std::vector<std::string>& commands) {
        int numCommands = commands.size();
        int pipefd[2];
        pid_t pid;
        int fd_in = 0;

        for (int i = 0; i < numCommands; ++i) {
            pipe(pipefd);
            if ((pid = fork()) == -1) {
                perror("fork failed");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                dup2(fd_in, 0);
                if (i < numCommands - 1) {
                    dup2(pipefd[1], 1);
                }
                close(pipefd[0]);

                std::vector<std::string> args = splitCommand(commands[i]);
                handleRedirection(args);
                char* argv[args.size() + 1];
                for (size_t j = 0; j < args.size(); ++j) {
                    argv[j] = const_cast<char*>(args[j].c_str());
                }
                argv[args.size()] = nullptr;

                execvp(argv[0], argv);
                perror("execvp failed");
                exit(EXIT_FAILURE);
            } else {
                wait(nullptr);
                close(pipefd[1]);
                fd_in = pipefd[0];
            }
        }
    }

    void executeScript(const std::string& script) {
        std::ifstream file(script);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << script << std::endl;
            return;
        }
        // 读取脚本文件每行命令并执行
        std::string line;
        while (std::getline(file, line)) {
            executeCommand(line);
        }

        file.close();
    }
};

int main(int argc, char* argv[]) {
    Shell shell;
    if (argc > 1) {
        shell.executeCommand("source " + std::string(argv[1]));
    } else {
        shell.run();
    }
    return 0;
}
