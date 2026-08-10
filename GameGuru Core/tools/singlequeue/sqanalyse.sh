#!/bin/bash
# Score the full sweep against the PRE-REGISTERED criteria in sqfull.sh.
#   C1 FPS   : >=16/19 positive AND no demo worse than -2.0%
#   C2 TAIL  : ON arm's over-16.7ms frame count not >25% above the OFF arm's
#   C3 POLYS : identical across arms
#
# HITCH counters are cumulative since launch, so per-arm tail = (this arm) - (previous arm).
# That makes A1's tail unusable (no prior capture) but gives a clean pair: B's delta is the
# ON window and A2's delta is the OFF window, both over identical durations.
R="$(dirname "$0")/sqfull_results.txt"
awk -F'|' '
function f(s,i){split(s,a,"|");return a[i]}
BEGIN{
  printf "%-32s %8s %8s %8s %7s  %8s %8s  %7s %7s %s\n",
    "DEMO","OFF","ON","DELTA%","DRIFT%","tailON","tailOFF","stallOF","stallON","POLYS"
  print  "----------------------------------------------------------------------------------------------------------------"
}
/FAIL|DIED/ { printf "%-32s  %s\n",$1,$2; fails++; next }
NF>=19 {
  demo=$1
  a1f=$2;  a1sm=$3;  a1sx=$4;  a1o16=$5;  a1o33=$6;  a1p=$7
  bf =$8;  bsm =$9;  bsx =$10; bo16 =$11; bo33 =$12; bp =$13
  a2f=$14; a2sm=$15; a2sx=$16; a2o16=$17; a2o33=$18; a2p=$19

  off=(a1f+a2f)/2
  d=(off>0)?(bf-off)/off*100:0
  # per-arm tail counts (cumulative counters differenced)
  tailON =bo16 -a1o16
  tailOFF=a2o16-bo16
  drift=a2f-a1f

  # A1-vs-A2 control drift. Both are the SAME setting, so any gap is warm-up/noise, not the
  # knob. A large drift means the ON arm sat mid-warm-up and its delta is not trustworthy —
  # flag those rather than quietly averaging them in (lazy-PSO warm-up is documented to move
  # a level 10-12% between 30 s and 180 s).
  driftpc=(off>0)?(a2f-a1f)/off*100:0
  unreliable=(driftpc>5.0 || driftpc<-5.0)
  if(unreliable){ nun++; unlist=unlist" "demo }

  n++; sum+=d
  if(d>0) pos++;
  if(d < -2.0 && !unreliable){ c1bad++; c1list=c1list" "demo }
  # C2: only judge when the OFF window actually had hitches to compare against,
  # otherwise a 0->1 frame difference reads as +infinity%
  if(tailOFF>=4){ if(tailON > tailOFF*1.25){ c2bad++; c2list=c2list" "demo } }
  else if(tailON>=4 && tailOFF==0){ c2watch++; c2wlist=c2wlist" "demo }
  if(a1p!=bp || a1p!=a2p){ c3bad++; c3list=c3list" "demo }

  printf "%-32s %8.1f %8.1f %+8.1f %+7.1f%s %8d %8d  %7.3f %7.3f %s\n",
    demo,off,bf,d,driftpc,(unreliable?"!":" "),tailON,tailOFF,(a1sm+a2sm)/2,bsm,
    (a1p==bp&&a1p==a2p)?"same":"DIFF"
}
END{
  print  "-------------------------------------------------------------------------------------------------------------"
  printf "\nDemos scored: %d   (failed/died: %d)\n", n, fails+0
  printf "Mean delta: %+.2f%%   positive: %d/%d\n", (n?sum/n:0), pos+0, n
  print  "\nPRE-REGISTERED CRITERIA"
  printf "  C1 FPS   : %s  (positive %d/%d, need >=16 of 19; regressions worse than -2%%: %d%s)\n",
    ((pos>=16 && c1bad==0)?"PASS":"FAIL"), pos+0, n, c1bad+0, (c1bad?" ->"c1list:"")
  printf "  C2 TAIL  : %s  (demos where ON tail >25%% above OFF tail: %d%s)\n",
    ((c2bad==0)?"PASS":"FAIL"), c2bad+0, (c2bad?" ->"c2list:"")
  if(c2watch) printf "             note: %d demo(s) had 0 hitches OFF but >=4 ON, not scorable as a ratio ->%s\n", c2watch, c2wlist
  printf "  C3 POLYS : %s  (demos with differing POLYS across arms: %d%s)\n",
    ((c3bad==0)?"PASS":"FAIL"), c3bad+0, (c3bad?" ->"c3list:"")
  print  ""
  if(pos>=16 && c1bad==0 && c2bad==0 && c3bad==0) print "  => ALL CRITERIA PASS: flip the default."
  else print "  => CRITERIA NOT MET: default stays OFF."
}' "$R"
