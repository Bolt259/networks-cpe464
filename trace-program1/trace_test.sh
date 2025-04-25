#!/bin/bash

# This script is used to test the trace program
# It will run the trace program with all test .pcap files and diff them against the .out files

echo "Running trace tests..."

# Directory for temporary output files
TEMP_DIR="./temp_out"
mkdir -p "$TEMP_DIR" # Create the directory if it doesn't exist

# Loop through all .pcap files in the test directory
for pcap_file in ./*.pcap; do
    # Get the base name of the pcap file (without the directory and extension)
    base_name=$(basename "$pcap_file" .pcap)
    
    # Run the trace program with the pcap file and redirect output to a temporary .out file
    temp_out_file="$TEMP_DIR/$base_name.out"
    ./trace "$pcap_file" > "$temp_out_file"
    
    # Compare the generated .out file with the original .out file
    if diff -q "$temp_out_file" "$base_name.out" > /dev/null; then
        echo "$base_name: Test passed"
    else
        echo "$base_name: Test failed"
        echo "Differences:"
        diff "$temp_out_file" "$base_name.out"
    fi
done

# Clean up temporary output files
rm -rf "$TEMP_DIR"

echo "All tests completed."

# Notes:
# - The script creates a temporary directory (`temp_out`) to store generated .out files.
# - The original .out files are preserved and not overwritten.
# - Temporary .out files are removed after the script finishes.