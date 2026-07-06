import { parseLine } from "../line.js";

// returns keys to insert
export function updateLine(tablet, state, line) {
  let keysInserted = [];

  const [fst] = parseLine(tablet.filename, line);

  const fstIsNew = state.fst === undefined || state.fst !== fst;

  // if previous isMatch and fstIsNew
  if (state.isMatch && fstIsNew) {
    keysInserted.push(state.fst);
  }

  if (fstIsNew) {
    // insert and forget all record keys between previous and next
    const keysBetween = state.keys
      .filter((key) => !keysInserted.includes(key))
      .filter((key) => {
        const isAfter =
          state.fst === undefined || !(key < state.fst || key === state.fst);

        const isBefore = key < fst;

        const isBetween = isAfter && isBefore;

        return isBetween;
      });

    for (const key of keysBetween) {
      keysInserted.push(key);
    }
  }

  // match this fst against all keys
  const isMatch = state.keys.includes(fst);

  return { fst, isMatch, keysInserted, keys: state.keys };
}
