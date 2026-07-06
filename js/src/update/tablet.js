import path from "path";
import csv from "papaparse";
import { isEmpty, chunksToLines } from "../stream.js";
import { mow } from "../record.js";
import { escapeNewline } from "../escape.js";
import { updateLine } from "./line.js";

async function appendTablet(fs, dir, tablet, query, tmpPath) {
  const filepath = path.join(dir, tablet.filename);

  const empty = await isEmpty(fs, filepath);

  const lineStream = empty
    ? new ReadableStream({
        start(controller) {
          controller.close();
        },
      })
    : chunksToLines(fs.createReadStream(filepath));

  const grains = mow(query, tablet.trunk, tablet.branch);

  // get the keys, sorted
  let keys = [...new Set(grains.map((grain) => grain[tablet.trunk]))]
    .filter((key) => key !== undefined)
    .sort(); // probably need to sort to insert in between in order

  // get values for each key, sorted
  const values = grains.reduce((acc, grain) => {
    const key = grain[tablet.trunk];

    const value = grain[tablet.branch];

    // filter out the { _: a, a: undefined } which mow returns
    // when there's no connections
    // TODO refactor mow to remove this
    if (value === undefined) return acc;

    const valuesOld = acc[key] ?? [];

    // TODO might remove this sort but need to update test fixtures
    const valuesNew = [...valuesOld, value].sort();

    return { ...acc, [key]: valuesNew };
  }, {});

  let state = { keys };

  async function insertAndForget(key) {
    for (const value of values[key] ?? []) {
      const keyEscaped = escapeNewline(key);

      const valueEscaped = escapeNewline(value);

      const line = csv.unparse([[keyEscaped, valueEscaped]], {
        delimiter: ",",
        newline: "\n",
      });

      await fs.promises.appendFile(tmpPath, line + "\n");
    }

    delete values[key];

    state.keys = state.keys.filter((k) => k !== key);

    return undefined;
  }

  for await (const line of lineStream) {
    if (line === "") continue;

    if (state.keys.length === 0) {
      await fs.promises.appendFile(tmpPath, line);

      continue;
    }

    state = updateLine(tablet, state, line);

    for (const key of state.keysInserted) {
      await insertAndForget(key);
    }

    if (state.isMatch) {
      // if keys include this key, prune line
      // it will be inserted again before the next key
    } else {
      // otherwise write line unchanged
      await fs.promises.appendFile(tmpPath, line);
    }
  }

  // TODO what if key repeats many times in keys
  // for each key in alphabetic order
  for (const key of keys) {
    await insertAndForget(key);
  }
}

export async function updateTablet(fs, dir, tablet, query) {
  const tmpdir = await fs.promises.mkdtemp(path.join(dir, "update-"));

  const tmpPath = path.join(tmpdir, tablet.filename);

  await appendTablet(fs, dir, tablet, query, tmpPath);

  if (!(await isEmpty(fs, tmpPath))) {
    // fs.rename doesn't work with external drives
    // fs.copyFile doesn't work with lightning fs
    const file = await fs.promises.readFile(tmpPath);

    const filepath = path.join(dir, tablet.filename);

    await fs.promises.writeFile(filepath, file);

    // unlink to rmdir later
    await fs.promises.unlink(tmpPath);
  }

  // use rmdir because lightningfs doesn't support rm
  await fs.promises.rmdir(tmpdir);
}
