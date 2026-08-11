# Mean/max absolute per-pixel difference between two screenshots, restricted to a RECTANGLE.
#
# Why this exists: a whole-frame diff on a grass-heavy level is dominated by wind animation, whose
# noise floor (measured on Aztec Game Kit Teaser: mean 11.03/765) swamps the thing under test. Far
# terrain removed by a smaller wi::terrain ring shows up in the UPPER viewport — sky line and
# distant hills — where there is no animated grass. Diffing only that band raises the signal-to-
# noise by roughly two orders of magnitude.
#
# Usage: powershell -File regiondiff.ps1 -A <img1> -B <img2> -X0 250 -Y0 105 -X1 1283 -Y1 310
param(
  [Parameter(Mandatory=$true)][string]$A,
  [Parameter(Mandatory=$true)][string]$B,
  [Parameter(Mandatory=$true)][int]$X0,
  [Parameter(Mandatory=$true)][int]$Y0,
  [Parameter(Mandatory=$true)][int]$X1,
  [Parameter(Mandatory=$true)][int]$Y1
)
Add-Type -AssemblyName System.Drawing
$ia = [System.Drawing.Bitmap]::FromFile((Resolve-Path $A))
$ib = [System.Drawing.Bitmap]::FromFile((Resolve-Path $B))
if ($ia.Width -ne $ib.Width -or $ia.Height -ne $ib.Height) {
  Write-Output "SIZE MISMATCH $($ia.Width)x$($ia.Height) vs $($ib.Width)x$($ib.Height)"; exit 1
}
if ($X1 -gt $ia.Width)  { $X1 = $ia.Width }
if ($Y1 -gt $ia.Height) { $Y1 = $ia.Height }
$sum = 0.0; $max = 0; $diffpx = 0; $n = 0
for ($y = $Y0; $y -lt $Y1; $y++) {
  for ($x = $X0; $x -lt $X1; $x++) {
    $pa = $ia.GetPixel($x, $y); $pb = $ib.GetPixel($x, $y)
    $d = [Math]::Abs($pa.R - $pb.R) + [Math]::Abs($pa.G - $pb.G) + [Math]::Abs($pa.B - $pb.B)
    $sum += $d; $n++
    if ($d -gt 0) { $diffpx++ }
    if ($d -gt $max) { $max = $d }
  }
}
$ia.Dispose(); $ib.Dispose()
$pct = if ($n) { 100.0 * $diffpx / $n } else { 0 }
$mean = if ($n) { $sum / $n } else { 0 }
Write-Output ("region={0},{1}-{2},{3} pixels={4} differing={5} ({6:N3}%) meanAbsDiff={7:N4}/765 maxPixelDiff={8}/765" -f $X0,$Y0,$X1,$Y1,$n,$diffpx,$pct,$mean,$max)
