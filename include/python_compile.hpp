#pragma once

#include <fstream>
#include <boost/filesystem.hpp>

#include "compile_interface.h"
#include "compile_settings.h"
#include "file_methods.hpp"

class PythonCompile : public CompileInterface {
private:
    json &taskData;
    json &task;
    std::string taskID;
    fs::path taskDir;
public:
    PythonCompile(json &taskData):
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

    ~PythonCompile() {
        // 移除任务目录及内容
        fs::remove_all(taskDir);
    };

    void save() override {
        json answer = task["answer"];
        json extra = taskData["extra"];

        // answer
        std::ofstream answerFile(fs::path(taskDir / "main.py"));
        if(!answerFile) {
            throw std::runtime_error("Cannot open file for writing");
        }
        answerFile << answer["code"].get<std::string>();    
    }

    void compile() override {
        // do nothing
    }

    void transcode() override {
        std::string id = taskData["task"]["id"];

        // main.py
        fs::path mainPath(taskDir / "main.py");
        if(!fs::exists(mainPath)) {
            throw std::runtime_error("File not exists");
        }
        std::string base64str;
        Base64::EncodeFileToBase64(mainPath, base64str);
        // 放入TaskData.task.result
        json result;
        result["main.py"] = base64str;
        taskData["task"]["result"].push_back(result);
        // 转移extra中模块文件
        taskData["task"]["result"].insert(taskData["task"]["result"].end(),
            taskData["extra"].begin(), taskData["extra"].end());
    }
};