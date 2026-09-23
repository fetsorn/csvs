# Unique keys

collection "text" with large multiline string values CAN be refactored into two collections - "text_hash" where each value is a hash of a `text` value, to create a content-addressable index of text records.

To learn more about csvs, see other [User Guides](./README.md) and the [Requirements](../requirements.md).
