# =============================================================================
# SplitCppFiles.ps1
# Splits .cpp files over 2000 lines into smaller _partN files, then removes
# the original. Files under the threshold are left untouched.
# 
# Usage:
#   .\SplitCppFiles.ps1 -DryRun        # Preview only, no changes
#   .\SplitCppFiles.ps1                 # Split and remove originals
#   .\SplitCppFiles.ps1 -MaxLines 1500  # Custom threshold
# =============================================================================

param(
    [int]$MaxLines = 2000,
    [switch]$DryRun
)

$splitCount = 0
$filesProcessed = 0
$filesDeleted = 0

function Find-SafeSplitPoint {
    <#
    .SYNOPSIS
    Finds a safe place to split near the target line.
    Prefers closing braces or blank lines to avoid cutting mid-function.
    #>
    param(
        [string[]]$Lines,
        [int]$TargetLine,
        [int]$SearchWindow = 100
    )

    $searchStart = [math]::Max(0, $TargetLine - $SearchWindow)
    $searchEnd = [math]::Min($Lines.Count - 1, $TargetLine + $SearchWindow)

    # Walk backwards from target looking for a clean break point
    for ($i = $TargetLine; $i -ge $searchStart; $i--) {
        $trimmed = $Lines[$i].Trim()

        # Ideal: closing brace alone on a line (end of function/class/namespace)
        if ($trimmed -eq '}' -or $trimmed -eq '};') {
            return $i + 1
        }

        # Good: blank line
        if ($trimmed -eq '') {
            return $i + 1
        }
    }

    # If nothing found backwards, try forwards
    for ($i = $TargetLine; $i -le $searchEnd; $i++) {
        $trimmed = $Lines[$i].Trim()

        if ($trimmed -eq '}' -or $trimmed -eq '};') {
            return $i + 1
        }

        if ($trimmed -eq '') {
            return $i + 1
        }
    }

    # Fallback: just split at the target line
    return $TargetLine
}

function Split-CppFile {
    param(
        [string]$FilePath
    )

    $lines = Get-Content -Path $FilePath
    $totalLines = $lines.Count

    if ($totalLines -le $MaxLines) {
        return  # File is small enough, skip it
    }

    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($FilePath)
    $extension = [System.IO.Path]::GetExtension($FilePath)
    $directory = [System.IO.Path]::GetDirectoryName($FilePath)

    # Skip files that are already split parts
    if ($baseName -match '_part\d+$') {
        Write-Host "  Skipping (already a split part): $FilePath" -ForegroundColor DarkGray
        return
    }

    Write-Host "`nProcessing: $FilePath" -ForegroundColor Cyan
    Write-Host "  Total lines: $totalLines | Max per part: $MaxLines"

    $partNumber = 0
    $currentStart = 0

    while ($currentStart -lt $totalLines) {
        $targetEnd = $currentStart + $MaxLines

        if ($targetEnd -ge $totalLines) {
            $splitPoint = $totalLines
        } else {
            $splitPoint = Find-SafeSplitPoint -Lines $lines -TargetLine $targetEnd
        }

        $chunkLines = $lines[$currentStart..($splitPoint - 1)]
        $chunkLineCount = $chunkLines.Count

        $outputFileName = "${baseName}_part${partNumber}${extension}"
        $outputPath = Join-Path $directory $outputFileName

        if ($DryRun) {
            Write-Host "  [DRY RUN] Would create: $outputFileName ($chunkLineCount lines)" -ForegroundColor Yellow
        } else {
            $chunkLines | Set-Content -Path $outputPath -Encoding UTF8
            Write-Host "  Created: $outputFileName ($chunkLineCount lines)" -ForegroundColor Green
        }

        $partNumber++
        $currentStart = $splitPoint
    }

    # Remove the original file now that parts have been written
    if ($DryRun) {
        Write-Host "  [DRY RUN] Would delete original: $FilePath" -ForegroundColor Red
    } else {
        Remove-Item -Path $FilePath -Force
        Write-Host "  Deleted original: $FilePath" -ForegroundColor Red
        $script:filesDeleted++
    }

    $script:splitCount += $partNumber
    $script:filesProcessed++

    Write-Host "  Split into $partNumber parts." -ForegroundColor Cyan
}

# =============================================================================
# Main
# =============================================================================

Write-Host "=============================================" -ForegroundColor White
Write-Host " C++ File Splitter" -ForegroundColor White
Write-Host " Max lines per part: $MaxLines" -ForegroundColor White
if ($DryRun) {
    Write-Host " MODE: DRY RUN (no changes will be made)" -ForegroundColor Yellow
} else {
    Write-Host " MODE: LIVE (originals WILL be deleted)" -ForegroundColor Red
}
Write-Host "=============================================" -ForegroundColor White

# Safety prompt for live runs
if (-not $DryRun) {
    Write-Host ""
    Write-Host " WARNING: This will split large .cpp files and DELETE the originals." -ForegroundColor Red
    Write-Host " Make sure you have a git commit or backup before proceeding." -ForegroundColor Red
    Write-Host ""
    $confirm = Read-Host "Type YES to continue"
    if ($confirm -ne 'YES') {
        Write-Host "Aborted." -ForegroundColor Yellow
        exit
    }
}

# Find all .cpp files recursively
$cppFiles = Get-ChildItem -Path . -Recurse -Filter *.cpp

$totalFiles = $cppFiles.Count
Write-Host "`nFound $totalFiles .cpp file(s) to scan.`n"

foreach ($file in $cppFiles) {
    Split-CppFile -FilePath $file.FullName
}

Write-Host "`n=============================================" -ForegroundColor White
Write-Host " Done!" -ForegroundColor Green
Write-Host " Files scanned:     $totalFiles" -ForegroundColor White
Write-Host " Files split:       $filesProcessed" -ForegroundColor White
Write-Host " Originals deleted: $filesDeleted" -ForegroundColor White
Write-Host " Parts created:     $splitCount" -ForegroundColor White
Write-Host "=============================================" -ForegroundColor White