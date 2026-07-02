/**
 * Test `value` against `pattern` interpreted as a regular expression.
 *
 * A query filter value is a regex. A malformed pattern — e.g. an
 * incomplete regex typed into a search bar — matches nothing instead of
 * throwing and aborting the whole operation. This is the single source
 * of truth for that contract, shared by every path that treats a filter
 * value as a regex: full-record query, option select, and prose search.
 * The Rust port mirrors it with `Regex::new(..).ok()`.
 *
 * @param {string} pattern - Filter value, treated as a regex.
 * @param {string} value - Candidate string from a tablet or blob.
 * @returns {Boolean}
 */
export function matchesRegex(pattern, value) {
  try {
    return new RegExp(pattern).test(value);
  } catch {
    return false;
  }
}
