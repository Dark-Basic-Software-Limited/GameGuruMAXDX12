#!/bin/bash
# Score the test-game + TESTPRO1 pass. Same reading as the editor sweep: judge the ON arm
# against the MEAN of the two OFF arms, and treat a large A1-vs-A2 control drift as a warning
# that the ON arm sat mid-warm-up rather than as a result.
R="$(dirname "$0")/sqgame_results.txt"
awk -F'|' '
BEGIN{ printf "%-34s %8s %8s %8s %7s  %7s %7s  %s\n","CASE","OFF","ON","DELTA%","DRIFT%","stallOF","stallON","POLYS"
       print "-------------------------------------------------------------------------------------------------" }
/SKIP|FAIL|DIED/ { printf "%-34s  %s\n",$1,$2; next }
NF>=16 {
  c=$1; a1=$2; a1sm=$3; a1p=$6
  b=$7;  bsm=$8;  bp=$11
  a2=$12; a2sm=$13; a2p=$16
  off=(a1+a2)/2; d=(off>0)?(b-off)/off*100:0; dr=(off>0)?(a2-a1)/off*100:0
  # ⚠ VSYNC-PINNED cases are not scorable. Test-game honours the per-level VSync setting, so a
  # level that can hold the refresh rate reads exactly ~60.0 on BOTH arms — that is the monitor
  # talking, not the knob. Counting it as "neutral" would dilute the real result in both
  # directions. (It also makes over-16.7ms counts meaningless: a 60 Hz frame IS 16.67 ms, so
  # the counter ticks nearly every frame regardless of arm.)
  vs=(off>59.0 && off<61.0 && b>59.0 && b<61.0)
  if(vs){ nvs++; vsl=vsl" "c
    printf "%-34s %8.1f %8.1f %8s %+7.1f  %7.3f %7.3f  %s\n",c,off,b,"vsync",dr,(a1sm+a2sm)/2,bsm,(a1p==bp&&a1p==a2p)?"same":"DIFF"
  } else {
    n++; sum+=d; if(d>0) pos++; if(d<-2.0){bad++; badl=badl" "c}
    printf "%-34s %8.1f %8.1f %+8.1f %+7.1f%s %7.3f %7.3f  %s\n",
      c,off,b,d,dr,((dr>5||dr<-5)?"!":" "),(a1sm+a2sm)/2,bsm,(a1p==bp&&a1p==a2p)?"same":"DIFF"
  }
}
END{ print "-------------------------------------------------------------------------------------------------"
  printf "\nScorable cases: %d   positive: %d   mean delta: %+.2f%%\n", n, pos+0, (n?sum/n:0)
  if(nvs) printf "not scorable (VSYNC-pinned at ~60, both arms): %d ->%s\n", nvs, vsl
  if(bad) printf "⚠ regressions worse than -2%%: %d ->%s\n", bad, badl
  else    print  "No scorable case regressed by more than 2%."
}' "$R"
