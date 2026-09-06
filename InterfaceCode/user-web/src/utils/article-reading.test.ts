import { describe, expect, it } from 'vitest'

import {
  buildTableOfContents,
  clampReaderTocWidth,
  READER_TOC_DEFAULT_WIDTH,
  READER_TOC_MAX_WIDTH,
  READER_TOC_MIN_WIDTH,
} from './article-reading'

describe('buildTableOfContents', () => {
  it('assigns stable anchors to article headings', () => {
    const root = document.createElement('article')
    root.innerHTML = '<h2>开始</h2><h3>细节</h3><h4>补充方法</h4>'
    expect(buildTableOfContents(root)).toEqual([
      { id: 'section-1', text: '开始', level: 2 },
      { id: 'section-2', text: '细节', level: 3 },
      { id: 'section-3', text: '补充方法', level: 4 },
    ])
  })
})

describe('clampReaderTocWidth', () => {
  it('keeps the dragged width inside the readable desktop range', () => {
    expect(clampReaderTocWidth(120)).toBe(READER_TOC_MIN_WIDTH)
    expect(clampReaderTocWidth(READER_TOC_DEFAULT_WIDTH)).toBe(READER_TOC_DEFAULT_WIDTH)
    expect(clampReaderTocWidth(900)).toBe(READER_TOC_MAX_WIDTH)
  })

  it('reserves minimum space for the article on a narrower desktop', () => {
    expect(clampReaderTocWidth(480, 900)).toBe(326)
  })
})
