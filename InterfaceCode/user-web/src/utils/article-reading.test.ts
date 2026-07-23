import { describe, expect, it } from 'vitest'

import { buildTableOfContents } from './article-reading'

describe('buildTableOfContents', () => {
  it('assigns stable anchors to article headings', () => {
    const root = document.createElement('article')
    root.innerHTML = '<h2>开始</h2><h3>细节</h3>'
    expect(buildTableOfContents(root)).toEqual([
      { id: 'section-1', text: '开始', level: 2 },
      { id: 'section-2', text: '细节', level: 3 },
    ])
  })
})
