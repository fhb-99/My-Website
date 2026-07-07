#include "middleware/deepseek_moderation.h"

#include "third_party/httplib.h"
#include "third_party/json.hpp"

#include <cstdlib>
#include <exception>
#include <string>

using json = nlohmann::json;

namespace {

std::string TrimConfigValue(const char* value)
{
    if (!value) {
        return "";
    }

    std::string text = value;
    while (!text.empty()) {
        const char ch = text[0];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            break;
        }
        text.erase(0, 1);
    }

    while (!text.empty()) {
        const char ch = text[text.size() - 1];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            break;
        }
        text.erase(text.size() - 1);
    }

    if (text.size() >= 2 &&
        ((text[0] == '"' && text[text.size() - 1] == '"') ||
         (text[0] == '\'' && text[text.size() - 1] == '\''))) {
        text = text.substr(1, text.size() - 2);
    }

    return text;
}

} // namespace

ModerationClient::ModerationClient()
    : api_host_("api.deepseek.com"),
      api_path_("/chat/completions"),
      model_("deepseek-chat")
{
    const std::string env_key = TrimConfigValue(std::getenv("DEEPSEEK_API_KEY"));
    if (!env_key.empty()) {
        api_key_ = env_key;
    }

    const std::string env_model = TrimConfigValue(std::getenv("DEEPSEEK_MODEL"));
    if (!env_model.empty()) {
        model_ = env_model;
    }

    const std::string env_base = TrimConfigValue(std::getenv("DEEPSEEK_API_BASE_URL"));
    if (env_base.empty()) {
        return;
    }

    std::string base_url = env_base;
    const std::string https_prefix = "https://";
    const std::string http_prefix = "http://";

    if (base_url.find(https_prefix) == 0) {
        base_url = base_url.substr(https_prefix.size());
    } else if (base_url.find(http_prefix) == 0) {
        // DeepSeek 线上接口必须走 HTTPS；HTTP 配置视为无效，保留默认 host。
        return;
    }

    while (!base_url.empty() && base_url[base_url.size() - 1] == '/') {
        base_url.erase(base_url.size() - 1);
    }

    const std::size_t slash_pos = base_url.find('/');
    if (slash_pos == std::string::npos) {
        api_host_ = base_url;
        api_path_ = "/chat/completions";
        return;
    }

    api_host_ = base_url.substr(0, slash_pos);
    api_path_ = base_url.substr(slash_pos);
    const std::string chat_path = "/chat/completions";
    if (api_path_.size() < chat_path.size() ||
        api_path_.substr(api_path_.size() - chat_path.size()) != chat_path) {
        api_path_ += chat_path;
    }
}

bool ModerationClient::IsConfigured() const
{
    return !api_host_.empty() && !api_key_.empty() && !model_.empty();
}

bool ModerationClient::Moderate(const std::string& content,
                                const std::string& target_type,
                                const ModerationConfig& config,
                                ModerationDecision& decision,
                                std::string& error) const
{
    if (!IsConfigured()) {
        error = "DeepSeek environment variables are not configured";
        return false;
    }

    httplib::SSLClient client(api_host_.c_str(), 443);
    client.set_address_family(AF_INET);
    client.set_connection_timeout(5);
    client.set_read_timeout(20, 0);
    client.enable_server_certificate_verification(true);

    const std::string request_model = config.model.empty() ? model_ : config.model;
    const std::string system_prompt = config.system_prompt.empty()
        ? "你是个人博客评论审核助手，请判断内容是否适合公开展示，并返回结构化 JSON。"
        : config.system_prompt;

    json request_body;
    request_body["model"] = request_model;
    request_body["response_format"] = {{"type", "json_object"}};
    request_body["messages"] = json::array({
        {
            {"role", "system"},
            {"content",
             system_prompt +
             "\n请只返回 JSON，不要返回 Markdown 或额外解释。"
             "JSON 格式必须是："
             "{\"decision\":\"approved|pending|rejected\",\"reason\":\"简短原因\",\"confidence\":0.0}。"
             "approved 表示可以公开展示，pending 表示需要人工复核，rejected 表示不适合公开展示。"}
        },
        {
            {"role", "user"},
            {"content", "内容类型：" + target_type + "\n待审核内容：\n" + content}
        }
    });

    httplib::Headers headers = {
        {"Authorization", std::string("Bearer ") + api_key_},
        {"Content-Type", "application/json"}
    };

    auto response = client.Post(api_path_.c_str(), headers, request_body.dump(), "application/json");
    if (!response) {
        error = std::string("failed to request DeepSeek API: ") +
            httplib::to_string(response.error()) +
            " host=" + api_host_ +
            " path=" + api_path_;
        return false;
    }

    if (response->status < 200 || response->status >= 300) {
        error = "DeepSeek API returned HTTP " + std::to_string(response->status);
        return false;
    }

    try {
        const json outer = json::parse(response->body);
        if (!outer.contains("choices") || !outer["choices"].is_array() || outer["choices"].empty()) {
            error = "DeepSeek response has no choices";
            return false;
        }

        const json& message = outer["choices"][0]["message"];
        if (!message.contains("content") || !message["content"].is_string()) {
            error = "DeepSeek response has no message content";
            return false;
        }

        const json parsed = json::parse(message["content"].get<std::string>());
        const std::string model_decision = parsed.value("decision", "pending");
        if (model_decision != "approved" && model_decision != "pending" && model_decision != "rejected") {
            error = "DeepSeek returned invalid decision";
            return false;
        }

        decision.decision = model_decision;
        decision.reason = parsed.value("reason", "DeepSeek moderation completed");
        decision.confidence = parsed.value("confidence", 0.5);
        decision.source = "ai";

        if (decision.confidence < 0.0) {
            decision.confidence = 0.0;
        }
        if (decision.confidence > 1.0) {
            decision.confidence = 1.0;
        }

        return true;
    } catch (const std::exception& e) {
        error = std::string("failed to parse DeepSeek response: ") + e.what();
        return false;
    }
}
