# Grass density metric from the pink-flower benchmark scene.
#
# The 2026-08-01 coverage-scaling failure was invisible to a mean-colour proxy because the grass
# was REDISTRIBUTED into clumps rather than removed. This measures what that proxy could not:
#   coverage - % of viewport pixels that are flower-pink (overall density)
#   clumpCV  - coefficient of variation of coverage across an 8x6 tile grid. Clumped grass keeps
#              a similar total but a much higher CV - exactly the regression that slipped through.
#   bands    - coverage per horizontal band, near (screen bottom) to far (top) = the distance curve,
#              which is what the Grass Draw Distance dial is supposed to shape.
# Usage: powershell -File grassdensity.ps1 -Img <png> [-Label name]
param([Parameter(Mandatory=$true)][string]$Img, [string]$Label = "")

Add-Type -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public class GrassDensity {
  public static string Measure(string path) {
    Bitmap b = (Bitmap)Bitmap.FromFile(path);
    int x0 = 218, x1 = Math.Min(1284, b.Width), y0 = 105, y1 = Math.Min(780, b.Height);
    BitmapData d = b.LockBits(new Rectangle(0,0,b.Width,b.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
    int stride = d.Stride;
    byte[] px = new byte[stride * b.Height];
    Marshal.Copy(d.Scan0, px, 0, px.Length);
    b.UnlockBits(d); b.Dispose();
    const int TX = 8, TY = 6, NB = 6;
    int[,] tile = new int[TX,TY]; int[,] tileTot = new int[TX,TY];
    int[] bandPink = new int[NB]; int[] bandTot = new int[NB];
    long pink = 0, tot = 0;
    int w = x1 - x0, h = y1 - y0;
    for (int y = y0; y < y1; y++) {
      int row = y * stride;
      int ty = (int)Math.Floor((double)(y - y0) * TY / h); if (ty >= TY) ty = TY-1;
      int bi = (int)Math.Floor((double)(y1 - 1 - y) * NB / h); if (bi >= NB) bi = NB-1;
      for (int x = x0; x < x1; x++) {
        int i = row + x * 4;
        int B = px[i], G = px[i+1], R = px[i+2];
        bool isPink = (R > G + 10) && (B > G - 5) && (R > 90);
        tot++;
        int tx = (int)Math.Floor((double)(x - x0) * TX / w); if (tx >= TX) tx = TX-1;
        tileTot[tx,ty]++; bandTot[bi]++;
        if (isPink) { pink++; tile[tx,ty]++; bandPink[bi]++; }
      }
    }
    double cov = 100.0 * pink / tot;
    double sum = 0, n = 0;
    for (int i=0;i<TX;i++) for (int j=0;j<TY;j++) if (tileTot[i,j] > 0) { sum += 100.0*tile[i,j]/tileTot[i,j]; n++; }
    double mean = n > 0 ? sum/n : 0, ss = 0;
    for (int i=0;i<TX;i++) for (int j=0;j<TY;j++) if (tileTot[i,j] > 0) { double v = 100.0*tile[i,j]/tileTot[i,j]; ss += (v-mean)*(v-mean); }
    double sd = n > 1 ? Math.Sqrt(ss/(n-1)) : 0;
    double cv = mean > 0 ? sd/mean : 0;
    string bands = "";
    for (int i=0;i<NB;i++) bands += (100.0*bandPink[i]/Math.Max(1,bandTot[i])).ToString("N2") + " ";
    return string.Format("coverage={0,7:N3}%  clumpCV={1,5:N3}  bands(near..far)= {2}", cov, cv, bands.Trim());
  }
}
'@ -ReferencedAssemblies System.Drawing 2>$null

$r = [GrassDensity]::Measure((Resolve-Path $Img).Path)
"{0,-26} {1}" -f $Label, $r
