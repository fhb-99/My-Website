// tools/import_post.cc — 将本地 .md 文件导入数据库
//
// 用法: ./import_post <文件.md> [选项]
//   ./import_post hello-world.md
//   ./import_post hello-world.md --draft      导入为草稿
//   ./import_post hello-world.md --slug xxx   自定义 URL 标识
//
// 约定：
//   1. 文件名去掉后缀 = slug（如 "hello-world.md" → slug="hello-world"）
//   2. 第一行 # 开头的 = 标题
//   3. 第二段（第一个空行后的段落）= 摘要
//   4. cover.jpg 和文章同名放在同目录 = 封面图（可选）

#include "SQLiteCpp/SQLiteCpp.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>

// ===== 工具函数 =====

// 读整个文件
static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "错误: 无法打开文件 " << path << std::endl;
        exit(1);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// 提取第一行 # 标题
static std::string extract_title(const std::string& md) {
    auto nl = md.find('\n');
    std::string first_line = md.substr(0, nl);
    // 去掉开头的 "# "
    if (first_line.size() >= 2 && first_line[0] == '#' && first_line[1] == ' ')
        return first_line.substr(2);
    return first_line;
}

// 提取摘要：跳过标题行和空行后，取第一段的前 200 字
static std::string extract_summary(const std::string& md) {
    size_t start = md.find('\n');
    if (start == std::string::npos) return "";
    start++;  // 跳过标题行
    while (start < md.size() && md[start] == '\n') start++;  // 跳过空行

    size_t end = md.find("\n\n", start);
    if (end == std::string::npos) end = md.size();
    std::string para = md.substr(start, std::min(size_t(200), end - start));
    return para;
}

// 确认文件存在
static bool file_exists(const std::string& path) {
    struct stat buf;
    return stat(path.c_str(), &buf) == 0;
}

// ===== 主函数 =====
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: import_post <文件.md> [选项]" << std::endl;
        std::cerr << "  --draft     导入为草稿（不发布）" << std::endl;
        std::cerr << "  --slug xxx  自定义 URL 标识" << std::endl;
        std::cerr << std::endl;
        std::cerr << "示例:" << std::endl;
        std::cerr << "  import_post hello-world.md" << std::endl;
        std::cerr << "  import_post hello-world.md --draft" << std::endl;
        return 1;
    }

    // 解析参数
    std::string filepath = argv[1];
    bool is_published = true;
    std::string custom_slug;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--draft") == 0)
            is_published = false;
        else if (strcmp(argv[i], "--slug") == 0 && i + 1 < argc)
            custom_slug = argv[++i];
    }

    // 提取文件名（去掉路径和 .md 后缀）
    auto slash = filepath.find_last_of("/\\");
    auto dot   = filepath.find_last_of('.');
    std::string filename = (slash == std::string::npos)
        ? filepath.substr(0, dot)
        : filepath.substr(slash + 1, dot - slash - 1);

    std::string slug = custom_slug.empty() ? filename : custom_slug;

    // 读 Markdown 内容
    std::string md = read_file(filepath);
    if (md.empty()) {
        std::cerr << "错误: 文件内容为空" << std::endl;
        return 1;
    }

    // 提取元数据
    std::string title   = extract_title(md);
    std::string summary = extract_summary(md);

    // 检查封面图（和 .md 同目录下的 .jpg/.png）
    std::string cover_url;
    for (auto ext : {".jpg", ".png", ".webp"}) {
        std::string cover_path = filepath.substr(0, filepath.find_last_of('.')) + ext;
        if (file_exists(cover_path)) {
            // 复制到 uploads/ 目录
            std::string dest = "../uploads/" + filename + ext;
            std::ifstream src(cover_path, std::ios::binary);
            std::ofstream dst(dest, std::ios::binary);
            dst << src.rdbuf();
            std::cout << "封面图: " << dest << std::endl;
            cover_url = "/uploads/" + filename + ext;
            break;
        }
    }

    // 打开数据库
    SQLite::Database db("../data/blog.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    // 检查 slug 是否已存在
    SQLite::Statement check(db, "SELECT COUNT(*) FROM posts WHERE slug = ?");
    check.bind(1, slug);
    check.executeStep();
    if (check.getColumn(0).getInt() > 0) {
        std::cerr << "错误: slug '" << slug << "' 已存在，用 --slug 指定其他名称" << std::endl;
        return 1;
    }

    // 插入数据库
    SQLite::Statement query(db, R"(
        INSERT INTO posts (title, slug, summary, content_md, cover_url, is_published)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    query.bind(1, title);
    query.bind(2, slug);
    query.bind(3, summary);
    query.bind(4, md);
    query.bind(5, cover_url);
    query.bind(6, is_published ? 1 : 0);
    query.exec();

    int new_id = db.getLastInsertRowid();

    std::cout << std::endl;
    std::cout << "✓ 导入成功!" << std::endl;
    std::cout << "  ID:      " << new_id << std::endl;
    std::cout << "  标题:    " << title << std::endl;
    std::cout << "  URL:     /post.html?id=" << new_id << std::endl;
    std::cout << "  状态:    " << (is_published ? "已发布" : "草稿") << std::endl;
    if (!cover_url.empty())
        std::cout << "  封面:    " << cover_url << std::endl;

    return 0;
}
