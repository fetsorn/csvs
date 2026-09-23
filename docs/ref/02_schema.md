# Schema

 - [to schema](#to-schema)
 - [is connected](#is-connected)
 - [find crown](#find-crown)
  
To learn more about the architecture of csvs, see other [User Guides](./user_guides.md), the [Reference](./reference.md) and the [Requirements](./requirements.md).

## to schema
[Entry](./00_data_types.md#entry) -> [Schema](./00_data_types.md#schema)

```pdl
if entry has no `_:_` return {}
for each trunk of entry except _
  for each leaf of entry.trunk
    append trunk to schema.leaf.trunks
    append leaf to schema.trunk.leaves
return schema
```
## is connected
[Schema](./00_data_types.md#schema) -> [Base](./00_data_types.md#base) -> [Branch](./00_data_types.md#branch) -> Boolean

This tells if a branch is connected to base branch.

```pdl
if branch equals base
  return true
for each trunk of branch
  if trunk equals base
    return true
  if trunk is connected to base
    return true
otherwise
  return false
```

## find crown
[Schema](./00_data_types.md#schema) -> [Base](./00_data_types.md#base) -> List [Branch](./00_data_types.md#branch)

This finds all branches that are connected to the base branch.

```pdl
if leaf is connected to base
  return leaf
```

## get nesting level

[Schema](./00_data_types.md#schema) -> [Branch](./00_data_types.md#branch)

```pdl
for each trunk of branch
  get nesting level of trunk
  increment level
```

### test cases
- naught
  - schema: record schema array
  - branch: datum
  - expected: 0
- first
  - schema: record schema array
  - branch: export tags
  - expected: 1
- second
  - schema: record schema array
  - branch: export2 tag
  - expected: 2
- third
  - schema: record schema array
  - branch: export2 tag description
  - expected: 3
- fourth
  - schema: record schema array
  - branch: export 2 tag description text 1
  - expected: 4

## sort nesting ascending
[Schema](./00_data_types.md#schema) -> ([Branch](./00_data_types.md#branch) -> [Branch](./00_data_types.md#branch) -> Ordering)

sort by level of nesting, twigs and leaves come first

### test cases
 - 
   - schema: record schema array
   - initial: 
   - expected: 
## sort nesting descending
[Schema](./00_data_types.md#schema) -> ([Branch](./00_data_types.md#branch) -> [Branch](./00_data_types.md#branch) -> Ordering)

sort by level of nesting, trunks come first

### test cases
 - 
   - schema: record schema array
   - initial: 
   - expected: 
