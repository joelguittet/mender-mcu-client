#!/bin/bash
# @file      check_headers.sh
# @brief     Check headers
#
# MIT License
#
# Copyright (c) 2022-2023 joelguittet and mender-mcu-client contributors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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
    pcregrep -Me "${first_line}${first_line:+\n}${new_line} @file      ${filename}\n${new_line} @brief     [[:print:]]*\n${new_line}\n${new_line} MIT License\n${new_line}\n${new_line} Copyright \(c\) 2022-2023 joelguittet and mender-mcu-client contributors\n${new_line}\n${new_line} Permission is hereby granted, free of charge, to any person obtaining a copy\n${new_line} of this software and associated documentation files \(the \"Software\"\), to deal\n${new_line} in the Software without restriction, including without limitation the rights\n${new_line} to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n${new_line} copies of the Software, and to permit persons to whom the Software is\n${new_line} furnished to do so, subject to the following conditions:\n${new_line}\n${new_line} The above copyright notice and this permission notice shall be included in all\n${new_line} copies or substantial portions of the Software.\n${new_line}\n${new_line} THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n${new_line} IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n${new_line} FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n${new_line} AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n${new_line} LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n${new_line} OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n${new_line} SOFTWARE.${last_line:+\n}${last_line}\n" ${source_file} > /dev/null 2>&1
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

