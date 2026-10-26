export function formatDateTime(value: string) {
  if (!value) return ''

  // PocketBase 返回 ISO 时间，公开页面统一保留到秒，避免显示毫秒和 Z 后缀。
  return value.replace('T', ' ').replace(/\.\d+Z?$/, '').replace(/Z$/, '').slice(0, 19)
}
