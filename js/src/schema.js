/**
 * This tells if a branch is connected to base branch.
 *
 * The schema graph may contain cycles: the format explicitly permits
 * recursive relations such as `event,event` or `event -> date -> event`.
 * `visited` records the branches already on the current search so a cycle
 * terminates instead of recursing until the stack overflows.
 *
 * @name isConnected
 * @function
 * @param {object} schema - Dataset schema.
 * @param {string} base - Base branch name.
 * @param {string} branch - Branch name.
 * @param {Set<string>} [visited] - Branches already visited on this walk.
 * @returns {Boolean}
 */
export function isConnected(schema, base, branch, visited = new Set()) {
  if (branch === base) {
    // if branch is base, it is connected
    return true;
  }

  if (schema[branch] === undefined) return false;

  if (visited.has(branch)) {
    // already explored this branch on the current walk: a cycle,
    // so this path yields no new way to reach base
    return false;
  }

  visited.add(branch);

  const { trunks } = schema[branch];

  for (const trunk of trunks) {
    if (trunk === undefined) {
      // if schema root is reached, leaf is not connected to base
      continue;
    }

    if (trunk === base) {
      // if trunk is base, leaf is connected to base
      return true;
    }

    if (isConnected(schema, base, trunk, visited)) {
      // if trunk is connected to base, leaf is also connected to base
      return true;
    }
  }

  // if trunk is not connected to base, leaf is also not connected to base
  return false;
}

/**
 * This finds all branches that are connected to the base branch.
 * @name findCrown
 * @function
 * @param {object} schema - Dataset schema.
 * @param {string} base - Base branch name.
 * @returns {string[]} - Array of leaf branches connected to the base branch.
 */
export function findCrown(schema, base) {
  return Object.keys(schema).filter((branch) =>
    isConnected(schema, base, branch),
  );
}

/**
 * Nesting level = distance from a leaf node in the schema graph.
 * Leaves (no trunks) are level 0, their trunks are level 1, etc.
 * E.g. for datum -> filepath -> moddate: moddate=0, filepath=1, datum=2.
 *
 * The schema graph may contain cycles (e.g. `event -> date -> event`),
 * for which distance-from-a-leaf is undefined. `path` holds the branches
 * on the current descent; revisiting one closes a cycle and stops the
 * descent so the recursion terminates instead of overflowing the stack.
 * A fresh copy is passed to each trunk so that a branch reachable through
 * two distinct paths (a diamond, not a cycle) is still counted fully.
 */
export function getNestingLevel(schema, branch, path = new Set()) {
  if (schema[branch] === undefined) return 0;

  if (path.has(branch)) return 0;

  const nextPath = new Set(path).add(branch);

  const { trunks } = schema[branch];

  const trunkLevels = trunks.map((trunk) =>
    getNestingLevel(schema, trunk, nextPath),
  );

  const level = trunkLevels.reduce((a, b) => Math.max(a, b), -1);

  return level + 1;
}

/**
 * sort by level of nesting, twigs and leaves come first
 * @name sortNestingAscending
 * @export function
 * @param {string} a - dataset entity.
 * @param {string} b - dataset entity.
 * @returns {number} - sorting index, a<b -1, a>b 1, a==b 0
 */
export function sortNestingAscending(schema) {
  // Precompute levels once so each comparison is O(1).
  const levels = Object.fromEntries(
    Object.keys(schema).map((k) => [k, getNestingLevel(schema, k)]),
  );

  return (a, b) => {
    const levelA = levels[a] ?? 0;

    const levelB = levels[b] ?? 0;

    if (levelA > levelB) {
      return -1;
    }

    if (levelA < levelB) {
      return 1;
    }

    return b < a ? -1 : b > a ? 1 : 0;
  };
}

/**
 * sort by level of nesting, trunks come first
 * @name sortNestingDescending
 * @export function
 * @param {string} a - dataset entity.
 * @param {string} b - dataset entity.
 * @returns {number} - sorting index, a<b -1, a>b 1, a==b 0
 */
export function sortNestingDescending(schema) {
  // Precompute levels once so each comparison is O(1).
  const levels = Object.fromEntries(
    Object.keys(schema).map((k) => [k, getNestingLevel(schema, k)]),
  );

  return (a, b) => {
    const levelA = levels[a] ?? 0;

    const levelB = levels[b] ?? 0;

    if (levelA < levelB) {
      return -1;
    }

    if (levelA > levelB) {
      return 1;
    }

    return a < b ? -1 : a > b ? 1 : 0;
  };
}

function append(list, item) {
  const isEmpty = list === undefined || list.length === 0;

  // use flat instead of spread here in case list is one item
  return isEmpty ? [item] : [list, item].flat();
}

/**
 * Characters a collection (branch) name must not contain, per the CSVS
 * spec. Branch names are interpolated into tablet filenames as
 * `{trunk}-{branch}.csv`, so an unconstrained name — e.g. one containing
 * a slash and `..` — escapes the dataset directory and allows arbitrary
 * file reads and writes (path traversal). This mirrors the prose
 * language-tag guard (`isWellFormedLang` in prose/index.js), which closes
 * the same vector for prose filenames.
 *
 * The set covers filesystem separators and traversal (`/ \ .`), the tablet
 * key separator (`-`), and the other characters the spec reserves because
 * collection names are used as filenames.
 */
const BRANCH_FORBIDDEN = /[/\\<>':"`|?*.,\[\];{}$&\n\r-]/;

/**
 * A well-formed collection (branch) name: a non-empty string that is
 * neither the reserved schema base `_` nor contains a forbidden character.
 * @param {string} name - Candidate collection name.
 * @returns {boolean}
 */
export function isWellFormedBranch(name) {
  return (
    typeof name === "string" &&
    name.length > 0 &&
    name !== "_" &&
    !BRANCH_FORBIDDEN.test(name)
  );
}

/**
 * Throw if a collection name would be unsafe to use as a tablet filename.
 * @param {string} name - Candidate collection name.
 */
export function assertWellFormedBranch(name) {
  if (!isWellFormedBranch(name)) {
    throw new Error(
      `csvs: collection name is not a well-formed branch: ${JSON.stringify(name)}`,
    );
  }
}

export function toSchema(schemaRecord) {
  const invalidRecord =
    !Object.hasOwn(schemaRecord, "_") || schemaRecord._ !== "_";

  if (invalidRecord) return {};

  const { _: omit, ...record } = schemaRecord;

  return Object.entries(record).reduce((withTrunk, [trunk, value]) => {
    assertWellFormedBranch(trunk);

    const leaves = Array.isArray(value) ? value : [value];

    return leaves.reduce((withLeaf, leaf) => {
      assertWellFormedBranch(leaf);

      const trunkOld = withLeaf[trunk] ?? {};

      const trunkTrunks = trunkOld.trunks ?? [];

      const trunkLeaves =
        withLeaf[trunk] !== undefined
          ? append(withLeaf[trunk].leaves, leaf)
          : [leaf];

      const trunkPartial = {
        [trunk]: {
          leaves: trunkLeaves,
          trunks: trunkTrunks,
        },
      };

      const leafOld = withLeaf[leaf] ?? {};

      const leafTrunks =
        withLeaf[leaf] !== undefined
          ? append(withLeaf[leaf].trunks, trunk)
          : [trunk];

      const leafLeaves = leafOld.leaves ?? [];

      const leafPartial = {
        [leaf]: {
          trunks: leafTrunks,
          leaves: leafLeaves,
        },
      };

      return { ...withLeaf, ...trunkPartial, ...leafPartial };
    }, withTrunk);
  }, {});
}
