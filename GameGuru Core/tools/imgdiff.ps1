# Mean/max absolute per-pixel difference between two screenshots.
# Usage: powershell -File imgdiff.ps1 -A <img1> -B <img2>
param([Parameter(Mandatory=$true)][string]$A, [Parameter(Mandatory=$true)][string]$B)
Add-Type -AssemblyName System.Drawing
$ia = [System.Drawing.Bitmap]::FromFile((Resolve-Path $A))
$ib = [System.Drawing.Bitmap]::FromFile((Resolve-Path $B))
if ($ia.Width -ne $ib.Width -or $ia.Height -ne $ib.Height) {
  Write-Output "SIZE MISMATCH $($ia.Width)x$($ia.Height) vs $($ib.Width)x$($ib.Height)"; exit 1
}
$w = $ia.Width; $h = $ia.Height
$ra = New-Object System.Drawing.Rectangle 0,0,$w,$h
$da = $ia.LockBits($ra, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$db = $ib.LockBits($ra, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$n = $w * $h * 4
$ba = New-Object byte[] $n; $bb = New-Object byte[] $n
[System.Runtime.InteropServices.Marshal]::Copy($da.Scan0, $ba, 0, $n)
[System.Runtime.InteropServices.Marshal]::Copy($db.Scan0, $bb, 0, $n)
$ia.UnlockBits($da); $ib.UnlockBits($db)
$sum = 0.0; $max = 0; $diffpx = 0
for ($i = 0; $i -lt $n; $i += 4) {
  $d = [Math]::Abs($ba[$i] - $bb[$i]) + [Math]::Abs($ba[$i+1] - $bb[$i+1]) + [Math]::Abs($ba[$i+2] - $bb[$i+2])
  if ($d -gt 0) { $diffpx++ }
  if ($d -gt $max) { $max = $d }
  $sum += $d
}
$px = $w * $h
Write-Output ("pixels={0} differing={1} ({2:N3}%) meanAbsDiff={3:N4}/765 maxPixelDiff={4}/765" -f $px, $diffpx, (100.0*$diffpx/$px), ($sum/$px), $max)
$ia.Dispose(); $ib.Dispose()
