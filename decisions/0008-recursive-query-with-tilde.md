# 8. Recursive query with tilde operator

- Status: proposed
- Date: 2026-06-30

## Context

CSVS schemas can describe recursive relationships. A schema like
`"event": "event"` means events can reference other events. Querying
these chains today requires issuing multiple flat queries
programmatically -- query parents, then query parents of parents, and
so on until exhaustion. There is no way to express "follow this branch
recursively" in a single SON query.

This matters for two use cases:

1. **Traversal queries** -- ancestry, dependency chains, threaded
   conversations. "Find all ancestors of alice" is a recursive CTE in
   SQL, a property path in SPARQL, but inexpressible in SON.

2. **Scoped replication** -- when a shard contract says "give audience
   X everything reachable from alice through the parent branch," the
   replication tool must expand that into a sequence of flat queries.
   If SON supported recursion natively, the contract could be a single
   query and the expansion would happen inside csvs.

ADR-0007 moves prose store access from `~` to `@`, freeing the tilde
for reuse. The tilde is a natural choice: it suggests "more of the
same" and is visually light in query records.

## Decision

In a SON query record, the string value `"~"` on a branch means
"follow this branch recursively." The query engine resolves the
branch value, treats each result as a new key, and recurses until
no new records are found (fixpoint) or a cycle is detected.

### Syntax

A tilde value on a branch activates recursive traversal for that
branch:

```
{ "_": "person", "parent": "~" }
```

This means: find all person records, and for each, follow the parent
branch -- if a parent value is itself a person key, query for that
person's parents too, and so on.

### Flat query (no tilde)

```
{ "_": "person", "person": "alice", "parent": "" }
```

Finds alice's direct parents:

```
{ "_": "person", "person": "alice", "parent": "bob" }
{ "_": "person", "person": "alice", "parent": "mary" }
```

### Recursive query (tilde)

```
{ "_": "person", "person": "alice", "parent": "~" }
```

Finds alice's parents, their parents, and so on:

```
{ "_": "person", "person": "alice", "parent": "bob" }
{ "_": "person", "person": "alice", "parent": "mary" }
{ "_": "person", "person": "bob", "parent": "carol" }
{ "_": "person", "person": "mary", "parent": "dave" }
{ "_": "person", "person": "carol", "parent": "eve" }
```

### Mixed query

Tilde composes with other filters. Non-tilde branches constrain as
usual:

```
{ "_": "event", "actdate": "18*", "actname": "~" }
```

Find events from the 1800s, then follow actname recursively -- if an
actname value is itself an event key, include that event and continue.

### Multiple tilde branches

Multiple branches can carry tilde independently:

```
{ "_": "person", "parent": "~", "child": "~" }
```

Follow both parent and child branches. Each branch is traversed
separately; the result is the union of all records encountered.

### Semantics

- **Output**: a flat stream of all records encountered during
  traversal. No nesting, no tree structure. Each record appears at
  most once.

- **Cycle detection**: the engine tracks visited base values. If a
  traversal step yields a value already visited, that branch stops.
  This guarantees termination on cyclic data (e.g. `alice → bob →
  alice`).

- **Fixpoint**: traversal continues until a step yields no new
  records. This is the natural termination for acyclic data.

- **Cross-base traversal**: the tilde follows the schema. If the
  schema says `"event": "date"` and the query has
  `{ "_": "event", "date": "~" }`, traversal follows event → date →
  (whatever date links to per schema). The recursion respects the
  schema graph, not just self-referential branches.

- **Data records**: tilde has no meaning in data records. A literal
  string `"~"` in a data record is stored as-is. This is consistent
  with `!` (negation prefix) which is also query-only.

### SPARQL mapping

The tilde maps to SPARQL property paths. A recursive query like:

```
{ "_": "person", "person": "alice", "parent": "~" }
```

corresponds to:

```sparql
SELECT ?ancestor WHERE {
  :alice :parent+ ?ancestor .
}
```

The `+` (one-or-more) path operator is the closest equivalent. The
tilde always implies one-or-more, not zero-or-more, because the
non-recursive results are already returned by a flat query without
tilde.

## Consequences

- SON query dialect gains one reserved value: `"~"` on any branch
  means recursive traversal
- Depends on ADR-0007: tilde is repurposed from prose store access
  (now `@`) to recursion
- The `~` character joins `_`, `@`, and `!` as reserved SON
  characters with query semantics
- CSV tablets and hexastore are unaffected -- recursion is a query
  engine concern, not a storage concern
- Recursive CTE expansion moves from application code into csvs,
  simplifying replication contracts and query planners
- Cycle detection adds a small memory cost (set of visited values)
  per recursive query
- No depth limit is imposed by default; applications needing bounded
  traversal can implement it above csvs
- A literal `"~"` in data is unambiguous because tilde is only
  special in query records
