/**
 * csvs-js - plain-text relational database backed by CSV files.
 *
 * Designed for single-writer access. Concurrent writes to the same
 * dataset directory may corrupt data. Use git-based sync for
 * multi-device access.
 *
 * Pass a pre-built `schema` option to any operation to avoid
 * repeated reads of `_-_.csv`.
 */
import { init } from "./init/index.js";
import { buildSchema } from "./schema/index.js";
import {
  selectRecord,
  selectRecordStream,
  selectRecordStreamPull,
} from "./select/index.js";
import { selectOption } from "./option/index.js";
import { queryRecord } from "./query/index.js";
import { buildRecord } from "./build/index.js";
import { updateRecord } from "./update/index.js";
import { deleteRecord } from "./delete/index.js";
import { insertRecord } from "./insert/index.js";
import {
  findCrown,
  toSchema,
  sortNestingAscending,
  sortNestingDescending,
  isWellFormedBranch,
} from "./schema.js";
import { mow, sow } from "./record.js";

export {
  init,
  selectRecordStream,
  selectRecordStreamPull,
  selectRecord,
  selectOption,
  buildSchema,
  buildRecord,
  updateRecord,
  insertRecord,
  deleteRecord,
  queryRecord,
  findCrown,
  toSchema,
  sortNestingAscending,
  sortNestingDescending,
  isWellFormedBranch,
  mow,
  sow,
};

export default {
  init,
  selectRecordStream,
  selectRecordStreamPull,
  selectRecord,
  selectOption,
  buildSchema,
  buildRecord,
  updateRecord,
  insertRecord,
  deleteRecord,
  queryRecord,
  findCrown,
  toSchema,
  sortNestingAscending,
  sortNestingDescending,
  isWellFormedBranch,
  mow,
  sow,
};
