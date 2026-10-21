#!/bin/sh

set -eu

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/mecab-writer.XXXXXX")
trap 'rm -rf "$tmpdir"' 0
trap 'exit 1' 1 2 15

srcdir=${srcdir:-.}
../src/mecab-dict-index -d "$srcdir/latin" -o "$tmpdir" \
  -f euc-jp -t euc-jp > "$tmpdir/dictionary.log"
cp "$srcdir/latin/dicrc" "$tmpdir/dicrc"
printf 'a\n' > "$tmpdir/input"

# Override the dictionary's yomi output style with unsuffixed formats.
../src/mecab -r /dev/null -d "$tmpdir" -O '' -E '' \
  -F '%m%%\n' -U '%m%%\n' < "$tmpdir/input" \
  > "$tmpdir/actual" 2> "$tmpdir/stderr"
printf 'a%%\n' > "$tmpdir/expected"
diff "$tmpdir/expected" "$tmpdir/actual"
test ! -s "$tmpdir/stderr"

for char in q '?'
do
  status=0
  ../src/mecab -r /dev/null -d "$tmpdir" -O '' -E '' \
    -F "%$char" -U "%$char" < "$tmpdir/input" \
    > "$tmpdir/actual" 2> "$tmpdir/stderr" || status=$?
  if [ "$status" -ne 1 ]; then
    echo "Expected exit status 1 for %$char, got $status" >&2
    exit 1
  fi
  printf 'unknown meta char: %s\n' "$char" > "$tmpdir/expected"
  diff "$tmpdir/expected" "$tmpdir/actual"
  test ! -s "$tmpdir/stderr"
done
