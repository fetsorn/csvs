# Insert

The update function takes a record object notation and mutates the dataset to append lines.

the insert record stream asks for insert strategy and pipes it through insert tablet streams. insert tablet stream writes the records as lines at the end of the tablet. at the end of the record stream the tablet content is grouped to a valid form by sorting.

- [insert in dataset](#insert-in-dataset)
  - [insert stream](#insert-stream)
    - [insert strategy](#insert-strategy)
    - [insert tablet stream](#insert-tablet-stream)

To learn more about the architecture of csvs, see other [User Guides](./user_guides.md), the [Reference](./reference.md) and the [Requirements](./requirements.md).

## insert in dataset
[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> List [Entry](./00_data_types.md#entry) -> IO List [Entry](./00_data_types.md#entry)

```pdl
pipe each query 
  to insert stream
  to return
```
## insert stream
[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> [Entry](./00_data_types.md#entry) -> IO List [Entry](./00_data_types.md#entry)

```pdl
schema = select schema
for each entry
  strategy = insert strategy with schema, query
  for each tablet of strategy
    append insert tablet stream
for each tablet of strategy
  sort tablet
pipe query 
  to each insert tablet stream
  to return
```
### test cases
- duplicate
  - query: record 2001
  - initial: default
  - expected: duplicate
- duplicate leaf
  - query: record 2001 edited
  - initial: default
  - expected: duplicate leaf
- add
  - query: record added
  - initial: default
  - expected: added

## insert strategy
[Schema](./00_data_types.md#schema) -> [Entry](./00_data_types.md#entry) -> List [Tablet](./00_data_types.md#tablet)

This describes all tablets needed to delete an entry

```pdl
base = entry._
crown = find crown with schema, base
for each branch of crown
  for each trunk of schema.branch.trunks
    return {
      filename: trunk-branch.csv,
      trunk: trunk,
      branch: branch,
    }
```

## insert tablet stream
[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> [Tablet](./00_data_types.md#tablet) -> [Entry](./00_data_types.md#entry) -> IO 
[Entry](./00_data_types.md#entry)

```pdl
filepath = dir/tablet.filename
// in order to start other tablet streams
enqueue entry 
grains = mow query with tablet.trunk, tablet.branch
for each grain
  append grain.key,grain.value to filepath
```
