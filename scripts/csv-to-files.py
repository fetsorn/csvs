#!/usr/bin/env python3
"""Turn a two-column CSV into one file per row.

Usage: csv-to-files.py -d <output-dir> <csv-file>

First column  → filename
Second column → file content (CSV-escaped, written as-is after CSV parsing)
"""
import argparse
import csv
import os
import sys

csv.field_size_limit(sys.maxsize)


def main():
    parser = argparse.ArgumentParser(description="Expand CSV rows into files")
    parser.add_argument("-d", "--dir", required=True, help="Output directory")
    parser.add_argument("csv_file", help="Input CSV file")
    args = parser.parse_args()

    os.makedirs(args.dir, exist_ok=True)

    with open(args.csv_file, newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        for lineno, row in enumerate(reader, start=1):
            if len(row) < 2:
                continue
            name, value = row[0], row[1]
            if not name:
                print(f"error: empty filename at line {lineno}, skipping", file=sys.stderr)
                continue
            # unescape literal \n sequences that aren't real newlines
            value = value.replace("\\n", "\n")
            path = os.path.join(args.dir, name)
            with open(path, "w", encoding="utf-8") as out:
                out.write(value)


if __name__ == "__main__":
    main()
