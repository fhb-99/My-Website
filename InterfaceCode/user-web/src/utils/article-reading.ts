export interface TocItem { id: string; text: string; level: 2 | 3 }

export function buildTableOfContents(root: HTMLElement): TocItem[] {
  return Array.from(root.querySelectorAll<HTMLHeadingElement>('h2, h3')).map((heading, index) => {
    const id = `section-${index + 1}`
    heading.id = id
    return { id, text: heading.textContent?.trim() || id, level: heading.tagName === 'H2' ? 2 : 3 }
  })
}
