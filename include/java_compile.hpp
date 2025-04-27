#pragma once

#include <fstream>
#include <boost/filesystem.hpp>
#include <sys/wait.h>

#include "compile_interface.h"
#include "compile_settings.h"
#include "file_methods.hpp"

class JavaCompile : public CompileInterface
{
private:
    json &taskData;
    json &task;
    std::string taskID;
    fs::path taskDir;

public:
    JavaCompile(json &taskData) : taskData(taskData),
                                  task(taskData["task"])
    {
        taskID = task["id"];
        taskDir = FILE_ROOT_PATH + taskID;

        if (!fs::exists(taskDir))
        { // 创建任务目录
            if (!fs::create_directories(taskDir))
                throw std::runtime_error("Cannot create directory");
        }
        else // 由于id的唯一性，理论上不触发
            throw std::runtime_error("Directory already exists");
    };

    ~JavaCompile() override
    {
        // 移除任务目录及内容
        fs::remove_all(taskDir);
    };

    void save() override
    {
        json answer = task["answer"];
        json extra = taskData["extra"];

        // 保存附加文件
        saveFromJsonList(extra, taskDir);

        // 从字符串保存answer代码
        std::ofstream answerFile(fs::path(taskDir / "Main.java"));
        if (!answerFile)
        {
            throw std::runtime_error("Cannot open file for writing");
        }
        answerFile << answer["code"].get<std::string>();
    }

    void compile() override
    {
        // javac
        std::vector<const char *> args;
        args.push_back("javac");
        args.push_back("*.java"); // 编译所有java文件
        args.push_back(nullptr);

        // pipe
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            throw std::runtime_error("Pipe failed");
        }

        // fork
        pid_t pid = fork();
        if (pid == -1)
        {
            close(pipefd[0]);
            close(pipefd[1]);
            throw std::runtime_error("Fork failed");
        }
        else if (pid == 0)
        {                     // 子进程
            close(pipefd[0]); // 关闭读端
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            // 切换到目标目录
            if (chdir(taskDir.c_str()) != 0)
            {
                std::cerr << "切换目录失败: " << taskDir << std::endl;
                _exit(1);
            }

            execvp("javac", const_cast<char *const *>(args.data()));

            // execvp 失败
            std::cerr << "执行编译命令失败" << std::endl;
            _exit(1);
        }
        else if (pid > 0)
        {                     // 父进程
            close(pipefd[1]); // 关闭写端

            int status;
            waitpid(pid, &status, 0); // 等待子进程结束

            // 读取错误信息
            char buffer[1024];
            std::string compileError;
            while (read(pipefd[0], buffer, sizeof(buffer)) > 0)
            {
                compileError += buffer;
            }
            close(pipefd[0]);

            if (status != 0)
            { // 子进程本身出错
                throw std::runtime_error("Compile failed");
            }
            if (!compileError.empty())
            { // 编译器报错
                throw compile_error(compileError);
            }
        }
    }

    void transcode() override
    {
        // transcode .class
        fs::directory_iterator endItr;
        for (fs::directory_iterator itr(taskDir); itr != endItr; itr++)
        {
            if (fs::is_regular_file(itr->status()))
            {
                fs::path filePath = itr->path();
                std::string fileName = filePath.filename().string();
                std::string extension = filePath.extension().string();

                if (extension == ".class")
                {
                    json resultItem;
                    std::string base64Str;
                    Base64::EncodeFileToBase64(filePath, base64Str);

                    // 放入TaskData.task.result
                    resultItem[fileName] = base64Str;
                    taskData["task"]["result"].push_back(resultItem);
                }
            }
        }
    }
};