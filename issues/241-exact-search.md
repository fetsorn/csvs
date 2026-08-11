# Document Title

user wants there to be a way to search for entries that match a combination keywords exactly, and do not have other fields.
this might be against evenor's and csvs data structure, but then there should be a discussion of it in the docs.

related but different issue is when a value matches exactly, rather than fuzzily


right now you can't look for a field that has no value set. so I have to set should be able to.

IS NULL in SQL

could also search for fields having a set amount of values. should I allow numbers in csvs and son for this?

could just be a "^$" regex if I wanted to filter out those where is not null. but how to match on null?

could just filter out in post

should I allow null in csvs and son for this?

where is this used? might be a "won't fix"
