import { describe, it, expect } from 'vitest';
import { EventNames } from '../bridge/eventNames';

// Self-policing tests for the shared event-name table. The C++ side
// (engine/bridge/EventNames.h) carries the mirror copy; cross-language
// drift is caught at PR review by diffing the two files (one entry per
// constant per side). These tests guard the TS side's local invariants.
describe('EventNames', () => {
  it('every event name follows the domain/action convention', () => {
    // Domain is lowercase or lowerCamelCase (e.g. block, blockState).
    // Action is lowerCamelCase, optionally with hyphens (e.g.
    // update/open-release-notes which predates the rename).
    const pattern = /^[a-z][a-zA-Z]*\/[a-zA-Z][a-zA-Z-]*$/;
    for (const [key, value] of Object.entries(EventNames)) {
      expect(value, `${key}: '${value}'`).toMatch(pattern);
    }
  });

  it('every value is unique', () => {
    const values = Object.values(EventNames);
    expect(new Set(values).size).toBe(values.length);
  });

  it('keys and values are stable strings', () => {
    for (const [key, value] of Object.entries(EventNames)) {
      expect(typeof key).toBe('string');
      expect(typeof value).toBe('string');
      expect(value.length).toBeGreaterThan(0);
    }
  });
});
