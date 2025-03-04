#include <workspace/workspace.hpp>

#include "rabbitmq_worker.hpp"
#include "compile_settings.h"

#include "compile_interface.h"
#include "cpp_compile.hpp"
#include "c_compile.hpp"
#include "java_compile.hpp"
#include "python_compile.hpp"
#include "verilog_compile.hpp"
#include "lua_compile.hpp"

void work_func(CompileInterface *compileImpl, json &taskData);

int main() {
    cout << "Hello JudgeCompile!" << endl;

    RabbitMQPull mqWorker;

    // 线程池配置
    wsp::workspace spc;
    auto workbranch = spc.attach(new wsp::workbranch);
    // 最小线程数 最大线程数 时间间隔
    auto supervisor = spc.attach(new wsp::supervisor(5, 10, 1000));
    spc[supervisor].supervise(spc[workbranch]);
    
    cout << "Start to Listen!" << endl;
    while(true) {
        CompileInterface *compileImpl = nullptr;
        json taskData;

        if(!mqWorker.pullTaskData(taskData)) {
            continue;
        }

        std::string taskID = taskData["task"]["id"];
        cout << "Deal with Task: " << taskID << endl;

        // 取出taskData.task.answer.language
        std::string language = taskData["task"]["answer"]["language"];
        // 根据language字段选择对应的编译实例
        if(language == "C")
            compileImpl = new CCompile();
        else if(language == "C++")
            compileImpl = new CppCompile();
        else if(language == "Java")
            compileImpl = new JavaCompile();
        else if(language == "Python")
            compileImpl = new PythonCompile();
        else if(language == "Verilog")
            compileImpl = new VerilogCompile();
        else if(language == "Lua")
            compileImpl = new LuaCompile();
        else {
            std::cerr << "Unsupported language: " << language << std::endl;
            continue;
        }
        
        // 提交工作线程
        spc.submit([compileImpl, &taskData] {
            work_func(compileImpl, taskData);
        });
    }
}

void work_func(CompileInterface *compileImpl, json &taskData) {
    std::string taskID = taskData["task"]["id"];
    cout << "Work with Task: " << taskID << endl;
    try {
        compileImpl->save(taskData);
        compileImpl->compile(taskData);
        compileImpl->transcode(taskData);
    }
    catch (compile_error &e) {// 编译错误
        taskData["task"]["status"] = "CE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = e.what();
    }
    catch (std::runtime_error &e) {// 运行时错误
        taskData["task"]["status"] = "RE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = e.what();
    }
    catch (std::exception &e) {// 按未知错误处理
        taskData["task"]["status"] = "UKE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = e.what();
    }

    // 回送TaskData
    RabbitMQPush mqWorker;
    mqWorker.pushTaskData(taskData);

    cout << "Finish Task: " << taskID << endl;
    // 在线程完成时释放资源
    delete compileImpl;
}