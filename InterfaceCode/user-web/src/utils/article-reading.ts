export interface TocItem { id: string; text: string; level: 2 | 3 | 4 }

export const READER_TOC_DEFAULT_WIDTH = 286
export const READER_TOC_MIN_WIDTH = 220
export const READER_TOC_MAX_WIDTH = 480
const READER_MAIN_MIN_WIDTH = 560
const READER_RESIZER_WIDTH = 14

/**
 * 限制目录宽度，既避免目录被压得无法阅读，也为正文保留基本空间。
 */
export function clampReaderTocWidth(width: number, layoutWidth = Number.POSITIVE_INFINITY) {
  const availableMax = Number.isFinite(layoutWidth)
    ? layoutWidth - READER_MAIN_MIN_WIDTH - READER_RESIZER_WIDTH
    : READER_TOC_MAX_WIDTH
  const maximum = Math.max(READER_TOC_MIN_WIDTH, Math.min(READER_TOC_MAX_WIDTH, availableMax))
  return Math.round(Math.min(maximum, Math.max(READER_TOC_MIN_WIDTH, width)))
}

export function buildTableOfContents(root: HTMLElement): TocItem[] {
  return Array.from(root.querySelectorAll<HTMLHeadingElement>('h2, h3, h4')).map((heading, index) => {
    const id = `section-${index + 1}`
    heading.id = id
    return { id, text: heading.textContent?.trim() || id, level: Number(heading.tagName.slice(1)) as TocItem['level'] }
  })
}

/**
 * Hash 路由会占用地址栏中的 #，目录只能通过 DOM 滚动定位。
 */
export function scrollToHeading(id: string) {
  document.getElementById(id)?.scrollIntoView({ behavior: 'smooth', block: 'start' })
}
