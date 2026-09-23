# Version Control

csvs dataset SHOULD be version controlled.
credentials and other sensitive data SHOULD be stored separately from the dataset, e.g. in `.git/config` or in another csvs dataset under access control.

## Concurrency

csvs is designed for **single-writer access**. Only one process should write to a dataset directory at a time. Running panrec and evenor against the same dataset simultaneously may corrupt tablet files, because there is no file-locking mechanism.

For multi-device access, use **git-based sync**: each device commits its changes locally and merges via `git pull`/`git push`. Git's merge semantics handle concurrent edits at the line level, and evenor's `resolve` operation performs three-way merge when conflicts arise.

If a client builds the schema once and reuses it across operations (see the `schema` option in csvs-js or `with_schema()` in csvs-rs), note that the cached schema becomes stale after any operation that modifies `_-_.csv`. Rebuild the schema after schema-modifying writes.

To learn more about csvs, see other [User Guides](./README.md) and the [Requirements](../requirements.md).
