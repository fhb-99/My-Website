#pragma once

#include <string>
#include "models/global.h"

class ModerationClient
{
public:
    // 启动时读取 DeepSeek 相关环境变量；未配置时不影响服务启动。
    ModerationClient();

    bool Moderate(const std::string& content,
                  const std::string& target_type,
                  const ModerationConfig& config,
                  ModerationDecision& decision,
                  std::string& error) const;

    bool IsConfigured() const;

private:
    std::string api_host_;
    std::string api_path_;
    std::string api_key_;
    std::string model_;
};
