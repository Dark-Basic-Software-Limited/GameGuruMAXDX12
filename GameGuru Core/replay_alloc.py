import sys, collections

# Replay the GGMAX 1.46 allocator op log and find the first illegal operation.
# Ops: A <inst> <offset> <pages> pc=<page_count>   grant
#      D <inst> <offset>                            deferred-free pushed (still reserved)
#      F <inst> <offset>                            immediate free (released to pool)
#      R <inst> <offset>                            drain (released to pool)
# OVERLAP-ALLOC / OOB-ALLOC lines are the in-game tripwire's own verdicts; skipped here.

lines = []
for path in sys.argv[1:]:
    with open(path, 'r', errors='replace') as f:
        lines.extend(f.read().splitlines())

live = collections.defaultdict(dict)      # inst -> {offset: (pages, state)} state: 'live'|'pending'
livenodes = collections.defaultdict(dict) # inst -> {nodeindex: offset} (from m= tokens, if present)
violations = []
history = collections.deque(maxlen=12)
counts = collections.Counter()

def get_m(parts):
    for p in parts:
        if p.startswith('m='):
            return int(p[2:])
    return None

def overlaps(inst, off, pages):
    out = []
    for o, (p, st) in live[inst].items():
        if o < off + pages and off < o + p:
            out.append((o, p, st))
    return out

for idx, ln in enumerate(lines):
    parts = ln.split()
    if not parts:
        continue
    op = parts[0]
    if op not in ('A', 'D', 'F', 'R'):
        counts['tripwire:' + op] += 1
        continue
    inst = parts[1]
    off = int(parts[2])
    counts[op] += 1
    m = get_m(parts)
    if op == 'A':
        pages = int(parts[3])
        pc = int(parts[4].split('=')[1])
        if off + pages > pc:
            violations.append((idx, ln, 'OOB grant', list(history)))
        ov = overlaps(inst, off, pages)
        if ov:
            violations.append((idx, ln, f'grant overlaps live {ov[:4]}', list(history)))
        live[inst][off] = (pages, 'live')
        if m is not None:
            if m in livenodes[inst]:
                violations.append((idx, ln, f'NODE {m} granted while already live at offset {livenodes[inst][m]} (bin self-loop / double-insert)', list(history)))
            livenodes[inst][m] = off
    elif op == 'D':
        if off not in live[inst]:
            violations.append((idx, ln, 'deferred-free of NON-LIVE offset (double free?)', list(history)))
        elif live[inst][off][1] == 'pending':
            violations.append((idx, ln, 'deferred-free of ALREADY-PENDING offset (double free!)', list(history)))
        else:
            live[inst][off] = (live[inst][off][0], 'pending')
        if m is not None and m not in livenodes[inst]:
            violations.append((idx, ln, f'deferred-free of NON-LIVE NODE {m} (double free!)', list(history)))
    elif op in ('F', 'R'):
        if off not in live[inst]:
            violations.append((idx, ln, f'{op} of NON-LIVE offset', list(history)))
        else:
            del live[inst][off]
        if m is not None:
            if m not in livenodes[inst]:
                violations.append((idx, ln, f'{op} of NON-LIVE NODE {m} (double release!)', list(history)))
            else:
                del livenodes[inst][m]
    history.append(ln)

print('op counts:', dict(counts))
print('instances:', {k: len(v) for k, v in live.items()})
print('violations:', len(violations))
for idx, ln, why, hist in violations[:12]:
    print(f'\n--- line {idx}: {ln}\n    WHY: {why}')
    print('    context (prior ops):')
    for h in hist[-8:]:
        print('      ', h)
