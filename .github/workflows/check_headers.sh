#!/bin/bash
# @file      check_headers.sh
# @brief     Check headers
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

# Function used to check headers
# Arg1: Source file path
# Arg2: First line, "" if it is not present
# Arg3: String added in front of each new line except the first and last lines
# Arg4: Last line, "" if it is not present
check_header() {
    source_file=$1
    first_line=$2
    new_line=$3
    last_line=$4
    filename=$(echo $(basename ${source_file}) | sed -r 's/\+/\\+/g')
    pcregrep -Me "${first_line}${first_line:+\n}${new_line} @file      ${filename}\n${new_line} @brief     [[:print:]]*\n${new_line}\n${new_line} Copyright joelguittet and mender-mcu-client contributors\n${new_line}\n${new_line} Licensed under the Apache License, Version 2.0 \(the \"License\"\);\n${new_line} you may not use this file except in compliance with the License.\n${new_line} You may obtain a copy of the License at\n${new_line}\n${new_line}     http://www.apache.org/licenses/LICENSE-2.0\n${new_line}\n${new_line} Unless required by applicable law or agreed to in writing, software\n${new_line} distributed under the License is distributed on an \"AS IS\" BASIS,\n${new_line} WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n${new_line} See the License for the specific language governing permissions and\n${new_line} limitations under the License.${last_line:+\n}${last_line}\n" ${source_file} > /dev/null 2>&1
    return $?
}

# Initialize list of files that are not properly formatted
result=""

# Check source, header, ASM and linker files
for source_file in `git ls-tree -r HEAD --name-only | grep -E '(.*\.c$|.*\.h$|.*\.cpp$|.*\.hpp$|.*\.s$|.*\.ld$)' | grep -vFf .clang-format-ignore`
do
    check_header ${source_file} "/\*\*" " \*" " \*/"
    if [[ ! $? -eq 0 ]]; then
        result="${result}\n${source_file}"
    fi
done

# Check YAML, Python, CMake and configuration files
for source_file in `git ls-tree -r HEAD --name-only | grep -E '(.*\.yml$|.*\.py$|CMakeLists.*\.txt$|.*\.cmake$|.*\.cfg$|.*\.conf$|.clang-format$)' | grep -vFf .clang-format-ignore`
do
    check_header ${source_file} "" "#" ""
    if [[ ! $? -eq 0 ]]; then
        result="${result}\n${source_file}"
    fi
done

# Check bash files
for source_file in `git ls-tree -r HEAD --name-only | grep -E '(.*\.sh$)' | grep -vFf .clang-format-ignore`
do
    check_header ${source_file} "#!/bin/bash" "#" ""
    if [[ ! $? -eq 0 ]]; then
        result="${result}\n${source_file}"
    fi
done

# Check result
if [[ ${result} != "" ]]; then
    echo "The following files have wrong header format:"
    echo -e $result
    exit 1
fi

exit 0

