# Lists of Values

repeat a line with the same key to represent a list of values. For example, let's say Donell visited Japan every year for three years. 

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event,date
> event,name
> branch,description
> ```
> 
> ``` csv
> # event-date.csv
> visited Japan,2001-01-01
> visited Japan,2002-02-02
> visited Japan,2003-03-03
> climbed Everest,2003-03-03
> ```
> 
> ``` csv
> # event-name.csv
> visited Japan,Donell
> climbed Everest,Eva
> ```

Notice that the year 2003 repeats two times - once in the list of events about Japan, and once in the list of events about Everest. Be careful when you add new branches that describe the date "2003-03-03" - they might apply to both mentions!

To avoid conflicts, make sure to use unique identifiers when you want singleton values. For example, use a [unique identifier](https://en.wikipedia.org/wiki/Universally_unique_identifier) as a key.

empty value is a value, one comma is empty key to an empty value

empty line is not a value

To learn more about csvs, see other [User Guides](./README.md) and the [Requirements](../requirements.md).
