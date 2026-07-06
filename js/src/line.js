import csv from "papaparse";
import { unescapeNewline } from "./escape.js";

/**
 * Parse one tablet line into its two fields.
 * A CSVS record is exactly "key,value" — a line that papaparse cannot
 * split into two fields is malformed data, and silently producing
 * undefined would crash far from the cause. Report the tablet and the
 * offending line instead.
 * @param {string} filename - tablet filename, for error reporting.
 * @param {string} line - one line of the tablet.
 * @returns {[string, string]} unescaped [key, value] pair.
 */
export function parseLine(filename, line) {
  if (process.env.CSVS_TRACE) {
    console.error(`[csvs] ${filename}: ${JSON.stringify(line)}`);
  }

  const {
    data: [[fstEscaped, sndEscaped]],
  } = csv.parse(line, { delimiter: "," });

  if (fstEscaped === undefined || sndEscaped === undefined) {
    throw new Error(
      `malformed line in ${filename}: expected two comma-separated fields, got ${JSON.stringify(line)}`,
    );
  }

  return [unescapeNewline(fstEscaped), unescapeNewline(sndEscaped)];
}
