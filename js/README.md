<div align="center">

part of the [ontonomy](https://norcivilianlabs.org) software suite

</div>

# csvs

A plain-text relational database. A csvs dataset is a directory
of two-column CSV files called tablets. The schema tablet
`_-_.csv` names the relationships between collections. Data
tablets hold the values.

```
_-_.csv              name-age.csv       name-city.csv
name,age             john,35            john,Bath
name,city            jane,36            john,London
```

This is the JavaScript implementation. It works in Node.js and
in the browser.

Full format description: [spec](https://norcivilianlabs.org).
Source and other implementations: [codeberg.org/fetsorn/csvs](https://codeberg.org/fetsorn/csvs).

## Install

```
npm i @fetsorn/csvs-js
```

## Use

```js
import { selectRecord } from "@fetsorn/csvs-js";

// all names with their age and city
const records = await selectRecord({
  fs,
  dir: "./my-dataset",
  query: { _: "name" },
});

// just john
const john = await selectRecord({
  fs,
  dir: "./my-dataset",
  query: { _: "name", name: "john" },
});
```

Query values are matched as regular expressions.

AGPL-3.0. Anton Davydov.
