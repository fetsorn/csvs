<div align="center">

part of the [ontonomy](https://norcivilianlabs.org) software suite

AGPL-3.0. Anton Davydov.

</div>

# csvs

A plain-text relational database.

A csvs dataset is a directory of two-column CSV files called
tablets. The schema tablet `_-_.csv` names the relationships
between collections. Data tablets hold the values.

```
_-_.csv              name-age.csv       name-city.csv
name,age             john,35            john,Bath
name,city            jane,36            john,London
```

John is 35, Jane is 36, John lives in Bath and London.

Collections only exist in relationship with another. A tablet
filename is two collection names joined by a hyphen. Values
are unconstrained UTF-8. A key can have multiple values
(multiple records with the same key). There are no headers
inside the file; the filename is the header.

Large text lives in the prose store (`@/`), addressed by value.

The [spec](https://norcivilianlabs.org) has the full grammar
and rules.

## Implementations

- [js/](js/) JavaScript (Node.js and browser)
- [rs/](rs/) Rust (library + CLI)
- [c/](c/) C (library, not at feature parity)

## Query

A query is a JSON object naming a collection and optionally
constraining its values:

```json
{ "_": "name" }
```

returns every name record with its age and city.

```json
{ "_": "name", "name": "john" }
```

returns only John's record. Values are matched as regular
expressions.

