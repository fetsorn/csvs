import path from "path";
import { isEmpty, chunksToLines } from "../stream.js";
import { parseLine } from "../line.js";

/**
 * Layer 1 — KeyGroupStream.
 * Reads a sorted CSV tablet and yields key groups: { key, values }.
 * Each group collects all values for a contiguous run of the same key.
 * Pure CSV logic, no csvs domain concepts.
 */
export async function* keyGroups(fs, dir, filename) {
  const filepath = path.join(dir, filename);

  const empty = await isEmpty(fs, filepath);

  if (empty) return;

  const lineStream = chunksToLines(fs.createReadStream(filepath));

  let currentKey = undefined;
  let currentValues = [];

  for await (const line of lineStream) {
    if (line === "" || line === "\n") continue;

    const [key, value] = parseLine(filename, line);

    if (currentKey !== undefined && key !== currentKey) {
      yield { key: currentKey, values: currentValues };
      currentValues = [];
    }

    currentKey = key;
    currentValues.push(value);
  }

  if (currentKey !== undefined) {
    yield { key: currentKey, values: currentValues };
  }
}
