import path from "path";
import { ReadableStream } from "@swimburger/isomorphic-streams";
import { isEmpty } from "../stream.js";
import { mow, sow } from "../record.js";
import { matchesRegex } from "../match.js";
import { keyGroups } from "./groups.js";

export function makeStateInitial({ query, entry, thingQuerying }, tablet) {
  // in a querying tablet, set initial entry to the base of the tablet
  // and preserve the received entry for sowing grains later
  // if tablet base is different from previous entry base
  // sow previous entry into the initial entry
  const isSameBase = tablet.base === query._;

  const doDiscard = entry === undefined || isSameBase;

  const entryFallback = doDiscard ? { _: tablet.base } : entry;

  const doSow = !doDiscard;

  const entryInitial = doSow
    ? sow(
        { _: tablet.base },
        { _: entry._, [entry._]: entry[entry._] },
        tablet.base,
        entry._,
      )
    : entryFallback;

  const entryBaseChanged = entry === undefined || entry._ !== entryInitial._;

  // if entry base changed forget thingQuerying
  const thingQueryingInitial = entryBaseChanged ? undefined : thingQuerying;

  return { entry: entryInitial, query, thingQuerying: thingQueryingInitial };
}

/**
 * Match a key group against query grains.
 * Returns { matched, entry, query, thingQuerying } where matched is true
 * if any value in the group satisfied all grain predicates.
 */
function matchGroup(key, values, tablet, grains, stateInitial) {
  let groupEntry = { ...stateInitial.entry };
  let groupQuery = { ...stateInitial.query };
  let matched = false;
  let groupThingQuerying = undefined;

  for (const value of values) {
    const trait_ = tablet.traitIsFirst ? key : value;
    const thing = tablet.thingIsFirst ? key : value;

    const grainNew = {
      _: tablet.base,
      [tablet.trait]: trait_,
      [tablet.thing]: thing,
    };

    // all grains must match
    const isMatchGrains = grains.reduce((acc, grain) => {
      const isMatchGrain = tablet.traitIsRegex
        ? matchesRegex(grain[tablet.trait], trait_)
        : grain[tablet.trait] === trait_;

      return acc === undefined ? isMatchGrain : acc && isMatchGrain;
    }, undefined);

    // when parent key (thingQuerying) is set, also require it matches
    const doDiff = stateInitial.thingQuerying !== undefined;
    const isMatchQuerying = doDiff
      ? stateInitial.thingQuerying === thing
      : true;

    const isMatch = isMatchGrains && isMatchQuerying;

    if (isMatch) {
      matched = true;
      groupThingQuerying = thing;
      groupEntry = sow(groupEntry, grainNew, tablet.trait, tablet.thing);
      groupQuery = sow(groupQuery, grainNew, tablet.trait, tablet.thing);
    }
  }

  return {
    matched,
    entry: groupEntry,
    query: groupQuery,
    thingQuerying: groupThingQuerying,
  };
}

export async function queryTabletStream(
  fs,
  dir,
  tablet,
  { query, entry, thingQuerying },
  first,
) {
  const filepath = path.join(dir, tablet.filename);
  const empty = await isEmpty(fs, filepath);

  // first tablet needs lines — empty file means no matches
  // later tablet avoids lines — empty file means match all
  if (empty && first) {
    return new ReadableStream({
      start(controller) {
        controller.close();
      },
    });
  }

  if (empty && !first) {
    return new ReadableStream({
      start(controller) {
        controller.enqueue({ query, entry, thingQuerying });

        controller.close();
      },
    });
  }

  const stateInitial = makeStateInitial(
    { query, entry, thingQuerying },
    tablet,
  );

  const grains = mow(stateInitial.query, tablet.trait, tablet.thing);

  async function* matchedEntries() {
    for await (const { key, values } of keyGroups(fs, dir, tablet.filename)) {
      const result = matchGroup(key, values, tablet, grains, stateInitial);

      if (result.matched) {
        yield {
          entry: result.entry,
          query: result.query,
          thingQuerying: result.thingQuerying,
        };
      }
    }
  }

  return new ReadableStream({
    async pull(controller) {
      for await (const entry of matchedEntries()) {
        controller.enqueue(entry);
      }

      controller.close();
    },
  });
}
