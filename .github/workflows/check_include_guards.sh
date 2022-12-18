#!/bin/bash
# @file      check_include_guards.sh
# @brief     Check include guards
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

# Initialize list of files that are not properly formatted
result=""

# Check header files
for source_file in `git ls-tree -r HEAD --name-only | grep -E '(.*\.h$|.*\.hpp$)' | grep -vFf .clang-format-ignore`
do
    uppercase=$(echo $(basename ${source_file^^}) | tr '.' '_' | tr '-' '_')
    pcregrep -Me "#ifndef __${uppercase}__\n#define __${uppercase}__" ${source_file} > /dev/null 2>&1
    if [[ ! $? -eq 0 ]]; then
        result="${result}\n${source_file}"
    else
        pcregrep -Me "#endif /\* __${uppercase}__ \*/" ${source_file} > /dev/null 2>&1
        if [[ ! $? -eq 0 ]]; then
            result="${result}\n${source_file}"
        fi
   fi
done

# Check result
if [[ ${result} != "" ]]; then
    echo "The following files have wrong or missing include guards:"
    echo -e $result
    exit 1
fi

exit 0

