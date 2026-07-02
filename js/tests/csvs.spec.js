/* eslint-disable no-console */
import { describe, expect, test } from "@jest/globals";
import nodefs from "fs";
import { join } from "path";
import os from "os";
import {
  readDir,
  readTestCase,
  readRecord,
  loadContents,
  sortList,
  copy,
  sortObject,
} from "@fetsorn/csvs-test";
import {
  init,
  selectRecord,
  updateRecord,
  insertRecord,
  deleteRecord,
  toSchema,
  findCrown,
  sortNestingAscending,
  sortNestingDescending,
  isWellFormedBranch,
  mow,
  sow,
  buildRecord,
} from "../src/index.js";
import { isConnected, getNestingLevel } from "../src/schema.js";

describe("selectRecord()", () => {
  readTestCase("select").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readDir(testCase.initial),
        query: testCase.query.map(readRecord),
        expected: testCase.expected.map(readRecord),
      };

      const data = await selectRecord({
        fs: nodefs,
        bare: true,
        dir: testCase.initial,
        query: testCase.query,
      });

      const dataSorted = sortList(data);

      const expected = sortList(testCase.expected);

      expect(dataSorted).toStrictEqual(expected);
    });
  });
});

describe("updateRecord()", () => {
  readTestCase("update").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readDir(testCase.initial),
        query: testCase.query.map(readRecord),
        expected: readDir(testCase.expected),
      };

      const tmpdir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-"));

      copy(testCase.initial, tmpdir);

      await updateRecord({
        fs: nodefs,
        bare: true,
        dir: tmpdir,
        query: testCase.query,
      });

      expect(loadContents(tmpdir)).toStrictEqual(
        loadContents(testCase.expected),
      );
    });
  });
});

describe("insertRecord()", () => {
  readTestCase("insert").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readDir(testCase.initial),
        query: testCase.query.map(readRecord),
        expected: readDir(testCase.expected),
      };

      const tmpdir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-"));

      copy(testCase.initial, tmpdir);

      await insertRecord({
        fs: nodefs,
        bare: true,
        dir: tmpdir,
        query: testCase.query,
      });

      expect(loadContents(tmpdir)).toStrictEqual(
        loadContents(testCase.expected),
      );
    });
  });
});

describe("deleteRecord()", () => {
  readTestCase("delete").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readDir(testCase.initial),
        query: testCase.query.map(readRecord),
        expected: readDir(testCase.expected),
      };

      const tmpdir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-"));

      copy(testCase.initial, tmpdir);

      await deleteRecord({
        fs: nodefs,
        bare: true,
        dir: tmpdir,
        query: testCase.query,
      });

      expect(loadContents(tmpdir)).toStrictEqual(
        loadContents(testCase.expected),
      );
    });
  });
});

describe("mow()", () => {
  readTestCase("mow").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readRecord(testCase.initial),
        trunk: testCase.trunk,
        branch: testCase.branch,
        expected: testCase.expected.map(readRecord),
      };

      const data = mow(testCase.initial, testCase.trunk, testCase.branch);

      const dataSorted = sortList(data);

      const expected = sortList(testCase.expected);

      expect(dataSorted).toStrictEqual(expected);
    });
  });
});

describe("sow()", () => {
  readTestCase("sow").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readRecord(testCase.initial),
        grain: readRecord(testCase.grain),
        trunk: testCase.trunk,
        branch: testCase.branch,
        expected: readRecord(testCase.expected),
      };

      const data = sow(
        testCase.initial,
        testCase.grain,
        testCase.trunk,
        testCase.branch,
      );

      const dataSorted = sortObject(data);

      const expected = sortObject(testCase.expected);

      expect(dataSorted).toStrictEqual(expected);
    });
  });
});

describe("toSchema()", () => {
  readTestCase("to_schema").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readRecord(testCase.initial),
        expected: readRecord(testCase.expected),
      };

      const data = toSchema(testCase.initial);

      const dataSorted = sortObject(data);

      const expected = sortObject(testCase.expected);

      expect(dataSorted).toStrictEqual(expected);
    });
  });
});

describe("prose", () => {
  readTestCase("prose").forEach((testCase) => {
    test(testCase.name, async () => {
      const dir = readDir(testCase.initial);
      const query = testCase.query.map(readRecord);

      if (testCase.op === "build") {
        const entry = await buildRecord({
          fs: nodefs,
          bare: true,
          dir,
          query,
          prose: testCase.prose,
        });

        const expected = readRecord(testCase.expected[0]);

        expect(sortObject(entry)).toStrictEqual(sortObject(expected));
      } else if (testCase.op === "select") {
        const data = await selectRecord({
          fs: nodefs,
          bare: true,
          dir,
          query,
          light: true,
        });

        const expected = testCase.expected.map(readRecord);

        expect(sortList(data)).toStrictEqual(sortList(expected));
      } else if (testCase.op === "insert" || testCase.op === "update") {
        const tmpdir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-"));

        copy(dir, tmpdir);

        const fn_ = testCase.op === "insert" ? insertRecord : updateRecord;

        await fn_({
          fs: nodefs,
          bare: true,
          dir: tmpdir,
          query,
        });

        const blobs =
          testCase.expected_blobs ||
          (testCase.expected_blob ? [testCase.expected_blob] : []);

        for (const blob of blobs) {
          const blobPath = join(tmpdir, blob.path);

          expect(nodefs.existsSync(blobPath)).toBe(true);
          expect(nodefs.readFileSync(blobPath, "utf8")).toBe(blob.content);
        }
      }
    });
  });
});

describe("init()", () => {
  readTestCase("init").forEach((testCase) => {
    test(testCase.name, async () => {
      testCase = {
        initial: readDir(testCase.initial),
        expected: readDir(testCase.expected),
      };

      const tmpdir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-"));

      copy(testCase.initial, tmpdir);

      await init({
        fs: nodefs,
        bare: true,
        dir: tmpdir,
      });

      expect(loadContents(tmpdir)).toStrictEqual(
        loadContents(testCase.expected),
      );
    });
  });
});

describe("recursive schema does not overflow the stack", () => {
  // The format explicitly permits recursive relations, e.g. `event,event`
  // or `event -> date -> event`. Walking the schema graph must terminate
  // on cycles instead of recursing until "Maximum call stack size exceeded".

  test("self-recursive branch (landmark -> landmark)", () => {
    const schema = toSchema({ _: "_", landmark: "landmark" });

    // an unrelated base must not send isConnected into a cycle
    expect(isConnected(schema, "unrelated", "landmark")).toBe(false);
    // the branch is trivially connected to itself
    expect(isConnected(schema, "landmark", "landmark")).toBe(true);
    expect(findCrown(schema, "landmark")).toStrictEqual(["landmark"]);

    expect(getNestingLevel(schema, "landmark")).toBe(1);
    expect(() =>
      Object.keys(schema).sort(sortNestingAscending(schema)),
    ).not.toThrow();
    expect(() =>
      Object.keys(schema).sort(sortNestingDescending(schema)),
    ).not.toThrow();
  });

  test("mutually-recursive branches (event <-> date)", () => {
    const schema = toSchema({ _: "_", event: "date", date: "event" });

    expect(isConnected(schema, "unrelated", "event")).toBe(false);
    expect(isConnected(schema, "event", "date")).toBe(true);
    expect(isConnected(schema, "date", "event")).toBe(true);

    expect(() => getNestingLevel(schema, "event")).not.toThrow();
    expect(() => getNestingLevel(schema, "date")).not.toThrow();
    expect(() =>
      Object.keys(schema).sort(sortNestingAscending(schema)),
    ).not.toThrow();
  });

  test("a diamond (non-cyclic) is still counted at full depth", () => {
    // datum -> { a, b } -> leaf : leaf is reachable through two distinct
    // paths but there is no cycle. The cycle guard must not treat the
    // second path as already-visited and collapse leaf's level; it stays
    // at 2 (one above a/b), the same as before the guard was added.
    const schema = toSchema({
      _: "_",
      datum: ["a", "b"],
      a: "leaf",
      b: "leaf",
    });

    expect(getNestingLevel(schema, "datum")).toBe(0);
    expect(getNestingLevel(schema, "a")).toBe(1);
    expect(getNestingLevel(schema, "b")).toBe(1);
    expect(getNestingLevel(schema, "leaf")).toBe(2);
  });
});

describe("prose language tag is a BCP 47 controlled vocabulary", () => {
  test("rejects a traversal tag instead of writing outside the store", async () => {
    const root = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-lang-"));
    const dir = join(root, "store");

    nodefs.mkdirSync(join(dir, "@"), { recursive: true });
    nodefs.writeFileSync(join(dir, ".csvs.csv"), "version,0.0.4\nid,test\n");
    nodefs.writeFileSync(join(dir, "_-_.csv"), "event,date\n");

    const outside = join(root, "PWNED.txt");

    await expect(
      insertRecord({
        fs: nodefs,
        bare: true,
        dir,
        query: {
          _: "event",
          event: "x",
          "@../../../../PWNED.txt": "arbitrary file write via lang tag",
        },
      }),
    ).rejects.toThrow(/BCP 47/);

    expect(nodefs.existsSync(outside)).toBe(false);
  });

  test("accepts a normal language tag", async () => {
    const dir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-lang-ok-"));

    nodefs.mkdirSync(join(dir, "@"), { recursive: true });
    nodefs.writeFileSync(join(dir, ".csvs.csv"), "version,0.0.4\nid,test\n");
    nodefs.writeFileSync(join(dir, "_-_.csv"), "event,date\n");

    await insertRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: { _: "event", event: "x", "@en-US": "hello" },
    });

    expect(nodefs.readFileSync(join(dir, "@", "x.en-US"), "utf8")).toBe(
      "hello",
    );
  });
});

describe("schema collection names must be safe as tablet filenames", () => {
  test("rejects a traversal branch instead of reading outside the store", async () => {
    const root = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-branch-"));
    const dir = join(root, "store");

    nodefs.mkdirSync(dir, { recursive: true });

    // secret file OUTSIDE the dataset directory
    nodefs.writeFileSync(join(root, "secret.csv"), "topsecret,hunter2\n");

    // branch escapes the `event-` filename prefix via an inner slash,
    // so `event-x/../../secret.csv` resolves to root/secret.csv
    const evil = "x/../../secret";
    nodefs.writeFileSync(join(dir, "_-_.csv"), `event,${evil}\n`);
    nodefs.writeFileSync(join(dir, "event-event.csv"), "topsecret,topsecret\n");

    await expect(
      selectRecord({
        fs: nodefs,
        bare: true,
        dir,
        query: { _: "event", event: "topsecret" },
      }),
    ).rejects.toThrow(/well-formed branch/);
  });

  test("rejects a traversal branch instead of writing outside the store", async () => {
    const root = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-branch-w-"));
    const dir = join(root, "store");

    nodefs.mkdirSync(dir, { recursive: true });

    const outside = join(root, "PWNED.csv");
    const evil = "x/../../PWNED";
    nodefs.writeFileSync(join(dir, "_-_.csv"), `event,${evil}\n`);

    await expect(
      updateRecord({
        fs: nodefs,
        bare: true,
        dir,
        query: { _: "event", event: "k", [evil]: "INJECTED" },
      }),
    ).rejects.toThrow(/well-formed branch/);

    expect(nodefs.existsSync(outside)).toBe(false);
  });

  test("isWellFormedBranch accepts normal names, rejects unsafe ones", () => {
    for (const ok of ["event", "date", "actName", "tag%40", "a b", "日本語"]) {
      expect(isWellFormedBranch(ok)).toBe(true);
    }

    for (const bad of [
      "_",
      "",
      "a/b",
      "a\\b",
      "a.b",
      "a-b",
      "..",
      "x/../../secret",
    ]) {
      expect(isWellFormedBranch(bad)).toBe(false);
    }
  });
});

describe("an invalid regex filter matches nothing instead of throwing", () => {
  // A query filter value is a regex. An incomplete pattern typed into a
  // search bar — e.g. an unbalanced "(" — must match nothing rather than
  // abort the whole select. This contract was established for the
  // full-record query path; these cover the option and prose-search
  // paths, which used to compile the pattern unguarded.

  function makeStore(prefix) {
    const dir = nodefs.mkdtempSync(join(os.tmpdir(), prefix));

    nodefs.writeFileSync(join(dir, ".csvs.csv"), "version,0.0.4\nid,test\n");
    nodefs.writeFileSync(join(dir, "_-_.csv"), "event,date\n");
    nodefs.writeFileSync(join(dir, "event-date.csv"), "visited,2001-01-01\n");

    return dir;
  }

  test("option path (base filter, no leaves): invalid regex → no rows", async () => {
    const dir = makeStore("csvs-rx-opt-");

    // a valid pattern still selects the option
    const ok = await selectRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: { _: "event", event: "vis" },
      light: true,
    });
    expect(ok.map((r) => r.event)).toStrictEqual(["visited"]);

    // an invalid pattern matches nothing without throwing
    let bad;
    await expect(
      (async () => {
        bad = await selectRecord({
          fs: nodefs,
          bare: true,
          dir,
          query: { _: "event", event: "vis(" },
          light: true,
        });
      })(),
    ).resolves.not.toThrow();
    expect(bad).toStrictEqual([]);
  });

  test("query path (leaf filter): invalid regex → no rows", async () => {
    const dir = makeStore("csvs-rx-qry-");

    const data = await selectRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: { _: "event", date: "2001(" },
      light: true,
    });

    expect(data).toStrictEqual([]);
  });

  test("prose-search path: invalid regex → no rows", async () => {
    const dir = makeStore("csvs-rx-prose-");

    nodefs.mkdirSync(join(dir, "@"), { recursive: true });
    nodefs.writeFileSync(join(dir, "@", "visited"), "a long description\n");

    // sanity: a valid prose regex finds the record
    const ok = await selectRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: { _: "event", "@": "description" },
      light: true,
    });
    expect(ok.map((r) => r.event)).toStrictEqual(["visited"]);

    // an invalid prose regex matches nothing without throwing
    let bad;
    await expect(
      (async () => {
        bad = await selectRecord({
          fs: nodefs,
          bare: true,
          dir,
          query: { _: "event", "@": "desc(" },
          light: true,
        });
      })(),
    ).resolves.not.toThrow();
    expect(bad).toStrictEqual([]);
  });
});

describe("an empty (0-byte) tablet behaves like a missing one", () => {
  // A data tablet that exists on disk but is empty must be treated the
  // same as an absent one: for a non-first tablet in a query join it is a
  // match-all passthrough, not a source that filters out every candidate.
  // `isEmpty` (stream.js) encodes this contract; the csvs-rs port used to
  // guard only on file existence and silently dropped the query state.

  test("query path: empty non-first tablet does not drop the match", async () => {
    const dir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-empty-"));

    nodefs.writeFileSync(join(dir, ".csvs.csv"), "version,0.0.4\nid,test\n");
    // `event` has two twig leaves; at equal nesting they sort
    // reverse-alphabetically, so the plan visits `event-tag` (populated)
    // before `event-date` (empty).
    nodefs.writeFileSync(join(dir, "_-_.csv"), "event,date\nevent,tag\n");
    nodefs.writeFileSync(join(dir, "event-tag.csv"), "visited-japan,japan\n");
    // an existing but empty intermediate tablet
    nodefs.writeFileSync(join(dir, "event-date.csv"), "");

    const data = await selectRecord({
      fs: nodefs,
      bare: true,
      dir,
      // `date` is constrained, but its tablet is empty, so that constraint
      // is ignored (match-all) rather than excluding the row.
      query: { _: "event", tag: "japan", date: "unused" },
      light: true,
    });

    expect(data.map((r) => r.event)).toStrictEqual(["visited-japan"]);
  });
});

// The module contract (index.js) promises: "Pass a pre-built `schema` option
// to any operation to avoid repeated reads of `_-_.csv`." The Rust port keeps
// a `schema_cache` on `Dataset` and reads the schema once. The JS select
// family used to ignore the option and rebuild the schema from disk once per
// record (via `buildRecord`), so a select over N records opened `_-_.csv`
// N+ times. These tests pin the cache boundary.
describe("selectRecord reads the schema tablet once, not once per record", () => {
  function countingFs(target) {
    let schemaReads = 0;

    // Only createReadStream touches `_-_.csv` (via selectSchema); wrap it and
    // delegate everything else to the real fs.
    // inherit everything (incl. the `promises` getter) from the real fs,
    // overriding only createReadStream to count schema-tablet opens
    const wrapped = Object.create(target);

    wrapped.createReadStream = (p, ...rest) => {
      if (String(p).endsWith("_-_.csv")) schemaReads += 1;

      return target.createReadStream(p, ...rest);
    };

    return { fs: wrapped, reads: () => schemaReads };
  }

  function seedDataset() {
    const dir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-schema-"));

    nodefs.writeFileSync(join(dir, ".csvs.csv"), "version,0.0.4\nid,test\n");
    nodefs.writeFileSync(join(dir, "_-_.csv"), "name,age\n");
    nodefs.writeFileSync(
      join(dir, "name-age.csv"),
      "jack,37\njane,36\njohn,35\n",
    );

    return dir;
  }

  test("a full (non-light) select re-reads the schema at most once", async () => {
    const dir = seedDataset();

    const { fs, reads } = countingFs(nodefs);

    const data = await selectRecord({
      fs,
      bare: true,
      dir,
      query: { _: "name" },
    });

    expect(data.map((r) => r.name).sort()).toStrictEqual([
      "jack",
      "jane",
      "john",
    ]);

    // three records built; without the cache this was >= 3 schema reads
    expect(reads()).toBeLessThanOrEqual(1);
  });

  test("a caller-supplied schema is honored and reads the tablet zero times", async () => {
    const dir = seedDataset();

    const schema = toSchema({ _: "_", name: "age" });

    const { fs, reads } = countingFs(nodefs);

    const data = await selectRecord({
      fs,
      bare: true,
      dir,
      query: { _: "name" },
      schema,
    });

    expect(data.map((r) => r.name).sort()).toStrictEqual([
      "jack",
      "jane",
      "john",
    ]);

    expect(reads()).toBe(0);
  });
});
