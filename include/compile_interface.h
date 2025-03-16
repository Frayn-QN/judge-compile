#pragma once

#include "compile_settings.h"

class CompileInterface {
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
};