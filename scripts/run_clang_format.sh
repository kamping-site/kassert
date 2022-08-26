#!/bin/bash

if [[ "$PWD" == */scripts ]]; then
	echo "Script must be run from the project's root directory"
	exit 1
fi

for directory in "include" "tests"; do 
	find $directory -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} \;
done
