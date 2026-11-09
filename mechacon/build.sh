#!/usr/bin/env bash
# Assembles the MechaCon window and the hook code of every profile with clang / ld.lld (arm7tdmi, Thumb)
# and checks them against manifest.json. The hook container (profile.h) is the plaintext encrypted with
# the MechaCon patch key, which is not part of this repository.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p out
for d in profiles/*/; do
  k=$(basename "$d")
  clang --target=arm-none-eabi -mcpu=arm7tdmi -mthumb -I "$d" -c window.S -o out/window_$k.o
  ld.lld -m armelf -T "$d/window.ld" out/window_$k.o -o out/window_$k.elf
  llvm-objcopy -O binary out/window_$k.elf out/window_$k.bin
  clang --target=arm-none-eabi -mcpu=arm7tdmi -mthumb -I "$d" -c pmap_hook.S -o out/hook_$k.o
  ld.lld -m armelf -T "$d/hook.ld" out/hook_$k.o -o out/hook_$k.elf
  llvm-objcopy -O binary out/hook_$k.elf out/hook_$k.bin
done
python3 - <<'PY'
import hashlib, json
m = json.load(open('manifest.json'))
for k, v in m.items():
    w = hashlib.sha256(open('out/window_%s.bin' % k, 'rb').read()).hexdigest()
    h = open('out/hook_%s.bin' % k, 'rb').read()
    pt = open('profiles/%s/hook_plaintext.bin' % k, 'rb').read()
    ok = w == v['window_sha256'] and hashlib.sha256(h).hexdigest() == v['hook_code_sha256'] and pt[0x20:0x20 + len(h)] == h
    print('%-6s window %s  hook %s' % (k, 'ok' if w == v['window_sha256'] else 'DIFF', 'ok' if ok else 'DIFF'))
    assert ok, k
PY
