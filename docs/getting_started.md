# Getting Started

A csvs dataset is a directory that contains plain text files in the "comma-separated value" format, or CSV. Any directory that contains a `.csvs.csv` file is a valid csvs dataset. Each CSV file represents a table with two columns and is called a "tablet". The first column is a `key`, and the second column is a `value`. You can store records by appending lines to the tablets. To represent complex objects and connect the tablets to each other, specify the relationships between values in the schema file `_-_.csv`.

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
> visited-japan,2001-01-01
> ```

To learn more about csvs, see the [Tutorial](./tutorial.md) and the [User guides](./user_guides/README.md).

## Note:
- too much information for getting started
- the example is cryptic at this point
- the example is evocative of what is to come, good
- this text should somewhere technical-oriented
- this page should have text oriented at non-technical consumers
- this is for a curious consumer, not an engineer
