# Data Types

## FS
FS is input output interface to the file system

## Dir
Dir is String of a path to a directory in [csvs dataset format](../specs/dataset_format.md)

## Branch
Branch is a string name of a given branch

## Trunk
Trunk is a String name of the trunk of Branch

## Leaf
Leaf is a String name of the leaf of Branch

## Schema
Schema is Map Branch Connection 

Connection is 
```js
{ 
  trunks: List Trunk, 
  leaves: List Leaf 
}
```

## Entry
Entry is a JSON in [Entry Object Notation](../specs/entry_object_notation.md)

## Query
Query is a JSON in [Query Object Notation](../specs/query_object_notation.md)

## Line
Line is a String in [csvs file format](../specs/file_format.md)

## State
State is 
```js 
{
  query: Query,
  entry: Entry,
  fst: String,
  match: Boolean,
  has match: Boolean,
  map: Map String Boolean,
  thing: String,
}
```

## Grain
Grain is a record with only _ field, base field and leaf field.

## Trait
Trait is String that we look for

## Thing
Thing is String we insert

## Tablet
Tablet is 
``` js
{
  filename: String, 
  thing: String,
  trait: String, 
  thing is first: Boolean,
  trait is first: Boolean,
  base: String,
  passthrough: Boolean,
  eager: Boolean,
  accumulating: Boolean,
  trait is regex: Boolean,
  querying: Boolean,
  trunk: String, 
  branch: String 
}
```
