#pragma once

#include <fstream>
#include <boost/filesystem.hpp>
#include <sys/wait.h>

#include "compile_settings.h"
#include "compile_interface.h"
#include "file_methods.hpp"

class CCompile : public CompileInterface {
private:
public:
    CCompile() {};
    ~CCompile() {};

    void save(json &taskData) override {
        json task = taskData["task"];
        json answer = task["answer"];
        json extra = taskData["extra"];
        std::string id = task["id"];
        
        fs::path dir(FILE_ROOT_PATH + id);
        if(!fs::exists(dir)) {
            if(!fs::create_directories(dir))
                throw std::runtime_error("Cannot create directory");
        }
        else // 由于id的唯一性，理论上不触发
            throw std::runtime_error("Directory already exists");

        for(auto &item : extra.items()) {// 保存附加文件
            std::string key = item.key();
            std::string value = item.value();
            fs::path filePath(dir / key);

            Base64::DecodeBase64ToFile(value, filePath);
        }

        // 从字符串保存answer代码
        std::ofstream answerFile("main.c");
        if(!answerFile) {
            throw std::runtime_error("Cannot open file for writing");
        }
        answerFile << answer["code"].get<std::string>();
    }

    void compile(json &taskData) override {
        json extra = taskData["extra"];
        std::string id = taskData["task"]["id"];

        fs::path dir(FILE_ROOT_PATH + id);
        if(!fs::exists(dir)) {
            throw std::runtime_error("Directory not exists");
        }

        // gcc参数
        std::vector<const char*> args;
        args.push_back("gcc");
        args.push_back("-o");
        args.push_back("main");
        args.push_back("main.c");
        for(auto &item: extra.items()) {
            std::string key = item.key();
            std::string suffix = key.substr(key.find_last_of('.') + 1);
            if(suffix == "c") {
                args.push_back(key.c_str());
            }
        }
        args.push_back("-std=c11");
        args.push_back(nullptr);

        // 创建管道
        int pipefd[2];
        if(pipe(pipefd) == -1) {
            throw std::runtime_error("Pipe failed");
        }
        // 子进程编译
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
            if (chdir(dir.c_str()) != 0) {
                std::cerr << "切换目录失败: " << dir << std::endl;
                _exit(1);
            }

            execvp("gcc", const_cast<char* const*>(args.data()));

            // execvp 失败
            std::cerr << "执行编译命令失败" << std::endl;
            _exit(1);
        }
        else if(pid > 0) {// 父进程
            close(pipefd[1]);// 关闭写端

            int status;
            waitpid(pid, &status, 0);
            if(status != 0) {
                close(pipefd[0]);// 抛出异常前关闭读端
                throw std::runtime_error("Compile failed");
            }

            // 读取编译错误信息
            char buffer[1024];
            std::string compileError;
            while(read(pipefd[0], buffer, sizeof(buffer)) > 0) {
                compileError += buffer;
            }
            close(pipefd[0]);
            if(!compileError.empty()) {
                throw compile_error(compileError);
            }
        }
    }

    void transcode(json &taskData) override {
        std::string id = taskData["task"]["id"];

        fs::path dir(FILE_ROOT_PATH + id);
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