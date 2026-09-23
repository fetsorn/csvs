# Delete

delete literal must take literal query object notation and delete records that match literal constraints

the delete record stream asks for delete strategy and pipes it through delete tablet streams. delete tablet stream pipes lines to the delete line stream to match the record and prune lines that are written to the file.

delete regular expression must take regular expression query object notation and delete records that match regular expression constraint

should remove literal and only match on regular expression?

WON'T delete unexpected extra records that match constraint by accident

should we only have literal match after query to avoid unexpected deletions?

 - [delete in dataset](#delete-in-dataset)
   - [delete stream](#delete-stream)
     - [delete strategy](#delete-strategy)
     - [delete tablet](#delete-tablet)
       - [delete line stream](#delete-line-stream)

To learn more about the architecture of csvs, see other [User Guides](./user_guides.md), the [Reference](./reference.md) and the [Requirements](./requirements.md).

## delete in dataset

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> List 
[Query](./00_data_types.md#query) -> IO List 
[Entry](./00_data_types.md#entry)

```pdl
pipe each query 
  to delete stream 
  to return
```
## delete stream

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> 
[Query](./00_data_types.md#query) -> IO List 
[Entry](./00_data_types.md#entry)

```pdl
schema = select schema
for each query
  strategy = delete strategy with schema, query
  for each tablet of strategy
    delete tablet with fs, dir, query
    return query
```

### test cases
 - deletes a record
   - query: record 2003 unedited
   - initial: dataset default
   - expected: dataset deleted
 - deletes a record with a trunk
   - query: record export 1 tag 
   - initial: dataset array
   - expected: dataset deleted leaf
 - deletes to an empty dir
   - query: record sow base is trait
   - initial: dataset deleted leaf
   - expected: dataset deleted leaf empty
## delete strategy

[Schema](./00_data_types.md#schema) -> 
[Entry](./00_data_types.md#entry) -> List [Tablet](./00_data_types.md#tablet)

This describes all tablets needed to delete an entry

```pdl
base = entry._
if base has trunk
  append { 
    filename: trunk-base.csv, 
    trait: entry.base, 
    trait is first: false 
  }
for each leaf of base
  append {
    filename: base-leaf.csv, 
    trait: entry.base, 
    trait is first: true 
  }
```
## delete tablet

[FS](./00_data_types.md#fs) -> [Dir](./00_data_types.md#dir) -> 
[Query](./00_data_types.md#query) -> IO

```pdl
filepath = dir/tablet.filename;
if filepath is empty return;
pipe filepath 
  to delete line stream
  to append temporary file;
move temporary file to filepath;
```
## delete line stream
[Tablet](./00_data_types.md#tablet) -> 
[Line](./00_data_types.md#line) -> 
[Line](./00_data_types.md#line)

```pdl
fst, snd = parse line
trait is fst if tablet.trait is first
trait is snd if not tablet.trait is first 
if trait equals tablet.trait
  enqueue line
```
