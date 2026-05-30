# tools/import_post.py — 将本地 .md 文件导入 SQLite 数据库
#
# 用法: python import_post.py <文件.md> [选项]
#   python import_post.py ../content/posts/hello-world.md
#   python import_post.py ../content/posts/hello-world.md --draft
#   python import_post.py ../content/posts/hello-world.md --slug 自定义slug
#
# 约定：
#   1. 文件名去掉后缀 = slug
#   2. 第一行 # 开头的 = 标题
#   3. 前 200 字 = 摘要
#   4. 同名 .jpg/.png 文件 = 封面图

import sqlite3
import sys
import os
import shutil
import re

# Windows 控制台 UTF-8 支持
if sys.platform == 'win32':
    sys.stdout.reconfigure(encoding='utf-8')

def extract_title(md_text):
    """取第一行 # 标题"""
    first_line = md_text.strip().split('\n')[0]
    return re.sub(r'^#+\s*', '', first_line).strip()

def extract_summary(md_text, max_len=200):
    """跳过标题行和空行后，取第一段的前 max_len 字"""
    lines = md_text.split('\n')
    start = 1
    while start < len(lines) and lines[start].strip() == '':
        start += 1
    para = []
    for line in lines[start:]:
        if line.strip() == '':
            break
        para.append(line)
    text = ' '.join(para).strip()
    return text[:max_len]

def find_cover(filepath):
    """查找和 .md 同名的 .jpg/.png/.webp 封面图"""
    base = os.path.splitext(filepath)[0]
    for ext in ['.jpg', '.jpeg', '.png', '.webp']:
        if os.path.exists(base + ext):
            return base + ext
    return None

def main():
    if len(sys.argv) < 2:
        print("用法: python import_post.py <文件.md> [选项]")
        print("  --draft     导入为草稿（不发布）")
        print("  --slug xxx  自定义 URL 标识")
        print()
        print("示例:")
        print("  python import_post.py ../content/posts/hello-world.md")
        return 1

    filepath = sys.argv[1]
    is_published = True
    custom_slug = None

    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--draft':
            is_published = False
        elif sys.argv[i] == '--slug' and i + 1 < len(sys.argv):
            custom_slug = sys.argv[i + 1]
            i += 1
        i += 1

    if not os.path.exists(filepath):
        print(f"错误: 文件不存在 - {filepath}")
        return 1

    # 读 Markdown
    with open(filepath, 'r', encoding='utf-8') as f:
        md_text = f.read()

    if not md_text.strip():
        print("错误: 文件内容为空")
        return 1

    # 提取元数据
    filename = os.path.splitext(os.path.basename(filepath))[0]
    slug = custom_slug or filename
    title = extract_title(md_text)
    summary = extract_summary(md_text)

    # 处理封面图
    cover_url = ''
    cover = find_cover(filepath)
    if cover:
        ext = os.path.splitext(cover)[1]
        dest = os.path.join('..', 'uploads', filename + ext)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        shutil.copy2(cover, dest)
        cover_url = '/uploads/' + filename + ext
        print(f"[OK] 封面图: {dest}")

    # 打开数据库（自动创建）
    db_path = os.path.join('..', 'data', 'blog.db')
    os.makedirs(os.path.dirname(db_path), exist_ok=True)
    conn = sqlite3.connect(db_path)
    conn.execute("PRAGMA journal_mode=WAL;")

    # 建表（如果不存在）
    conn.execute('''
        CREATE TABLE IF NOT EXISTS posts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            slug TEXT NOT NULL UNIQUE,
            summary TEXT DEFAULT '',
            content_md TEXT NOT NULL,
            content_html TEXT DEFAULT '',
            cover_url TEXT DEFAULT '',
            tags TEXT DEFAULT '[]',
            is_published INTEGER DEFAULT 0,
            views INTEGER DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        )
    ''')

    # 检查 slug 是否已存在
    cur = conn.execute("SELECT COUNT(*) FROM posts WHERE slug = ?", (slug,))
    if cur.fetchone()[0] > 0:
        print(f"错误: slug '{slug}' 已存在，用 --slug 指定其他名称")
        conn.close()
        return 1

    # 插入
    conn.execute('''
        INSERT INTO posts (title, slug, summary, content_md, cover_url, is_published)
        VALUES (?, ?, ?, ?, ?, ?)
    ''', (title, slug, summary, md_text, cover_url, 1 if is_published else 0))
    conn.commit()

    new_id = conn.execute("SELECT last_insert_rowid()").fetchone()[0]
    conn.close()

    print()
    print("[OK] 导入成功!")
    print(f"  ID:      {new_id}")
    print(f"  标题:    {title}")
    print(f"  URL:     /post.html?id={new_id}")
    print(f"  状态:    {'已发布' if is_published else '草稿'}")
    if cover_url:
        print(f"  封面:    {cover_url}")

    return 0

if __name__ == '__main__':
    sys.exit(main())
