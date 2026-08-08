#!/bin/bash
# Join two demo_fps_sweep result files by demo name and flag regressions.
# Usage: compare_sweep.sh <oldtag> <newtag>
# Gates:  FPS delta >= 10% (rig drift band is +-8-15%, so 10% is "look at it")
#         in-game VRAM >= 4096 MB (the 4 GB min-spec gate)
#         POLYS must be BIT-IDENTICAL (the standing acceptance test for pipeline changes)
# Results live wherever demo_fps_sweep.sh wrote them; override with DEMO_FPS_DIR.
O="${DEMO_FPS_DIR:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/demo_fps}"
OLD="$O/results_${1}.txt"
NEW="$O/results_${2}.txt"

awk -F'|' -v oldf="$OLD" -v newf="$NEW" -v oldtag="$1" -v newtag="$2" '
function avg3(a,b,c,   n,s) { n=0; s=0
  if (a+0>0) {s+=a; n++} ; if (b+0>0) {s+=b; n++} ; if (c+0>0) {s+=c; n++}
  return n? s/n : 0 }
function pct(o,n) { return (o>0)? (n-o)/o*100 : 0 }
BEGIN {
  while ((getline line < oldf) > 0) { n=split(line,f,/\|/); if (n<12) continue
    d=f[1]; oed[d]=avg3(f[3],f[4],f[5]); ogm[d]=avg3(f[7],f[8],f[9]);
    ov[d]=f[11]+0; op[d]=f[12]; ogv[d]=f[13]; sub("gvram=","",ogv[d]); ogv[d]=ogv[d]+0 }
  while ((getline line < newf) > 0) { n=split(line,f,/\|/); if (n<12) continue
    d=f[1]; order[++cnt]=d; st[d]=f[2]; ned[d]=avg3(f[3],f[4],f[5]); ngm[d]=avg3(f[7],f[8],f[9]);
    gst[d]=f[6]; nv[d]=f[11]+0; np[d]=f[12]; ngv[d]=f[13]; sub("gvram=","",ngv[d]); ngv[d]=ngv[d]+0 }

  printf "%-32s %-16s %-18s %-9s %-9s %s\n", "DEMO", "EDITOR "oldtag">"newtag, "GAME "oldtag">"newtag, "gVRAM", "POLYS", "FLAGS"
  printf "%s\n", "--------------------------------------------------------------------------------------------------------"
  for (i=1;i<=cnt;i++) { d=order[i]
    flags=""
    if (st[d]!="OK") flags=flags" LOAD-FAIL("st[d]")"
    ep=pct(oed[d],ned[d]); gp=pct(ogm[d],ngm[d])
    if (oed[d]>0 && ep<=-10) flags=flags" EDITOR"sprintf("%.0f%%",ep)
    if (ogm[d]>0 && gp<=-10) flags=flags" GAME"sprintf("%.0f%%",gp)
    if (ngv[d]>=4096) flags=flags" VRAM-OVER-4GB"
    if (op[d]!="" && np[d]!="" && op[d]!=np[d]) flags=flags" POLYS-CHANGED"
    if (gst[d]!="GAME" && gst[d]!="GAME_RECHECK") flags=flags" GAMESTATE="gst[d]
    if (flags=="") flags="ok"
    polys=np[d]; sub("POLYS: ","",polys)
    printf "%-32s %6.1f>%6.1f %+5.0f%%  %6.1f>%6.1f %+5.0f%%  %5.0f>%5.0f  %-9s %s\n", \
      d, oed[d], ned[d], ep, ogm[d], ngm[d], gp, ogv[d], ngv[d], polys, flags
    sed_o+=oed[d]; sed_n+=ned[d]; sgm_o+=ogm[d]; sgm_n+=ngm[d]
    if (ngv[d]>maxgv) { maxgv=ngv[d]; maxgvd=d }
  }
  printf "%s\n", "--------------------------------------------------------------------------------------------------------"
  printf "SUM editor %.1f -> %.1f (%+.1f%%)   SUM game %.1f -> %.1f (%+.1f%%)   worst in-game VRAM %.0f MB (%s)\n", \
    sed_o, sed_n, pct(sed_o,sed_n), sgm_o, sgm_n, pct(sgm_o,sgm_n), maxgv, maxgvd
  printf "demos compared: %d\n", cnt
}' /dev/null
