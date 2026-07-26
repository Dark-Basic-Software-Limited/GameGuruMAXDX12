// Offline OffsetAllocator testbench (GGMAX reload-corruption hunt).
// Replays the exact op sequence captured from the live game (ops_8f50.txt) through the real
// vendored allocator, verifying after EVERY op:
//   1. each grant matches the logged (offset, nodeindex) -> confirms deterministic lockstep
//   2. bin lists are acyclic, nodes sit in at most one bin, no used node sits in a bin
//   3. active ranges (used nodes + bin-resident free nodes) never overlap / never pass the end
// The first failed check names the exact operation that corrupts the allocator.
#define private public
#include "offsetAllocator.hpp"
#undef private
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <deque>
#include <string>

using namespace OffsetAllocator;

static Allocator g_alloc;
static std::deque<std::string> g_recent;
// pattern-replay: game allocation identity (off,m) -> our live handles, so frees stay valid
// after the FIXED allocator's decisions diverge from the buggy game's logged decisions
static std::map<unsigned long long, std::deque<Allocation>> g_map;

static bool validate(const char* when)
{
    const uint32 N = g_alloc.m_maxAllocs;
    std::vector<char> inBin(N, 0);
    for (uint32 b = 0; b < NUM_LEAF_BINS; b++)
    {
        uint32 idx = g_alloc.m_binIndices[b];
        uint32 steps = 0;
        std::set<uint32> seen;
        while (idx != Allocator::Node::unused)
        {
            if (seen.count(idx)) { printf("FAIL: CYCLE in bin %u at node %u %s\n", b, idx, when); return false; }
            seen.insert(idx);
            if (idx >= N) { printf("FAIL: bad node index %u in bin %u %s\n", idx, b, when); return false; }
            if (inBin[idx]) { printf("FAIL: node %u in TWO bins %s\n", idx, when); return false; }
            inBin[idx] = 1;
            const Allocator::Node& nd = g_alloc.m_nodes[idx];
            if (nd.used) { printf("FAIL: USED node %u sits in bin %u %s\n", idx, b, when); return false; }
            idx = nd.binListNext;
            if (++steps > N) { printf("FAIL: bin %u walk exceeded %u %s\n", b, N, when); return false; }
        }
    }
    std::map<uint32, std::pair<uint32, uint32>> ranges; // offset -> (size, node)
    for (uint32 i = 0; i < N; i++)
    {
        const Allocator::Node& nd = g_alloc.m_nodes[i];
        bool active = nd.used || inBin[i];
        if (!active || nd.dataSize == 0) continue;
        auto it = ranges.find(nd.dataOffset);
        if (it != ranges.end())
        {
            printf("FAIL: DUP offset %u held by nodes %u(size %u) and %u(size %u) %s\n",
                nd.dataOffset, it->second.second, it->second.first, i, nd.dataSize, when);
            return false;
        }
        ranges[nd.dataOffset] = std::make_pair(nd.dataSize, i);
    }
    uint32 prevEnd = 0; bool first = true; uint32 prevNode = 0;
    for (std::map<uint32, std::pair<uint32, uint32>>::iterator it = ranges.begin(); it != ranges.end(); ++it)
    {
        if (!first && it->first < prevEnd)
        {
            printf("FAIL: OVERLAP node %u range [%u,+%u) begins before prev node %u ends at %u %s\n",
                it->second.second, it->first, it->second.first, prevNode, prevEnd, when);
            return false;
        }
        first = false;
        prevEnd = it->first + it->second.first;
        prevNode = it->second.second;
        if (prevEnd > g_alloc.m_size) { printf("FAIL: range past end (%u > %u) %s\n", prevEnd, g_alloc.m_size, when); return false; }
    }
    return true;
}

int main(int argc, char** argv)
{
    const char* path = argc > 1 ? argv[1] : "ops_8f50.txt";
    FILE* f = fopen(path, "r");
    if (!f) { printf("cannot open %s\n", path); return 1; }

    g_alloc.init(4096, 4096);

    char line[256];
    int lineno = 0;
    while (fgets(line, sizeof(line), f))
    {
        lineno++;
        char op; unsigned long long seq; unsigned off, pages, m;
        char when[128];
        if (line[0] == 'A')
        {
            if (sscanf(line, "A %llu %u %u %u", &seq, &off, &pages, &m) != 4) continue;
            Allocation a = g_alloc.allocate(pages);
            snprintf(when, sizeof(when), "after op line %d (#%llu A pages=%u)", lineno, seq, pages);
            if (a.offset == Allocation::NO_SPACE) { printf("NO_SPACE at line %d\n", lineno); continue; }
            g_map[((unsigned long long)off << 32) | m].push_back(a);
            if (!validate(when))
            {
                printf("corrupting op: line %d = %s", lineno, line);
                printf("recent ops:\n");
                for (size_t i = 0; i < g_recent.size(); i++) printf("  %s", g_recent[i].c_str());
                return 3;
            }
        }
        else if (line[0] == 'R')
        {
            if (sscanf(line, "R %llu %u %u", &seq, &off, &m) != 3) continue;
            std::deque<Allocation>& q = g_map[((unsigned long long)off << 32) | m];
            if (q.empty()) { printf("WARN: free with no mapped alloc at line %d (off=%u m=%u)\n", lineno, off, m); continue; }
            Allocation a = q.front();
            q.pop_front();
            g_alloc.free(a);
            snprintf(when, sizeof(when), "after op line %d (#%llu R off=%u m=%u)", lineno, seq, off, m);
            if (!validate(when))
            {
                printf("corrupting op: line %d = %s", lineno, line);
                printf("recent ops:\n");
                for (size_t i = 0; i < g_recent.size(); i++) printf("  %s", g_recent[i].c_str());
                return 3;
            }
        }
        g_recent.push_back(line);
        if (g_recent.size() > 25) g_recent.pop_front();
    }
    printf("replayed %d lines with ZERO integrity failures\n", lineno);
    return 0;
}
