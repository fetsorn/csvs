# Scenarios

 - [Insert](#insert)
 - [Update](#update)
 - [Update](#update-2)
 - [Select](#select)

## Insert
### step 1: beginning 
> empty directory
> ``` csv
> # .csvs.csv
> # - no changes
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> # - no changes
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```

### step 2: add a record

``` javascript
// a record about vising Japan in 2001
{
  _: "event",
  event: "visited-japan",
  date: "2001-01-01"
}
```

> - added `visited-japan,2001-01-01` to `event-date.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> visited-japan,2001-01-01
> ```

### step 3: add another record

``` javascript
// a record about cooking a lasagna in 2002
{
  _: "event",
  event: "cooked-lasagna",
  date: "2002-02-02"
}
```

> - added `cooked-lasagna,2002-02-02` to `event-date.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> visited-japan,2001-01-01
> cooked-lasagna,2002-02-02
> ```

### step 4: add attribute of a record

``` javascript
// a record that adds another visit to Japan on December 12th 2012
{
  _: "event",
  event: "visited-japan",
  date: "2001-12-12"
}
```

> - added `visited-japan,2001-12-12` to `event-date.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> cooked-lasagna,2002-02-02
> visited-japan,2001-01-01
> visited-japan,2001-12-12
> ```



## Update
### step 1: beginning 
> empty directory
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```

### step 2: add a schema

``` javascript
// a record with base `_` holds the schema
{
  _: "_",
  event: [ "date", "filepath" ],
  filepath: [ "filehash" ]
}
```

> - added relation for each item in a value array to `_-_.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv 
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> ```

### step 3: add a branch to the schema

``` javascript
// a record holds a schema with new branch `filesize`
{
  _: "_",
  event: [ "date", "filepath" ],
  filepath: [ "filehash", "filesize" ]
}
```

> - added filepath, filesize to `_-_.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```

### step 4: add a record

``` javascript
// a record about visiting Japan in 2001
{
  _: "event",
  event: "visited-japan",
  date: "2001-01-01"
}
```

> - added `visited-japan,2001-01-01` to `event-date.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> visited-japan,2001-01-01
> ```

### step 5: add another record

``` javascript
// a record about cooking a lasagna in 2002
{
  _: "event",
  event: "cooked-lasagna",
  date: "2002-02-02"
}
```

> - added `cooked-lasagna,2002-02-02` to `event-date.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> cooked-lasagna,2002-02-02
> visited-japan,2001-01-01
> ```

### step 6: edit attribute of a record
    
``` javascript
// a record that changes the date for visiting Japan to December 2001
{
  _: "event",
  event: "visited-japan",
  date: "2001-12-12"
}
```

> - removed `visited-japan,2001-01-01` from `event-date.csv`
> - added `visited-japan,2001-12-12` to `event-date.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> cooked-lasagna,2002-02-02
> visited-japan,2001-12-12
> ```

### step 7: add another record

``` javascript
// a record about climbing Everest in 2003, with a photo
{
  _: "event",
  event: "climbed-everest",
  date: "2003-03-03",
  filepath: "photo-everest"
}
```

> - added `climbed-everest,2003-03-03` to `event-date.csv`
> - added `climbed-everest,photo-everest` to `event-filepath.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> cooked-lasagna,2002-02-02
> climbed-everest,2003-03-03
> visited-japan,2001-12-12
> ```
> 
> ``` csv
> # event-filepath.csv
> climbed-everest,photo-everest
> ```

### step 8: add an attribute

``` javascript
// a record with a hashsum for the photo of Everest
{
  _: "filepath",
  filepath: "photo-everest",
  filehash: "0x0000"
}
```

> - added `photo-everest,0x0000` to `filepath-filehash.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> cooked-lasagna,2002-02-02
> climbed-everest,2003-03-03
> visited-japan,2001-12-12
> ```
> 
> ``` csv
> # event-filepath.csv
> climbed-everest,photo-everest
> ```
> 
> ``` csv
> # filepath-filehash.csv
> photo-everest,0x0000
> ```

### step 9: add a record with a nested attribute

``` javascript
// a record about breaking a leg in 2004, with an X-ray photo
{
  _: "event",
  event: "broke-leg",
  date: "2004-04-04",
  filepath: {
    _: "filepath",
    filepath: "photo-xray",
    filehash: "0x4444"
  }
}
```

> - added `broke-leg,2004-04-04` to `event-date.csv`
> - added `broke-leg,photo-xray` to `event-filepath.csv`
> - added `broke-leg,0x4444` to `filepath-filehash.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> broke-leg,2004-04-04
> cooked-lasagna,2002-02-02
> climbed-everest,2003-03-03
> visited-japan,2001-12-12
> ```
> 
> ``` csv
> # event-filepath.csv
> broke-leg,photo-xray
> climbed-everest,photo-everest
> ```
> 
> ``` csv
> # filepath-filehash.csv
> photo-everest,0x0000
> photo-xray,0x4444
> ```

### step 10: delete an attribute from a record

``` javascript
// a record that removes the photo from the event about Everest
{
  _: "event",
  event: "climbed-everest",
  date: "2003-03-03"
}
```

> - removed `climbed-everest,photo-everest` from `event-filepath.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> broke-leg,2004-04-04
> cooked-lasagna,2002-02-02
> climbed-everest,2003-03-03
> visited-japan,2001-12-12
> ```
> 
> ``` csv
> # event-filepath.csv
> broke-leg,photo-xray
> ```
> 
> ``` csv
> # filepath-filehash.csv
> photo-everest,0x0000
> photo-xray,0x4444
> ```

### step 11: add a record with an attribute list

``` javascript
# a record about a wedding, with two photos
{
  _: "event",
  event: "married",
  filepath: [ "photo-bride", "photo-groom" ]
}
```

> - added `married,photo-bride` to `event-filepath.csv`
> - added `married,photo-groom` to `event-filepath.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> visited-japan,2001-12-12
> cooked-lasagna,2002-02-02
> climbed-everest,2003-03-03
> broke-leg,2004-04-04
> ```
> 
> ``` csv
> # event-filepath.csv
> broke-leg,photo-xray
> married,photo-bride
> married,photo-groom
> ```
> 
> ``` csv
> # filepath-filehash.csv
> photo-everest,0x0000
> photo-xray,0x4444
> ```

### step 12: edit an element of an attribute list

``` javascript
// a record that removes the photo of the groom
// and duplicates the photo of the bride
{
  _: "event",
  event: "married",
  filepath: [ "photo-bride", "photo-bride" ]
}
```

> - removed `married,photo-groom` from `event-filepath.csv`
> - added `married,photo-bride` to `event-filepath.csv`
> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event, date
> event, filepath
> filepath, filehash
> filepath, filesize
> ```
> 
> ``` csv
> # event-date.csv
> broke-leg,2004-04-04
> cooked-lasagna,2002-02-02
> climbed-everest,2003-03-03
> visited-japan,2001-12-12
> ```
> 
> ``` csv
> # event-filepath.csv
> broke-leg,photo-xray
> married,photo-bride
> married,photo-bride
> ```
> 
> ``` csv
> # filepath-filehash.csv
> photo-everest,0x0000
> photo-xray,0x4444
> ```

## Update 2

 - update()
   - add relation between two data entities
     - { _: _, entity1: "entity2" }
     - _-_::"entity1,entity2"
   - add a bunch of relations at once
     - { _: _, entity1: "entity2", entity2: "entity3" }
     - _-_::"entity1,entity2\nentity2,entity3"
 - ignore in update()
   - ignore value without "_"
     - { entity1: "entity2" }
   - ignore value "_" in a schema relation file
     - _-_::"_,entity"
     - _-_::"entity,_"
   - writes two records for record when _ is array of two items
     - { _: [ "entity1", "entity2" ], "entity3": "value3" }
     - [{ _: "entity1", "entity3": "value3" }, { _: "value2", "entity3": "value3" }]
   - ignore fields that have no relation to declared base branch
     - { _: "entity1", entity2: "value1", entity2: "value", unrelated_entity: "ignored_value" }
   - ignore array items that have _ other than declared base branch
     - { _: "entity1", entity1: [ { _: "entity1", entity1: "value1"}, { _: "entity2", entity2: "ignored value" }] }

## Select

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> datum, date
> datum, filepath
> filepath, filehash
> filepath, filetype
> filepath, filesize
> ```
> 
> ``` csv
> # datum-date.csv
> datum1,date1Overwrite
> datum2,date2
> datum3,date3
> datum4,date4
> ```
> 
> ``` csv
> # datum-filepath.csv
> datum4,filepath4
> datum5,filepath5a
> datum5,filepath5a
> ```
> 
> ``` csv
> # filepath-filehash.csv
> filepath3,filehash3
> filepath4,filehash4
> ```
### query(queryDatum, fs)

``` javascript
# queryDatum
{
  _: "datum",
  datum: "value1"
}
```

``` javascript
# resultDatum
[
  {
    _: "datum",
    datum: "datum1",
    date: "date1Overwrite"
  }
]
```

### query(queryRegex, fs)

``` javascript
# queryRegex
{
  _: "datum",
  datum: "value[12]"
}
```

``` javascript
# resultRegex
[
  {
    _: "datum",
    datum: "datum1",
    date: "date1Overwrite"
  },
  {
    _: "datum",
    datum: "datum2",
    date: "date2"
  },
]
```

### query(queryOptions, fs)

``` javascript
# queryOptions
{
  _: "datum",
}
```

``` javascript
# resultOptions
[
  {
    _: "datum",
    datum: "datum1",
    date: "date1Overwrite"
  },
  {
    _: "datum",
    datum: "datum2",
    date: "date2"
  },
  {
    _: "datum",
    datum: "datum3",
    date: "date3"
  },
  {
    _: "datum",
    datum: "datum4",
    date: "date4"
    filepath: {
      _: "filepath",
      filepath: "filepath4",
      filehash: "filehash4"
    }
  },
]
```

### query(queryFilter, fs)

``` javascript
# queryfilter
{
  _: "datum",
  filepath: "filepath4"
}
```

``` javascript
# resultFilter
[
  {
    _: "datum",
    datum: "datum4",
    date: "date4"
    filepath: {
      _: "filepath",
      filepath: "filepath4",
      filehash: "filehash4"
    }
  },
]
```

### query(queryFilterNested, fs)

``` javascript
# queryfilterNested
{
  _: "datum",
  filehash: "filehash4"
}
```

### resultFilterNested
``` javascript
[
  {
    _: "datum",
    datum: "datum4",
    date: "date4"
    filepath: {
      _: "filepath",
      filepath: "filepath4",
      filehash: "filehash4"
    }
  },
]
```
