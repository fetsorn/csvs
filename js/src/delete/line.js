import { parseLine } from "../line.js";

export function pruneLine(tablet, line) {
  if (line === "") return false;

  const [fst, snd] = parseLine(tablet.filename, line);

  const trait = tablet.traitIsFirst ? fst : snd;

  const isMatch = trait === tablet.trait;

  return isMatch;
}
