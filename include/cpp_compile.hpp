#pragma once

#include <fstream>
#include <boost/filesystem.hpp>
#include <sys/wait.h>

#include "compile_interface.h"
#include "compile_settings.h"
#include "file_methods.hpp"

class CppCompile : public CompileInterface {
private:
    json &taskData;
    json &task;
    std::string taskID;
    fs::path taskDir;
public:
    CppCompile(json &taskData):
        taskData(taskData),
        task(taskData["task"])
    {
        taskID = task["id"];
        taskDir = FILE_ROOT_PATH + taskID;

        if(!fs::exists(taskDir)) {// 创建任务目录
            if(!fs::create_directories(taskDir))
                throw std::runtime_error("Cannot create directory");
        }
        else // 由于id的唯一性，理论上不触发
            throw std::runtime_error("Directory already exists");
    };

    ~CppCompile() {
        // 移除任务目录及内容
        fs::remove_all(taskDir);
    };

    void save() override {
        json answer = task["answer"];
        json extra = taskData["extra"];

        for(auto &item : extra.items()) {// 保存附加文件
            std::string key = item.key();
            std::string value = item.value();
            fs::path filePath(taskDir / key);

            Base64::DecodeBase64ToFile(value, filePath);
        }

        // 从字符串保存answer代码
        std::ofstream answerFile(fs::path(taskDir / "main.cpp"));
        if(!answerFile) {
            throw std::runtime_error("Cannot open file for writing");
        }
        answerFile << answer["code"].get<std::string>();
    }

    void compile() override {
        json extra = taskData["extra"];

        // 构造 g++ 参数数组
        std::vector<const char*> args;
        args.push_back("g++");
        args.push_back("-o");
        args.push_back("main");
        args.push_back("main.cpp");
        for(auto &item: extra.items()) {
            std::string key = item.key();

            std::string suffix = key.substr(key.find_last_of('.') + 1);
            if(suffix == "cpp" || suffix == "hpp") {// 附加编译参数
                args.push_back(key.c_str());
            }
        }
        args.push_back(nullptr);  // execvp 要求以 nullptr 结尾

        // 创建管道用于获取编译失败信息
        int pipefd[2];
        if(pipe(pipefd) == -1) {
            throw std::runtime_error("Pipe failed");
        }

        // 使用子进程编译
        pid_t pid = fork();
        if(pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            throw std::runtime_error("Fork failed");
        }
        else if(pid == 0) {// 子进程
            close(pipefd[0]);// 关闭读端
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            // 切换到目标目录
            if (chdir(taskDir.c_str()) != 0) {
                std::cerr << "切换目录失败: " << taskDir << std::endl;
                _exit(1);
            }

            execvp("g++", const_cast<char* const*>(args.data()));

            // execvp 失败
            std::cerr << "执行编译命令失败" << std::endl;
            _exit(1);
        }
        else if(pid > 0) {// 父进程
            close(pipefd[1]);// 关闭写端

            int status;
            waitpid(pid, &status, 0);// 等待子进程结束

            // 读取错误信息
            char buffer[1024];
            std::string compileError;
            while(read(pipefd[0], buffer, sizeof(buffer)) > 0) {
                compileError += buffer;
            }
            close(pipefd[0]);

            if(status != 0) {// 子进程本身出错
                throw std::runtime_error("Compile failed");
            }
            if(!compileError.empty()) {// 编译器报错
                throw compile_error(compileError);
            }
        }
    }

    void transcode() override {
        fs::path dir(FILE_ROOT_PATH + taskID);
        if(!fs::exists(dir)) {
            throw std::runtime_error("Directory not exists");
        }

        // main可执行文件转码
        fs::path mainPath(dir / "main");
        if(!fs::exists(mainPath)) {
            throw std::runtime_error("File not exists");
        }
        std::string base64str;
        Base64::EncodeFileToBase64(mainPath, base64str);
        // 放入TaskData.task.result
        json result;
        result["main"] = base64str;
        taskData["task"]["result"].push_back(result);
    }
};