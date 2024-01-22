#!/bin/zsh

# Define the directory where the new file will be saved
destination_directory="./sample_codes"

# Check if the destination directory exists
if [[ ! -d "$destination_directory" ]]; then
    echo "Error: Destination directory '$destination_directory' does not exist."
    exit 1
fi

# Ask the user for the name of the new file
printf "Enter the name for the new file : "
read new_filename

# Full path for the new file
new_file_path="${destination_directory}/${new_filename}"

# Check if the new file already exists in the destination directory
if [[ -f "$new_file_path" ]]; then
    echo "Error: File '$new_file_path' already exists."
    exit 1
fi

# Path to the main.c file
main_c_file="main.c"

# Check if main.c exists
if [[ ! -f "$main_c_file" ]]; then
    echo "Error: The source file 'main.c' does not exist."
    exit 1
fi

# Copy the content from main.c to the new file in the destination directory
cp "$main_c_file" "$new_file_path"

echo "File '$new_filename' has been created in '$destination_directory' with the content of 'main.c'."
