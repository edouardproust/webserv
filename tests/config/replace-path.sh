#!/bin/bash

# Configuration modification script
# Usage: ./script.sh <source_file> <output_directory>

# Parameter validation
if [ $# -ne 2 ]; then
	echo "Usage: $0 <source_file> <output_directory>"
	exit 1
fi

PLACEHOLDER="PATH_TO_WEBSERV_DIR"
SOURCE_FILE="$1"
OUTPUT_DIR="$2"
CURRENT_DIR=$(pwd)

# Verify source file exists
if [ ! -f "$SOURCE_FILE" ]; then
	echo "Error: Source file $SOURCE_FILE does not exist"
	exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
if [ $? -ne 0 ]; then
	echo "Error: Cannot create directory $OUTPUT_DIR"
	exit 1
fi

# Generate output file path
FILENAME=$(basename "$SOURCE_FILE")
MODIFIED_FILE="$OUTPUT_DIR/$FILENAME"

# Escape special characters in current directory path for sed
ESCAPED_CURRENT_DIR=$(printf '%s\n' "$CURRENT_DIR" | sed 's/[[\.*^$()+?{|]/\\&/g')

# Create modified version with PATH_TO_WEBSERV_DIR replaced
sed "s|$PLACEHOLDER|$ESCAPED_CURRENT_DIR|g" "$SOURCE_FILE" > "$MODIFIED_FILE"

# Verify the modification was successful
if [ $? -eq 0 ]; then
	echo "Config success:"
	echo "- Created file: $MODIFIED_FILE"
	echo "- Original file: $SOURCE_FILE (unchanged)"
	echo "- \"$PLACEHOLDER\" replaced with \"$CURRENT_DIR\""
	echo ""
else
	echo "Error: Failed to create modified file"
	exit 1
fi