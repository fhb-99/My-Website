import { describe, expect, it } from 'vitest'

import { formatDateTime } from './date-time'

describe('formatDateTime', () => {
  it.each([
    ['2026-09-04 14:46:41.589Z', '2026-09-04 14:46:41'],
    ['2026-09-04T14:46:41.589Z', '2026-09-04 14:46:41'],
    ['2026-09-04T14:46:41Z', '2026-09-04 14:46:41'],
    ['2026-09-04', '2026-09-04'],
    ['', ''],
  ])('将 %s 格式化为 %s', (value, expected) => {
    expect(formatDateTime(value)).toBe(expected)
  })
})
