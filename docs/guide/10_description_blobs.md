# Description blobs

Any value in a CSVS dataset can have a large text description stored
in the prose store. This lets you start with a minimal graph and grow
structure over time.

## Starting small

A single-predicate dataset is already a useful knowledge base:

```
.csvs.csv:
version,0.0.4
id,my-dataset

_-_.csv:
name,age

name-age.csv:
john,35
```

Add a description to any value with the `@` operator:

```
{ "_": "name", "name": "john", "@": "John is a software engineer from Bath, 35 years old" }
```

This stores the text at `prose/john`. The graph stays tiny and fast,
but "john" now carries rich prose behind it. A person browsing the
dataset can open the `prose/` folder, see the file `john`, open it,
and read about John.

Language-tagged descriptions work the same way:

```
{ "_": "name", "name": "john", "@en": "A software engineer from Bath", "@ru": "Инженер-программист из Бата" }
```

## Growing structure from blobs

Over time, you notice patterns in your descriptions. "Bath" keeps
coming up. "Software engineer" is something you want to filter by.
Extract them into the graph:

```
_-_.csv:
name,age
name,city
name,occupation

name-city.csv:
john,Bath

name-occupation.csv:
john,software engineer
```

The blob shrinks as its content graduates into structure:

```
{ "_": "name", "name": "john", "@": "Likes cycling and tea" }
```

Or disappears entirely once everything is formalized.

## Blobs as a staging area

Blobs are the staging area for knowledge that isn't yet worth
formalizing. You don't need to design your schema upfront -- write
prose first, extract predicates later. The graph grows organically
from unstructured descriptions into structured relationships.

## Searching blobs

Search across all descriptions with a regex:

```
{ "_": "name", "@": "Bath" }
```

Or search within a specific language:

```
{ "_": "name", "@en": "engineer" }
```
