# Sow & Mow

 - [sow](#sow)
 - [mow](#mow)

To learn more about the architecture of csvs, see other [User Guides](./user_guides.md), the [Reference](./reference.md) and the [Requirements](./requirements.md).

## sow
[Entry](./00_data_types.md#entry) -> [Grain](./00_data_types.md#grain) -> [Trait](./00_data_types.md#trait) -> [Thing](./00_data_types.md#thing) -> [Entry](./00_data_types.md#entry)

puts one field from Grain into Record

sow will find a place in Record that has the same value of Trait as Grain, and insert a Thing nearby.

how it can tell whether to insert the thing as leaf or trunk is beyond me. i guess it assumes thing is always leaf of trait? 

```pdl
base = record._

if base equals thing
  append grain.thing to record.thing
if base equals trait
  append grain.thing to record.thing
if record has trait
  for each item of record.trait
    if item.trait equals grain.trait
      append grain.thing to item.thing
otherwise 
  for each field of record
    for each item of record.field
      sow grain to item
```
### test cases
 - 
   - initial: record 2001
   - grain: grain 2001
   - trait: datum
   - thing: actdate
   - expected: record 2001 sow
 - 
   - initial: record array
   - grain: grain array
   - trait: export tags
   - thing: export 1 tag
   - expected: record array sow
 - 
   - initial: record sow base is trait
   - grain: grain sow base is trait
   - trait: datum
   - thing: filepath
   - expected: record sow base is trait
## mow
[Entry](./00_data_types.md#entry) -> [Trait](./00_data_types.md#trait) -> [Thing](./00_data_types.md#thing) -> List [Grain](./00_data_types.md#grain)

Break Record down into grains of Trait and Thing

mow will find all individual trait-thing relations in the record
```pdl
base = record._

<!-- TODO: weird trait/thing mix here -->
if base equals thing
  for each item of record.trait
    return { _: base, base: record.base, trait: item.trait }
<!-- TODO: weird trait/thing mix here -->
if base equals trait
  for each item of record.thing
    return { _: base, base: record.base, thing: item.thing }
if record has trait
  for each trunk of record.trait
    for each leaf of item.thing
      return { _: trait, trait: trunk, thing: leaf }
otherwise 
  for each field of record
    mow each item of record.field
```


### test cases
 - first level
   - initial: record 2001
   - trait: datum
   - thing: actdate
   - expected: grain 2001
 - second level
   - initial: record array
   - trait: export 1 tag
   - thing: export 1 channel
   - expected: grain channel 1, grain channel 2
 - empty
   - initial: record mow empty
   - trait: export 1 tag
   - thing: export 1 channel
   - expected: grain empty
