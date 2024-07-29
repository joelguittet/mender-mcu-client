#!/bin/bash
# @file      check_equivalence_tests.sh
# @brief     Check equivalence tests
#
# Copyright joelguittet and mender-mcu-client contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Initialize list of files that are not properly formatted
result=""

# Check source and header files
for source_file in `git ls-tree -r HEAD --name-only | grep -E '(.*\.c$|.*\.h$|.*\.cpp$|.*\.hpp$)' | grep -vFf .clang-format-ignore`
do
    # This grep matches following formats (which are not allowed):
    # - "variable == CONSTANT" / "variable != CONSTANT"
    # - "function(...) == CONSTANT" / "function(...) != CONSTANT"
    # - "table[...] == CONSTANT" / "table[...] != CONSTANT"
    # Where CONSTANT can be a definition or a number
    # Any number of spaces before and after "==" / "!=" is catched
    lines=$(grep "[])a-z][ ]*[!=]=[ ]*[A-Z0-9]" ${source_file} | wc -l)
    if [[ ! $lines -eq 0 ]]; then
        result="${result}\n${source_file}"
    fi
done

# Check result
if [[ ${result} != "" ]]; then
    echo "The following files are not properly formatted:"
    echo -e $result
    exit 1
fi

exit 0

