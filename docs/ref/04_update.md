# Update

The update function takes a record object notation and mutates the dataset to add or overwrite lines.

the update record stream asks for update strategy and pipes it through update tablet streams. update tablet stream pipes lines to the update line stream which searches the record for values that match the line, signals a match and passes novel lines to a stream that writes them to file.

Each step passes a record to `csvs.update` and changes the state of the dataset.

- [update in dataset](#update-in-dataset)
  - [update stream](#update-stream)
    - [update strategy](#update-strategy)
    - [update tablet stream](#update-tablet-stream)
      - [update schema stream](#update-schema-stream)
      - [update line stream](#update-line-stream)

To learn more about the architecture of csvs, see other [User Guides](./user_guides.md), the [Reference](./reference.md) and the [Requirements](./requirements.md).

## update in dataset

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> List 
[Entry](./00_data_types.md#entry) -> List 
[Entry](./00_data_types.md#entry)

```pdl
pipe each query 
  to update stream 
  to return
```
## update stream

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> 
[Entry](./00_data_types.md#entry) -> List 
[Entry](./00_data_types.md#entry)

```pdl
schema = select schema
strategy = update strategy with schema, query
for each tablet of strategy
  append update tablet stream
pipe query
  to each update tablet stream
  to return
```

### test cases
 - 
   - initial: default
   - query: record 2001
   - expected: default
 - 
   - initial: default
   - query: record 2003 edited
   - expected: edited
 - 
   - initial: default
   - query: record added
   - expected: added
 - 
   - initial: array
   - query: record array added
   - expected: array added
 - 
   - initial: array empty
   - query: record array
   - expected: array
 - 
   - initial: array
   - query: record added array item
   - expected: added array item
 - 
   - initial: array
   - query: record edited array item
   - expected: edited array item
 - 
   - initial: array
   - query: record deleted array item
   - expected: deleted array item
 - 
   - initial: array
   - query: record edited array item object
   - expected: edited array item object
 - 
   - initial: schema none
   - query: record schema
   - expected: schema
 - 
   - initial: schema none
   - query: record schema literal
   - expected: schema literal
 - 
   - initial: default
   - query: record array literal
   - expected: array literal
 - 
   - initial: quotes
   - query: record quotes
   - expected: quotes
 - 
   - initial: newline
   - query: record newline
   - expected: newline
 - 
   - initial: pipe
   - query: record pipe
   - expected: pipe
 - 
   - initial: empty
   - query: record 2001, record 2002, record 2003 unedited
   - expected: default
## update strategy

[Schema](./00_data_types.md#schema) -> 
[Entry](./00_data_types.md#entry) -> List 
[Tablet](./00_data_types.md#tablet)

This describes all tablets needed to update an entry

```pdl
base = entry._
crown = find crown with schema, base
if base equals _
  return [{
    filename: _-_.csv
  }]
for each branch of crown
  for each trunk of schema.branch.trunks
    return {
      filename: trunk-branch.csv,
      trunk,
      branch,
    }
```
## update tablet stream

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> 
[Schema](./00_data_types.md#schema) -> 
[Tablet](./00_data_types.md#tablet) -> 
[Entry](./00_data_types.md#entry) -> IO 
[Entry](./00_data_types.md#entry)

```pdl
filepath = dir/tablet.filename
// in order to start other tablet streams
enqueue entry 
if tablet.filename equals _-_.csv
  pipe filepath
    to update schema stream
    to temporary file
otherwise
  pipe filepath
    to update line stream
    to temporary file
move temporary file to filepath
```
## update schema stream

[Entry](./00_data_types.md#entry) -> [Line](./00_data_types.md#line)

```pdl
for each field of entry
  for each leaf of entry.field
    enqueue field,leaf
```
## update line stream

[Entry](./00_data_types.md#entry) -> 
[Tablet](./00_data_types.md#tablet) -> [Line](./00_data_types.md#line) -> [Line](./00_data_types.md#line)

```pdl
grains = mow query with tablet.trunk, tablet.branch
keys = map grain to grain[tablet.trunk] sorted
values = reduce grains to { grain[tablet.trunk]: grain[tablet.branch] }
for each line
  fst, snd = parse line
  fst is new = state.fst is undefined or state.fst not equal fst
  if fst is new and state.match
    for each value of values[state.fst]
      enqueue state.fst,value
    keys = filter keys where key not equal state.fst
  if fst is new
    between = filter keys where key is after state.fst and before fst
    for each key of between
      for each value of values[key]
        enqueue key,value
      keys = filter keys where key not equal state.fst
  if keys not include fst
    enqueue line
  state = { fst, match }
for key of keys
  for each value of values[key]
    enqueue key,value
  keys = filter keys where key not equal state.fst
```
