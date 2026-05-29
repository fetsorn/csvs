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
  mow,
  sow,
  buildRecord,
} from "../src/index.js";

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
  test("build without prose flag returns no @ keys", async () => {
    const dir = readDir("prose_default");
    const query = readRecord("query_prose_japan");

    const entry = await buildRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: [query],
    });

    expect(entry["@"]).toBeUndefined();
    expect(entry["@en"]).toBeUndefined();
    expect(entry["@ru"]).toBeUndefined();
    expect(entry.event).toBe("visited-japan");
    expect(entry.date).toBe("2001-01-01");
  });

  test("build with prose flag returns @ keys", async () => {
    const dir = readDir("prose_default");
    const query = readRecord("query_prose_japan");

    const entry = await buildRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: [query],
      prose: true,
    });

    expect(entry["@"]).toBe("We visited Tokyo and Kyoto in spring");
    expect(entry["@en"]).toBe("We visited Tokyo and Kyoto in spring");
    expect(entry["@ru"]).toBe("Мы посетили Токио и Киото весной");
  });

  test("build with prose flag returns no @ keys when no blobs exist", async () => {
    const dir = readDir("prose_default");

    const entry = await buildRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: [{ _: "event", event: "climbed-everest" }],
      prose: true,
    });

    expect(entry["@"]).toBeUndefined();
    expect(entry["@en"]).toBeUndefined();
    expect(entry.event).toBe("climbed-everest");
  });

  test("search by untagged prose content returns light records", async () => {
    const dir = readDir("prose_default");
    const query = readRecord("query_prose_search");

    const entries = await selectRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: [query],
      light: true,
    });

    expect(entries.length).toBe(1);
    expect(entries[0].event).toBe("visited-japan");
    expect(entries[0].date).toBeUndefined();
  });

  test("search by language-tagged prose content returns light records", async () => {
    const dir = readDir("prose_default");
    const query = readRecord("query_prose_search_ru");

    const entries = await selectRecord({
      fs: nodefs,
      bare: true,
      dir,
      query: [query],
      light: true,
    });

    expect(entries.length).toBe(1);
    expect(entries[0].event).toBe("visited-japan");
    expect(entries[0].date).toBeUndefined();
  });

  test("insert with @ keys writes blob and strips @ from tablets", async () => {
    const tmpdir = nodefs.mkdtempSync(join(os.tmpdir(), "csvs-"));

    copy(readDir("prose_default"), tmpdir);

    const record = readRecord("record_prose_insert");

    await insertRecord({
      fs: nodefs,
      bare: true,
      dir: tmpdir,
      query: [record],
    });

    // Check blob was written
    const blobPath = join(tmpdir, "@", "moved-to-bath.en");
    expect(nodefs.existsSync(blobPath)).toBe(true);
    expect(nodefs.readFileSync(blobPath, "utf8")).toBe(
      "Relocated to Bath for work",
    );

    // Check tablet has the event but not prose content
    const tablet = nodefs.readFileSync(join(tmpdir, "event-date.csv"), "utf8");
    expect(tablet).toContain("moved-to-bath");
    expect(tablet).not.toContain("Relocated");
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
