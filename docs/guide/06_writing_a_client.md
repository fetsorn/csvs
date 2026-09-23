# Writing a Client

Let's write a simple client for csvs. We will use pseudocode so you can follow along in your favorite language.

A client library defines three functions that mirror SQL's SELECT, INSERT and DELETE commands.
```
# finds a branch key that is connected to `value`
select branch value =
  # find a leaf of the branch
  schema = parse "_-_.csv"
  relation = find line in schema where line contains branch
  tokens = split relation ","
  leaf = tokens[1]
  # find a value in the tablet where leaf matches value
  tablet = parse "$branch-$leaf.csv"
  relation = find line in tablet where line contains value
  tokens = split relation ","
  value = tokens[0]
  return value
  
# adds a trunk key connected to a leaf value
insert trunk leaf key value =
  line = "$key,$value"
  append "$trunk-$leaf" line
  
# removes the branch key and the connected leaf value
delete branch key = 
  # find a leaf of the branch
  schema = parse "_-_.csv"
  relation = find line in schema where line contains branch
  tokens = split relation ","
  leaf = tokens[1]
  # find a value in the tablet where branch matches key
  tablet = parse "$branch-$leaf.csv"
  relation = find line in tablet where line contains key
  # delete the line from the tablet
  filter tablet relation
```

This can be implemented in an evening. A more carefully written client could be much more robust and efficient - try to write your own!

To learn more about csvs, see other [User Guides](./README.md) and the [Requirements](../requirements.md).
