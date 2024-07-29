#!/bin/bash
# @file      check_include_guards.sh
# @brief     Check include guards
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

# Check header files
for source_file in `git ls-tree -r HEAD --name-only | grep -E '(.*\.h$|.*\.hpp$)' | grep -vFf .clang-format-ignore`
do
    uppercase=$(echo $(basename ${source_file^^}) | tr '.' '_' | tr '-' '_')
    pcregrep -Me "#ifndef __${uppercase}__\n#define __${uppercase}__\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif /\* __cplusplus \*/" ${source_file} > /dev/null 2>&1
    if [[ ! $? -eq 0 ]]; then
        result="${result}\n${source_file}"
    else
        pcregrep -Me "#ifdef __cplusplus\n}\n#endif /\* __cplusplus \*/\n\n#endif /\* __${uppercase}__ \*/" ${source_file} > /dev/null 2>&1
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

