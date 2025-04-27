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

void work_func(json taskData);

int main()
{
    std::cout << getCurrentTime() << "Hello JudgeCompile!" << endl;

    RabbitMQPull mqWorker;

    // 线程池配置
    wsp::workspace spc;
    auto brh_id = spc.attach(new wsp::workbranch);
    // 最小线程数 最大线程数 时间间隔
    auto spv_id = spc.attach(new wsp::supervisor(10, 100, 1000));
    spc[spv_id].supervise(spc[brh_id]);

    cout << getCurrentTime() << "Start to Listen!" << endl;

    while (true)
    {
        json taskData;
        try
        {
            if (!mqWorker.pullTaskData(taskData))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }
        catch (const AmqpClient::ChannelException &e)
        {
            cout << e.what() << endl;
            continue;
        }

        // 提交工作线程
        spc.submit([taskData]
                   {
            // 值传递，生成副本，避免循环继续使得生命周期结束
            work_func(taskData); });
    }
}

void work_func(json taskData)
{
    CompileInterface *compileImpl = nullptr;

    std::string taskID = taskData["task"]["id"];
    std::cout << getCurrentTime() << "Deal with Task: " << taskID << endl;

    // 取出taskData.task.answer.language
    std::string language = taskData["task"]["answer"]["language"];
    // 根据language字段选择对应的编译实例
    if (language == "C")
        compileImpl = new CCompile(taskData);
    else if (language == "C++")
        compileImpl = new CppCompile(taskData);
    else if (language == "Java")
        compileImpl = new JavaCompile(taskData);
    else if (language == "Python")
        compileImpl = new PythonCompile(taskData);
    else if (language == "Verilog")
        compileImpl = new VerilogCompile(taskData);
    else if (language == "Lua")
        compileImpl = new LuaCompile(taskData);
    else
    {
        std::cerr << getCurrentTime() << "Unsupported language: " << language << std::endl;
        return;
    }

    std::cout << getCurrentTime() << "Work with Task: " << taskID << endl;
    try
    {
        compileImpl->save();
        compileImpl->compile();
        compileImpl->transcode();
    }
    catch (compile_error &e)
    { // 编译错误
        taskData["task"]["status"] = "CE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = e.what();
        std::cerr << "编译错误: " << e.what() << std::endl;
    }
    catch (std::runtime_error &e)
    { // 运行时错误
        taskData["task"]["status"] = "RE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = e.what();
        std::cerr << "运行时异常: " << e.what() << std::endl;
    }
    catch (const std::string &msg)
    { // 按未知错误处理
        taskData["task"]["status"] = "UKE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = msg;
        std::cerr << "字符串异常: " << msg << std::endl;
    }
    catch (const std::exception &e)
    { // 按未知错误处理
        taskData["task"]["status"] = "UKE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = e.what();
        std::cerr << "标准异常: " << e.what() << std::endl;
    }
    catch (...)
    { // 未知错误
        taskData["task"]["status"] = "UKE";
        taskData["task"]["result"].clear();
        taskData["task"]["result"]["msg"] = "Unknown Error";
        std::cerr << "Unknown Error" << std::endl;
    }

    std::cout << getCurrentTime() << "Push back task: " << taskID << endl;

    // 回送TaskData
    RabbitMQPush mqWorker;
    mqWorker.pushTaskData(taskData);

    std::cout << getCurrentTime() << "Finish Task: " << taskID << endl;
    // 释放指针
    delete compileImpl;
}