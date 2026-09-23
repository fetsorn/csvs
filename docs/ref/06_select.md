# Select

select must take query object notation

the select record stream asks for select strategy and pipes it through select tablet streams. select tablet stream pipes lines to the select line stream which searches the record for values that match the line, signals a match and transforms the record.

- [select schema](#select-schema)
- [select in dataset](#select-in-dataset)
  - [select stream](#select-stream)
    - [select strategy](#select-strategy)
    - [leader stream](#leader-stream)
    - [select tablet stream](#select-tablet-stream)
      - [select schema stream](#select-schema-stream)
      - [select line stream](#select-line-stream)
        - [drop stream](#drop-stream)
        - [forward stream](#forward-stream)
        - [parse line stream](#parse-line-stream)
          - [initial state](#initial-state)
          - [line state](#line-state)

To learn more about the architecture of csvs, see other [User Guides](./user_guides.md), the [Reference](./reference.md) and the [Requirements](./requirements.md).

## select schema

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> 
[Schema](./00_data_types.md#schema)

<!-- TODO rename schema to trunkToLeaf -->
```pdl
select in dataset { _: _ }
first result to schema
return schema
```
## select in dataset

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> List [Query](./00_data_types.md#query) -> List 
[Entry](./00_data_types.md#entry)

<!-- TODO rename record object notation to entry object notation -->
```pdl
pipe each query 
  to select stream 
  to return
```
## select stream

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> 
[Query](./00_data_types.md#query) -> List 
[Entry](./00_data_types.md#entry)

```pdl
base = query._
if base not equals _
  schema = select schema
strategy = select strategy with schema, query
for each tablet of strategy
  append select tablet stream
pipe query
  to each select tablet stream
  to leader stream
  to return
```

### test cases
 - first level
   - initial: default
   - query: 
   - expected: record 2001
 - first level
   - initial: default
   - query: 
   - expected: record 2002
 - first level
   - initial: default
   - query: 
   - expected: record 2023 unedited
 - unordered leaf
   - initial: unordered
   - query: 
   - expected: record 2002
 - array
   - initial: array simple
   - query: 
   - expected: record array simple
 - duplicate array item
   - initial: array double
   - query: 
   - expected: record array double 1, record array double 2
 - array item
   - initial: array
   - query: 
   - expected: record array
 - deep array item
   - initial: array added
   - query: 
   - expected: record array
 - regular expression
   - initial: default
   - query: 
   - expected: record 2003 unedited, record 2001, record 2002
 - failed regular expression
   - initial: default
   - query: 
   - expected: 
 - deep regular expression
   - initial: default
   - query: 
   - expected: record 2001, record 2002
## select strategy
[Schema](./00_data_types.md#schema) -> [Query](./00_data_types.md#query) -> List [Tablet](./00_data_types.md#tablet)

This describes all tablets needed to update an entry

```pdl
base = query._
if base equals _
  return {
    thing: _,
    trait: _,
    thing is first: false,
    trait is first: true,
    filename: _-_.csv,
  }
queried = gather keys sorted ascending with schema
if queried.length > 0
  for each branch of queried
    for each trunk of schema[branch].trunks
      append {
        thing: trunk,
        trait: branch,
        thing is first: true,
        trait is first: false,
        base: trunk,
        filename: trunk-branch.csv,
        trait is regex: true,
        querying: true,
        eager: true,
      }
otherwise
  for each trunk of schema[base].trunks
    append {
      thing: base,
      trait: trunk,
      thing is first: false,
      trait is first: false,
      base: trunk,
      filename: trunk-base.csv,
      trait is regex: true,
      eager: true,
      accumulating: true,
    }
  for each leaf of schema[base].leaves
    append {
      thing: base,
      trait: base
      thing is first: true,
      trait is first: true,
      base: base, 
      filename: base-leaf.csv,
      trait is regex: true,
      accumulating: true,
      eager: true,
    }
crown = find crown with schema, base sorted descending
for each branch of crown
  for each trunk of schema[branch].trunks
    if trunk is connected with schema, base
      append {
        thing: branch,
        trait: trunk,
        thing is first: false,
        trait is first: true,
        base: trunk,
        filename: trunk-branch.csv,
        passthrough: true,
        eager: if trunk equals base then true,
      }
```
## select tablet stream

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> 
[Tablet](./00_data_types.md#tablet)

```pdl
filepath = dir/tablet.filename
if tablet.filename equals _-_.csv
  pipe filepath
    to select schema stream
    to return
otherwise
  pipe filepath
    to select line stream
    to return
```
## select schema stream
[State](./00_data_types.md#state) -> 
[Line](./00_data_types.md#line) -> [State](./00_data_types.md#state)

```pdl
state.entry: { _: _ }

for each line
  trunk,leaf = parse line
  leaves = state.entry[trunk]
  new leaves = [leaf] if no leaves otherwise leaves + leaf
  state.entry[trunk] = new leaves
enqueue {
  query: state.query,
  entry: state.entry,
}
```
## select line stream
[State](./00_data_types.md#state) -> 
[Tablet](./00_data_types.md#tablet) -> 
[State](./00_data_types.md#state)

```pdl
if tablet.passthrough and state.map
  return drop stream
if tablet.accumulating and no state.map
  return forward stream
otherwise
  return parse line stream
```
## drop stream

[State](./00_data_types.md#state) -> [Line](./00_data_types.md#line) -> Void

```pdl
do nothing
```
## forward stream

[State](./00_data_types.md#state) -> [Line](./00_data_types.md#line) -> 
[State](./00_data_types.md#state)

```pdl
at the start
  enqueue {
    query: state.query,
    entry: state.entry,
  }
do nothing
```
## parse line stream

[State](./00_data_types.md#state) -> 
[Tablet](./00_data_types.md#tablet) -> 
[Line](./00_data_types.md#line) -> 
[State](./00_data_types.md#state)

```pdl
initial = initial state with state, tablet
grains = mow query with tablet.trait, tablet.thing
for each line
  fst,snd = parse line
  fst is new = state.fst not equals fst
  state.fst = fst;
  if state.match and tablet.eager and fst is new
    enqueue {
      query: state.query,
      entry: state.entry,
      thing: state.thing,
    }
    state.entry = initial.entry
    state.query = initial.query
    state.match = initial.match
  trait = fst if tablet.trait is first otherwise snd
  thing = fst if tablet.thing is first otherwise snd
  state = line state with initial, state, tablet, grains, trait, thing
if not tablet.eager or state.match
  enqueue {
    query: state.query,
    entry: state.entry,
    thing: state.thing,
  }
if tablet.accumulating
  enqueue {
   query: state.query,
   entry: initial.entry,
   map: state.map,
  }
else if tablet.passthrough and not state.has match
  enqueue {
    query: state.query,
    entry: state.entry,
  }
```
## initial state

[State](./00_data_types.md#state) -> 
[Tablet](./00_data_types.md#tablet) -> 
[State](./00_data_types.md#state)

```pdl
same base = tablet.querying and tablet.base === query._
fallback entry = { _: tablet.base } if same base otherwise entry
initial entry = sow { _: tablet.base } with entry, tablet.base, entry._ if tablet.querying and not same base otherwise fallback entry
initial thing = undefined if entry._ not equal initial entry._ otherwise thing
value tablet = not tablet.accumulating and not tablet.querying
accumulating by trunk = tablet.accumulating and not tablet.thing is first
initial query = initial entry if value tablet or accumulating by trunk otherwise query
return {
  query: initial query,
  entry: initial entry,
  fst: undefined,
  match: false,
  has match: false,
  map: map,
  thing: initial thing,
}
```
## line state

[State](./00_data_types.md#state) -> 
[State](./00_data_types.md#state) -> 
[Tablet](./00_data_types.md#tablet) -> List [Grain](./00_data_types.md#grain) -> [Trait](./00_data_types.md#trait) -> [Thing](./00_data_types.md#thing) -> 
[State](./00_data_types.md#state)

```pdl
new grain = {
  _: tablet.trait,
  tablet.trait: trait,
  tablet.thing: thing,
}

new grains = for each grain in grains
  match grain = grain[tablet.trait].test(trait) if tablet.trait is regex otherwise grain[tablet.trait] equals trait
  match querying = tablet.querying and has initial.thing
  match = match grain or match querying
  new match = no state.map or no thing in state.map
  state.match = state.match if state.match otherwise match and new match
  state.thing = thing if tablet.querying and state.match
  state.map.thing = true if tablet.accumulating and match and new match
  state.has match = state.has match if state.has match otherwise state.match
  if match and new match
    return new grain
state.entry = reduce new grains to sow state.entry with grain, tablet.trait, tablet.thing
state.query = reduce state.query to grain, tablet.trait, tablet.thing if tablet.querying
return state
```
## leader stream
[Base](./00_data_types.md#base) -> [Query](./00_data_types.md#query) -> 
[State](./00_data_types.md#state) -> 
[State](./00_data_types.md#state)

```pdl
if state.entry._ equals base 
  base = state.entry._
if query.__ 
  entry = state.query[query.__]
otherwise
  entry = state.entry
enqueue entry
```
