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

        const blobs = testCase.expected_blobs
          || (testCase.expected_blob ? [testCase.expected_blob] : []);

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
