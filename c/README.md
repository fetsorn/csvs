csvs - Comma-Separated Value Store
===================================

This is the C implementation of csvs, a plain-text relational
database. A csvs dataset is a directory of two-column CSV files
called tablets. The schema tablet `_-_.csv` names the
relationships between collections. Data tablets hold the values.

This implementation is not at feature parity with the JavaScript
and Rust implementations. It builds as a static library
(`libcsvs.a`).

For the full format description, see the spec at
<https://norcivilianlabs.org>.

For other implementations and the monorepo, see
<https://codeberg.org/fetsorn/csvs>.

Dependencies
------------

  * libcjson (cJSON)
  * libcsv

On Fedora:

    dnf install cjson-devel libcsv-devel

Building
--------

    make

This produces `libcsvs.a`. Link against it with `-lcjson -lcsv`.

    make test
    make sanitize
    make valgrind

API
---

See `src/csvs.h`. The public interface follows:

    csvs_dataset *ds = csvs_open("./my-dataset");
    csvs_schema_load(ds);

    csvs_entry q = csvs_entry_new("name");
    csvs_iter *it = csvs_select(ds, &q, 1, 0, 0);

    const csvs_entry *e;
    while ((e = csvs_next(it)) != NULL) {
        cJSON *j = csvs_entry_to_json(e);
        /* ... */
        cJSON_Delete(j);
    }

    csvs_iter_free(it);
    csvs_entry_free(&q);
    csvs_close(ds);

License
-------

Copyright (C) 2026 Anton Davydov

csvs is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 3 of
the License, or (at your option) any later version.

csvs is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.
