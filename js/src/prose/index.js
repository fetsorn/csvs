import path from "path";

/**
 * Percent-encode filesystem-reserved characters.
 */
function uriEncode(value) {
  return value.replace(/[/\\<>:"|?*.%\0]/g, (ch) =>
    [...Buffer.from(ch, "utf8")]
      .map((b) => `%${b.toString(16).toUpperCase().padStart(2, "0")}`)
      .join(""),
  );
}

/**
 * Decode percent-encoded characters.
 */
function uriDecode(value) {
  return value.replace(/%([0-9A-Fa-f]{2})/g, (_, hex) =>
    String.fromCharCode(parseInt(hex, 16)),
  );
}

/**
 * Resolve a value + optional language tag to a filesystem path.
 */
function prosePath(dir, value, lang) {
  const encoded = uriEncode(value);
  const filename = lang ? `${encoded}.${lang}` : encoded;
  return path.join(dir, "@", filename);
}

/**
 * Read all prose for a value (untagged + all language tags).
 * Returns an object like { null: "untagged text", "en": "english", "ru": "russian" }
 * where null key means untagged.
 */
export async function readProse(fs, dir, value) {
  const result = {};

  // Try untagged
  const untaggedPath = prosePath(dir, value, null);
  try {
    const content = await fs.promises.readFile(untaggedPath, "utf8");

    result["@"] = content;
  } catch (e) {
    // no untagged blob
  }

  // Scan for language-tagged files
  const proseDir = path.join(dir, "@");
  const encoded = uriEncode(value);
  const prefix = `${encoded}.`;

  try {
    const files = await fs.promises.readdir(proseDir);

    for (const file of files) {
      if (file.startsWith(prefix) && file.length > prefix.length) {
        const lang = file.slice(prefix.length);

        const content = await fs.promises.readFile(
          path.join(proseDir, file),
          "utf8",
        );

        result[`@${lang}`] = content;
      }
    }
  } catch {
    // prose dir doesn't exist
  }

  return result;
}

/**
 * Write a single prose blob.
 */
export async function writeProse(fs, dir, value, lang, content) {
  const filePath = prosePath(dir, value, lang);
  const fileDir = path.dirname(filePath);

  try {
    await fs.promises.mkdir(fileDir, { recursive: true });
  } catch {
    // already exists
  }

  await fs.promises.writeFile(filePath, content, "utf8");
}

/**
 * Search prose files for content matching a regex pattern.
 * Returns values whose prose matches.
 */
export async function searchProse(fs, dir, pattern, lang) {
  const proseDir = path.join(dir, "@");
  const re = new RegExp(pattern);
  const matches = [];

  let files;
  try {
    files = await fs.promises.readdir(proseDir);
  } catch {
    return matches;
  }

  for (const file of files) {
    // Parse filename into value and lang
    const dotIndex = file.lastIndexOf(".");
    let fileEncoded, fileLang;

    if (dotIndex > 0) {
      fileEncoded = file.slice(0, dotIndex);
      fileLang = file.slice(dotIndex + 1);
    } else {
      fileEncoded = file;
      fileLang = null;
    }

    // Filter by language tag
    // "@" (lang=null) searches all prose regardless of tag;
    // "@en" etc. matches only that specific language.
    const langMatches = lang === null ? true : fileLang === lang;

    if (!langMatches) continue;

    const content = await fs.promises.readFile(
      path.join(proseDir, file),
      "utf8",
    );

    if (re.test(content)) {
      matches.push(uriDecode(fileEncoded));
    }
  }

  return matches;
}

/**
 * Recursively extract @ keys from a record and all nested entries.
 * Returns { proseEntries, stripped } where proseEntries is an array
 * of { value, key, content } for each @ key found at any depth,
 * and stripped is the record with all @ keys removed.
 */
export function extractProse(record) {
  const proseEntries = [];

  function walk(obj) {
    if (obj === null || typeof obj !== "object") return obj;

    if (Array.isArray(obj)) return obj.map(walk);

    const result = {};
    const base = obj._;
    const baseValue = base !== undefined ? obj[base] : undefined;

    for (const [key, value] of Object.entries(obj)) {
      if (key.startsWith("@")) {
        proseEntries.push({
          value: typeof baseValue === "string" ? baseValue : undefined,
          key,
          content: value,
        });
      } else if (typeof value === "object" && value !== null) {
        result[key] = walk(value);
      } else {
        result[key] = value;
      }
    }

    return result;
  }

  const stripped = walk(record);

  return { proseEntries, stripped };
}

/**
 * Parse a prose key like "@en" into a language tag or null for "@".
 */
export function parseLang(key) {
  if (key === "@") return null;
  return key.slice(1);
}
