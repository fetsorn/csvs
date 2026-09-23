# Tutorial

Here's an example of the simplest csvs dataset that contains a record about visiting Japan in 2001.

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event,date
> ```
> 
> ``` csv
> # event-date.csv
> visited Japan,2001-01-01
> ```

Technically, this dataset represents three records:
 - `event` record that says `visited Japan in 2001-01-01`
 - `date` record that says `2001-01-01`
 - a `_` record, pronounced `schema record`, that says `"event" has a "date"`

Let's add another event about climbing the Everest in 2003

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event,date
> ```
> 
> ``` csv
> # event-date.csv
> visited Japan,2001-01-01
> climbed Everest,2003-03-03
> ```

Now, the dataset represents five records:
 - `event` record that says `visited Japan in 2001-01-01`
 - `event` record that says `climbed Everest in 2003-03-03`
 - `date` record that says `2001-01-01`
 - `date` record that says `2003-03-03`
 - a `_` record, pronounced `schema record`, that says `"event" has a "date"`
 
Let's add another value to the database to show that events happened to different people.

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event,date
> event,name
> ```
> 
> ``` csv
> # event-date.csv
> visited Japan,2001-01-01
> climbed Everest,2003-03-03
> ```
> 
> ``` csv
> # event-name.csv
> visited Japan,Donell
> climbed Everest,Eva
> ```

Finally, the dataset represents seven records:
 - `event` record that says `Donell visited Japan in 2001-01-01`
 - `event` record that says `Eva climbed Everest in 2003-03-03`
 - `date` record that says `2001-01-01`
 - `date` record that says `2003-03-03`
 - `name` record that says `Eva`
 - `name` record that says `Donell`
 - a `_` record, pronounced `schema record`, that says `"event" has a "date" and a "name"`

To remove records from the dataset, delete corresponding lines from the tablets.

Learn more about csvs in the [User guides](./user_guides.md).

## Note
- this page should explain how csvs works without teaching programming 
- should have an explanation for the "listing of files" format, saying it shows the names and the content of the files
- introduces the terminology of a "record" without giving the context
- there should be confirmation of existing knowledge
- there should not be references to specific database alternatives
- there should be real life metaphors like "folder" or "archive", and suggestions like "imagine" or "remember"
- the intention right now is to give data so the reader makes connections on their own, there should be confirmation of right connections and some attention to the possible incorrect presumptions
- the goal of the tutorial is to give some control and awareness, this text right now instead gives a quick technical summary of the csvs API
- this text should be moved to some technical page, perhaps to the README page of reference or user guides
