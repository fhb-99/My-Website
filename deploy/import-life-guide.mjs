#!/usr/bin/env node

import { execFileSync } from 'node:child_process'
import { readFile, readdir } from 'node:fs/promises'
import path from 'node:path'

const GUIDE_SLUG = 'how-to-live-better'
const GUIDE_TITLE = '高性价比人生指南'
const GUIDE_SUMMARY = '用较少的钱、时间和精力，查阅健康、金钱、法律、工作与生活选择中的实用建议。'
const SOURCE_URL = 'https://github.com/eternity4719/HowToLiveBetter'
const LICENSE_URL = 'https://creativecommons.org/licenses/by/4.0/'
const EXPECTED_CHAPTERS = 34

function parseArgs(argv) {
  const options = { apply: false, publish: false, source: '', url: '' }
  for (let index = 0; index < argv.length; index += 1) {
    const value = argv[index]
    if (value === '--apply') options.apply = true
    else if (value === '--publish') options.publish = true
    else if (value === '--source') options.source = argv[++index] || ''
    else if (value === '--url') options.url = argv[++index] || ''
    else throw new Error(`未知参数：${value}`)
  }
  return options
}

function sourceVersion(sourceRoot) {
  if (process.env.GUIDE_SOURCE_VERSION?.trim()) return process.env.GUIDE_SOURCE_VERSION.trim()
  try {
    return execFileSync('git', ['-C', sourceRoot, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim()
  } catch {
    throw new Error('无法读取来源版本，请设置 GUIDE_SOURCE_VERSION')
  }
}

function encodeSourcePath(value) {
  const [filePath, fragment] = value.split('#', 2)
  const encodedPath = filePath.split('/').map((part) => encodeURIComponent(part)).join('/')
  return fragment ? `${encodedPath}#${encodeURIComponent(fragment)}` : encodedPath
}

function transformMarkdown(source, version) {
  const sourceBase = `${SOURCE_URL}/blob/${encodeURIComponent(version)}`
  return source
    .replace(/^\uFEFF/u, '')
    .replace(/^\[← 回总目录\]\(\.\.\/README\.md\)\s*/u, '')
    .replace(/^#\s+\d+\.\s+.+(?:\r?\n)+/u, '')
    // 原文注释用于检索页生成，不属于读者需要看到的正文。
    .replace(/<!--[^]*?-->\s*/gu, '')
    // 博客没有复制原仓库 docs 目录，相对链接固定指向本次导入的来源版本。
    .replace(/\]\(\.\.\/([^)]+)\)/gu, (_match, target) => `](${sourceBase}/${encodeSourcePath(target)})`)
    .trim()
    .concat('\n')
}

async function readChapters(bookDir, version) {
  const names = (await readdir(bookDir))
    .filter((name) => /^\d{2}-.+\.md$/u.test(name))
    .sort((left, right) => left.localeCompare(right, 'zh-CN', { numeric: true }))

  if (names.length !== EXPECTED_CHAPTERS) {
    throw new Error(`预期 ${EXPECTED_CHAPTERS} 个章节，实际找到 ${names.length} 个`)
  }

  return Promise.all(names.map(async (name) => {
    const match = name.match(/^(\d{2})-(.+)\.md$/u)
    const raw = await readFile(path.join(bookDir, name), 'utf8')
    const heading = raw.match(/^#\s+\d+\.\s+(.+)$/mu)
    if (!match || !heading) throw new Error(`无法识别章节文件：${name}`)
    return {
      chapter_no: Number(match[1]),
      title: heading[1].trim(),
      slug: name.slice(0, -3),
      content_md: transformMarkdown(raw, version),
    }
  }))
}

async function request(baseUrl, route, options = {}) {
  const response = await fetch(`${baseUrl}${route}`, options)
  const data = await response.json().catch(() => null)
  if (!response.ok) {
    const details = Object.entries(data?.data || {})
      .map(([field, detail]) => `${field}：${detail?.message || '字段无效'}`)
    const message = details.length
      ? details.join('；')
      : data?.message || `PocketBase 请求失败（${response.status}）`
    throw new Error(message)
  }
  return data
}

function recordsPath(collection, query) {
  const base = `/api/collections/${encodeURIComponent(collection)}/records`
  return query ? `${base}?${query}` : base
}

async function findFirst(baseUrl, token, collection, filter) {
  const query = new URLSearchParams({ page: '1', perPage: '1', filter })
  const result = await request(baseUrl, recordsPath(collection, query), {
    headers: { Authorization: token },
  })
  return result.items[0] || null
}

async function saveRecord(baseUrl, token, collection, id, body) {
  const route = recordsPath(collection) + (id ? `/${encodeURIComponent(id)}` : '')
  return request(baseUrl, route, {
    method: id ? 'PATCH' : 'POST',
    headers: { Authorization: token, 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  })
}

async function adminToken(baseUrl) {
  if (process.env.POCKETBASE_ADMIN_TOKEN?.trim()) return process.env.POCKETBASE_ADMIN_TOKEN.trim()
  const identity = process.env.POCKETBASE_ADMIN_EMAIL?.trim()
  const password = process.env.POCKETBASE_ADMIN_PASSWORD
  if (!identity || !password) {
    throw new Error('执行写入需要设置 POCKETBASE_ADMIN_TOKEN，或同时设置 POCKETBASE_ADMIN_EMAIL 与 POCKETBASE_ADMIN_PASSWORD')
  }
  const result = await request(baseUrl, '/api/collections/_superusers/auth-with-password', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ identity, password }),
  })
  return result.token
}

async function importGuide(baseUrl, token, chapters, version, publish) {
  const guideBody = {
    title: GUIDE_TITLE,
    slug: GUIDE_SLUG,
    summary: GUIDE_SUMMARY,
    source_url: SOURCE_URL,
    license_url: LICENSE_URL,
    source_version: version,
  }
  const existingGuide = await findFirst(baseUrl, token, 'guides', `slug = ${JSON.stringify(GUIDE_SLUG)}`)
  const guide = await saveRecord(baseUrl, token, 'guides', existingGuide?.id, existingGuide
    ? guideBody
    : { ...guideBody, is_published: false })
  let created = 0
  let updated = 0

  for (const chapter of chapters) {
    const filter = `guide = ${JSON.stringify(guide.id)} && chapter_no = ${chapter.chapter_no}`
    const existing = await findFirst(baseUrl, token, 'guide_chapters', filter)
    const body = {
      guide: guide.id,
      ...chapter,
      ...(existing ? (publish ? { is_published: true } : {}) : { is_published: publish }),
    }
    await saveRecord(baseUrl, token, 'guide_chapters', existing?.id, body)
    if (existing) updated += 1
    else created += 1
  }

  // 所有章节保存成功后才公开指南，避免读者看到导入到一半的目录。
  if (publish) await saveRecord(baseUrl, token, 'guides', guide.id, { is_published: true })
  return { created, updated, guideId: guide.id }
}

async function main() {
  const options = parseArgs(process.argv.slice(2))
  const bookDir = path.resolve(options.source || process.env.GUIDE_SOURCE_DIR || '')
  if (!options.source && !process.env.GUIDE_SOURCE_DIR) {
    throw new Error('请使用 --source 指定 HowToLiveBetter/book 目录，或设置 GUIDE_SOURCE_DIR')
  }
  const rootDir = path.dirname(bookDir)
  const version = sourceVersion(rootDir)
  const chapters = await readChapters(bookDir, version)
  const bytes = chapters.reduce((total, chapter) => total + Buffer.byteLength(chapter.content_md), 0)

  console.log(`已校验 ${chapters.length} 个章节，共 ${(bytes / 1024 / 1024).toFixed(2)} MiB`)
  console.log(`来源版本：${version}`)
  console.log(`章节范围：${chapters[0].slug} → ${chapters.at(-1).slug}`)

  if (!options.apply) {
    console.log('当前为校验模式，未写入 PocketBase；确认后增加 --apply。')
    return
  }

  const baseUrl = (options.url || process.env.POCKETBASE_URL || '').trim().replace(/\/+$/, '')
  if (!baseUrl) throw new Error('执行写入需要使用 --url 或 POCKETBASE_URL 指定 PocketBase 地址')
  const token = await adminToken(baseUrl)
  const result = await importGuide(baseUrl, token, chapters, version, options.publish)
  console.log(`导入完成：新建 ${result.created} 章，更新 ${result.updated} 章，指南 ID ${result.guideId}`)
  console.log(options.publish ? '指南与本次导入的章节已发布。' : '未改变已有发布状态；新建内容保持草稿。')
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : error)
  process.exitCode = 1
})
