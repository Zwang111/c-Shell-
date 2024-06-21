#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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
            if(! input.empty()){
            executeCommand(input);
            }
        }
    }

public:
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
                if (chdir(args[1].c_str()) != 0) {
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
        // 将参数转换为 C 风格的数组
        char* argv[args.size() + 1];
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = const_cast<char*>(args[i].c_str());
        }
        argv[args.size()] = nullptr;

        // 创建子进程并执行命令
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            // 子进程
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else {
            // 父进程
            int status;
            waitpid(pid, &status, 0);
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

int main(int argc,char* argv[]) {
    Shell shell;
    if(argc > 1){
        shell.executeCommand("source " + std::string(argv[1]));
    }else{
        shell.run();
    }
    return 0;
}
