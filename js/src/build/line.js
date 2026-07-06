import { sow } from "../record.js";
import { parseLine } from "../line.js";

export function buildLine(state, tablet, grains, line) {
  const [fst, snd] = parseLine(tablet.filename, line);

  const fstIsNew = state.fst === undefined || state.fst !== fst;

  state.fst = fst;

  const isComplete = state.isMatch;

  const isEndOfGroup = tablet.eager && fstIsNew;

  const pushEndOfGroup = isEndOfGroup && isComplete;

  if (pushEndOfGroup) {
    const stateToPush = {
      last: state.entry,
    };

    return stateToPush;
  }

  const grainNew = {
    _: tablet.trait,
    [tablet.trait]: fst,
    [tablet.thing]: snd,
  };

  const grainsNew = grains
    .map((grain) => {
      const isMatch = grain[tablet.trait] === fst;

      state.isMatch = state.isMatch ? state.isMatch : isMatch;

      if (isMatch) {
        return grainNew;
      }

      return undefined;
    })
    .filter((grain) => grain !== undefined);

  state.entry = grainsNew.reduce((withGrain, grain) => {
    return sow(withGrain, grain, tablet.trait, tablet.thing);
  }, state.entry);

  return state;
}
