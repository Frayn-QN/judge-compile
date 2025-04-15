#pragma once

#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include "compile_settings.h"

using namespace AmqpClient;

// 推送由主线程完成；拉取由工作线程完成，每个线程一个通道

class RabbitMQPull {
private:
    std::string queue_input = "CompileQueueInput";
    Channel::ptr_t channel_input;

public:
    RabbitMQPull() // 声明队列
    : channel_input(Channel::Create(MQ_HOST, MQ_PORT, MQ_USER, MQ_PASSWORD))
    {
        channel_input->DeclareQueue(queue_input, false, true, false, false);
    };

    ~RabbitMQPull() {};

    /**
     * @brief 拉取任务数据 
     * @param taskData 任务数据
     * @return 是否成功
     */
    bool pullTaskData(json &taskData) {
        std::string consume_tag = channel_input->BasicConsume(
            queue_input, "", true,
            true,/* no_ack */
            false/* exclusive */
        );
        Envelope::ptr_t envelope;
        bool flag = channel_input->BasicConsumeMessage(consume_tag, envelope);
        channel_input->BasicCancel(consume_tag);  // 释放消费者
        if(!flag) return false;

        std::string message = envelope->Message()->Body();
        taskData = json::parse(message);
        return true;
    };
};

class RabbitMQPush {
private:
    std::string queue_output = "CompileQueueOutput";
    Channel::ptr_t channel_output;

public:
    RabbitMQPush() // 声明队列
    : channel_output(Channel::Create(MQ_HOST, MQ_PORT, MQ_USER, MQ_PASSWORD))
    {
        channel_output->DeclareQueue(queue_output, false, true, false, false);
    };

    ~RabbitMQPush() {};

    /**
     * @brief 推送任务数据
     */
    void pushTaskData(const json &taskData) {
        BasicMessage::ptr_t message = BasicMessage::Create(taskData.dump());
        channel_output->BasicPublish("", queue_output, message);
    }
};