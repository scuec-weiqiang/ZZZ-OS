#!/bin/sh

set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <kernel.elf> <output.c>" >&2
    exit 1
fi

elf="$1"
out="$2"
nm_bin="${NM:-nm}"

"$nm_bin" -n "$elf" | awk '
BEGIN {
    print "#include <os/kallsyms.h>"
    print ""
    print "const struct kernel_symbol __kallsyms[] = {"
}
$2 ~ /^[tTwW]$/ {
    name = $3
    if (name == "" || name ~ /^[.$]/) {
        next
    }
    if (name ~ /^__/) {
        next
    }
    if (name ~ /^_/ && name != "_start") {
        next
    }
    if (name ~ /_(start|end)$/ && name != "_start") {
        next
    }
    printf("    { 0x%sUL, \"%s\" },\n", $1, name)
    count++
}
END {
    print "};"
    print ""
    printf("const unsigned int __kallsyms_count = %u;\n", count)
}
' > "$out"
