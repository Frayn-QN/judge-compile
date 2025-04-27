#pragma once

#include "compile_settings.h"
#include "file_methods.hpp"

class CompileInterface
{
public:
    // 发生错误抛出异常

    /**
     * @brief 在文件系统保存
     */
    virtual void save() = 0;

    /**
     * @brief 编译（非必须）
     */
    virtual void compile() = 0;

    /**
     * @brief 转码
     */
    virtual void transcode() = 0;

    virtual ~CompileInterface() {};

    /**
     * @brief 向指定目录下保存文件
     * @param list Json List
     * @param dirPath 父目录
     */
    void saveFromJsonList(json &list, fs::path dirPath)
    {
        if (!list.is_array())
            throw std::runtime_error("wrong json type: not array");

        for (auto &element : list)
        {
            if (!element.is_object())
                throw std::runtime_error("array element is not an object");

            auto item = element.begin();
            if (item == element.end())
                throw std::runtime_error("array element has no key-value pair");

            std::string key = item.key();
            std::string value = item.value();

            fs::path filePath(dirPath / key);
            Base64::DecodeBase64ToFile(value, filePath);
        }
    }
};