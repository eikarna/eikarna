#!/bin/bash

# Function to generate random string and get MD5 hash
generate_random_md5() {
    # Generate random string using date and random number
    random_string=$(date +%s%N | md5sum | head -c 32)
    
    # Get MD5 hash of the random string
    full_hash=$(echo -n "$random_string" | md5sum | cut -d' ' -f1)
    
    # Get first 6 characters
    short_hash=${full_hash:0:6}
    
    echo "$short_hash"
}

# Function to check if required commands exist
check_dependencies() {
    for cmd in md5sum date; do
        if ! command -v $cmd &> /dev/null; then
            echo "Error: Required command '$cmd' not found"
            exit 1
        fi
    done
}

# Main execution
main() {
    # Check dependencies first
    check_dependencies
    
    # Number of hashes to generate (default: 1)
    num_hashes=${1:-1}
    
    # Validate input
    if ! [[ "$num_hashes" =~ ^[0-9]+$ ]]; then
        echo "Error: Please provide a valid number"
        exit 1
    fi
    
    # Generate specified number of hashes
    for ((i=1; i<=num_hashes; i++)); do
        generate_random_md5
    done
}

# Run main function with first argument
main "$1"
