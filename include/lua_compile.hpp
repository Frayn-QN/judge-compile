#include <fstream>
#include <boost/filesystem.hpp>

#include "compile_interface.h"
#include "compile_settings.h"
#include "file_methods.hpp"

class LuaCompile : public CompileInterface {
private:
public:
    LuaCompile() {};
    ~LuaCompile() {};

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

        // answer
        std::ofstream answerFile("main.lua");
        if(!answerFile) {
            throw std::runtime_error("Cannot open file for writing");
        }
        answerFile << answer["code"].get<std::string>();    
    }

    void compile(json &taskData) override {
        // do nothing
    }

    void transcode(json &taskData) override {
        std::string id = taskData["task"]["id"];

        fs::path dir(FILE_ROOT_PATH + id);
        if(!fs::exists(dir)) {
            throw std::runtime_error("Directory not exists");
        }

        // main.lua
        fs::path mainPath(dir / "main.lua");
        if(!fs::exists(mainPath)) {
            throw std::runtime_error("File not exists");
        }
        std::string base64str;
        Base64::EncodeFileToBase64(mainPath, base64str);
        // 放入TaskData.task.result
        json result;
        result["main.lua"] = base64str;
        taskData["task"]["result"].push_back(result);
        // 转移extra中模块文件
        taskData["task"]["result"].insert(taskData["task"]["result"].end(),
            taskData["extra"].begin(), taskData["extra"].end());
    }
};