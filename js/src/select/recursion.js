import { keyGroups } from "../query/groups.js";
import { isWellFormedBranch } from "../schema.js";
import { matchesRegex } from "../match.js";

/**
 * One `~` traversal: a closure over the tablet that stores the
 * relationship between the query base and the named field.
 * Mirrors the Rust `Traversal` in rs/src/dataset/select/recursion.rs.
 */
export class Traversal {
  constructor(stops, neighbors) {
    // Stop patterns (regex strings): a value matching any of these
    // is neither returned nor expanded.
    this.stops = stops;

    // Two-way adjacency over the tablet lines. The direction in
    // which the tablet stores the relationship does not affect
    // the result.
    this.neighbors = neighbors;

    // Values this traversal has already expanded.
    this.visited = new Set();
  }

  /**
   * Whether a value matches a stop value.
   * @param {string} value
   * @returns {Boolean}
   */
  isStopped(value) {
    return this.stops.some((pattern) => matchesRegex(pattern, value));
  }

  /**
   * Breadth-first closure from a seed value over the two-way join.
   * Returns newly reached values in discovery order, skipping stops
   * and values this traversal has already expanded.
   * @param {string} seed
   * @returns {string[]}
   */
  closure(seed) {
    const reached = [];

    if (this.visited.has(seed)) return reached;

    this.visited.add(seed);

    const queue = [seed];

    while (queue.length > 0) {
      const value = queue.shift();

      const nexts = this.neighbors.get(value);

      if (nexts === undefined) continue;

      for (const next of nexts) {
        if (this.visited.has(next) || this.isStopped(next)) continue;

        this.visited.add(next);

        reached.push(next);

        queue.push(next);
      }
    }

    return reached;
  }
}

/**
 * Parse `~` specs out of the query, removing the `~` key.
 * Each spec names a field of the query base collection and optionally
 * carries stop regexes under `.`. A spec without a base value or with
 * a base other than `~` is discarded.
 * @returns {{ traversals: Traversal[], stripped: object }}
 */
export async function extractTraversals(fs, dir, schema, query) {
  const { "~": specs, ...stripped } = query;

  if (specs === undefined) return { traversals: [], stripped };

  const specList = Array.isArray(specs) ? specs : [specs];

  const traversals = [];

  for (const specConcise of specList) {
    // concise form: "actname" is short for { _: "~", "~": "actname" }
    const spec =
      typeof specConcise === "string"
        ? { _: "~", "~": specConcise }
        : specConcise;

    if (spec === null || typeof spec !== "object") continue;

    if (spec._ !== "~") continue;

    // the base value of a `~` record is an exact field name
    const field = spec["~"];

    if (typeof field !== "string" || field === "") continue;

    // stop values are regexes; a malformed regex matches nothing
    const stopSpecs =
      spec["."] === undefined
        ? []
        : Array.isArray(spec["."])
          ? spec["."]
          : [spec["."]];

    const stops = stopSpecs
      .map((stop) =>
        typeof stop === "string"
          ? stop
          : stop !== null && typeof stop === "object"
            ? stop[stop._]
            : undefined,
      )
      .filter((stop) => typeof stop === "string");

    const neighbors = await loadNeighbors(fs, dir, schema, query._, field);

    traversals.push(new Traversal(stops, neighbors));
  }

  return { traversals, stripped };
}

/**
 * Find the tablet that stores the relationship between `base` and
 * `field` and read it into a two-way adjacency map. An unknown
 * relationship or a missing tablet yields an empty map.
 * @returns {Map<string, string[]>}
 */
async function loadNeighbors(fs, dir, schema, base, field) {
  const neighbors = new Map();

  // collection names are interpolated into a filename
  if (!isWellFormedBranch(base) || !isWellFormedBranch(field)) {
    return neighbors;
  }

  const hasTrunk = (branch, trunk) =>
    schema[branch] !== undefined && schema[branch].trunks.includes(trunk);

  let filename;

  if (hasTrunk(field, base)) {
    filename = `${base}-${field}.csv`;
  } else if (hasTrunk(base, field)) {
    filename = `${field}-${base}.csv`;
  } else {
    return neighbors;
  }

  const push = (key, value) => {
    const values = neighbors.get(key);

    if (values === undefined) {
      neighbors.set(key, [value]);
    } else {
      values.push(value);
    }
  };

  for await (const group of keyGroups(fs, dir, filename)) {
    for (const value of group.values) {
      push(group.key, value);

      push(value, group.key);
    }
  }

  return neighbors;
}
