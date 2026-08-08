# Per-screenshot mean luma + blown-pixel fraction, for two sweep shot dirs.
# Animation makes a raw pixel diff useless between runs; mean luma and the
# fraction of near-white pixels are stable frame to frame and are exactly what a
# white-out / lighting regression moves. Usage: shotstats.ps1 -Old shots0807 -New shots0808
param(
  [string]$Old = "shots0807",
  [string]$New = "shots0808",
  [string]$Root = "C:\Users\leeba\AppData\Local\Temp\claude\D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892\9a28c586-4c13-4447-916e-7fb51301bfa8\scratchpad\demo_fps"
)
Add-Type -AssemblyName System.Drawing
$root = $Root

function Stats([string]$path) {
  if (-not (Test-Path $path)) { return $null }
  $img = [System.Drawing.Bitmap]::FromFile((Resolve-Path $path))
  $w = $img.Width; $h = $img.Height
  $r = New-Object System.Drawing.Rectangle 0,0,$w,$h
  $d = $img.LockBits($r, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $n = $w * $h * 4
  $b = New-Object byte[] $n
  [System.Runtime.InteropServices.Marshal]::Copy($d.Scan0, $b, 0, $n)
  $img.UnlockBits($d); $img.Dispose()
  $sum = 0.0; $blown = 0; $dark = 0; $px = 0
  for ($i = 0; $i -lt $n; $i += 32) {   # stride-sample every 8th pixel: plenty for a mean
    $lum = 0.114*$b[$i] + 0.587*$b[$i+1] + 0.299*$b[$i+2]
    $sum += $lum; $px++
    if ($lum -gt 240) { $blown++ }
    if ($lum -lt 12)  { $dark++ }
  }
  return @{ luma = $sum/$px; blown = 100.0*$blown/$px; dark = 100.0*$dark/$px; w = $w; h = $h }
}

"{0,-34} {1,-24} {2,-24} {3}" -f "SHOT", "luma old>new", "blown% old>new", "FLAG"
"-" * 100
$names = Get-ChildItem "$root\$New" -Filter *.png | Sort-Object Name
foreach ($f in $names) {
  $a = Stats "$root\$Old\$($f.Name)"
  $bb = Stats "$root\$New\$($f.Name)"
  if ($null -eq $bb) { continue }
  if ($null -eq $a) {
    "{0,-34} {1,-24} {2,-24} {3}" -f $f.BaseName, ("--> {0:N1}" -f $bb.luma), ("--> {0:N2}" -f $bb.blown), "NO BASELINE"
    continue
  }
  $dl = $bb.luma - $a.luma
  $db = $bb.blown - $a.blown
  $flag = "ok"
  if ([Math]::Abs($dl) -gt 18) { $flag = "LUMA SHIFT" }
  if ($db -gt 8) { $flag = "BLOWN-OUT (white-out suspect)" }
  if ($bb.luma -lt 8) { $flag = "BLACK FRAME" }
  "{0,-34} {1,-24} {2,-24} {3}" -f $f.BaseName, ("{0:N1} > {1:N1} ({2:+0.0;-0.0})" -f $a.luma, $bb.luma, $dl), ("{0:N2} > {1:N2} ({2:+0.00;-0.00})" -f $a.blown, $bb.blown, $db), $flag
}
