# =============================================================================
# SplitCppFiles.ps1
# Splits .cpp files over 2000 lines into smaller _partN files, then removes
# the original. Files under the threshold are left untouched.
#
# Uses brace-depth tracking and splits at the blank line BEFORE the next
# function declaration, so function signatures always stay with their body.
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

function Get-BraceDepthMap {
    <#
    .SYNOPSIS
    Returns an array where each index holds the brace depth AFTER that line.
    Depth 0 = between top-level blocks (functions, classes, namespaces).
    Skips braces inside comments and string/char literals.
    #>
    param(
        [string[]]$Lines
    )

    $depthMap = New-Object int[] $Lines.Count
    $depth = 0
    $inBlockComment = $false

    for ($i = 0; $i -lt $Lines.Count; $i++) {
        $line = $Lines[$i]

        for ($c = 0; $c -lt $line.Length; $c++) {
            $char = $line[$c]
            $nextChar = if ($c + 1 -lt $line.Length) { $line[$c + 1] } else { '' }

            # Handle block comments
            if ($inBlockComment) {
                if ($char -eq '*' -and $nextChar -eq '/') {
                    $inBlockComment = $false
                    $c++
                }
                continue
            }

            # Start of block comment
            if ($char -eq '/' -and $nextChar -eq '*') {
                $inBlockComment = $true
                $c++
                continue
            }

            # Line comment - skip rest of line
            if ($char -eq '/' -and $nextChar -eq '/') {
                break
            }

            # Skip string literals
            if ($char -eq '"') {
                $c++
                while ($c -lt $line.Length) {
                    if ($line[$c] -eq '\') { $c++ }
                    elseif ($line[$c] -eq '"') { break }
                    $c++
                }
                continue
            }

            # Skip char literals
            if ($char -eq "'") {
                $c++
                while ($c -lt $line.Length) {
                    if ($line[$c] -eq '\') { $c++ }
                    elseif ($line[$c] -eq "'") { break }
                    $c++
                }
                continue
            }

            # Count braces
            if ($char -eq '{') { $depth++ }
            elseif ($char -eq '}') { $depth = [math]::Max(0, $depth - 1) }
        }

        $depthMap[$i] = $depth
    }

    return $depthMap
}

function Find-FunctionBoundaries {
    <#
    .SYNOPSIS
    Returns a sorted list of line indices where it is safe to split.
    A safe split point is the first non-blank line after a depth-0 closing
    brace — i.e., the start of the next top-level declaration.
    This ensures function signatures stay with their bodies.
    #>
    param(
        [string[]]$Lines,
        [int[]]$DepthMap
    )

    $boundaries = @()

    for ($i = 0; $i -lt $Lines.Count - 1; $i++) {
        # Look for lines where depth drops to 0 (end of a top-level block)
        if ($DepthMap[$i] -eq 0) {
            $trimmed = $Lines[$i].Trim()
            # Must be a closing brace line (end of function/class/namespace)
            if ($trimmed -eq '}' -or $trimmed -eq '};' -or $trimmed -match '^\}') {
                # Now find the start of the next non-blank content
                $nextContentLine = $i + 1
                while ($nextContentLine -lt $Lines.Count -and $Lines[$nextContentLine].Trim() -eq '') {
                    $nextContentLine++
                }
                if ($nextContentLine -lt $Lines.Count) {
                    $boundaries += $nextContentLine
                }
            }
        }
    }

    return $boundaries | Sort-Object -Unique
}

function Find-SafeSplitPoint {
    <#
    .SYNOPSIS
    From the precomputed function boundaries, find the one closest to
    (but not exceeding) the target line.
    #>
    param(
        [int[]]$Boundaries,
        [int]$TargetLine,
        [int]$CurrentStart
    )

    $best = -1

    # Find the boundary closest to the target without going below currentStart
    foreach ($b in $Boundaries) {
        if ($b -le $CurrentStart) { continue }
        if ($b -le $TargetLine) {
            $best = $b
        } else {
            # We've passed the target. If we never found one before target,
            # use this one (the first boundary after target) to avoid
            # creating a chunk that's way too large
            if ($best -eq -1) {
                $best = $b
            }
            break
        }
    }

    return $best
}

function Split-CppFile {
    param(
        [string]$FilePath
    )

    $lines = Get-Content -Path $FilePath
    $totalLines = $lines.Count

    if ($totalLines -le $MaxLines) {
        return
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
    Write-Host "  Analyzing function boundaries..." -ForegroundColor DarkGray

    # Build depth map and find all function boundaries
    $depthMap = Get-BraceDepthMap -Lines $lines
    $boundaries = Find-FunctionBoundaries -Lines $lines -DepthMap $depthMap

    Write-Host "  Found $($boundaries.Count) function boundaries." -ForegroundColor DarkGray

    $partNumber = 0
    $currentStart = 0

    while ($currentStart -lt $totalLines) {
        $targetEnd = $currentStart + $MaxLines

        if ($targetEnd -ge $totalLines) {
            # Last chunk - take everything remaining
            $splitPoint = $totalLines
        } else {
            $splitPoint = Find-SafeSplitPoint -Boundaries $boundaries -TargetLine $targetEnd -CurrentStart $currentStart

            # Fallback if no boundary found
            if ($splitPoint -eq -1) {
                $splitPoint = $totalLines
            }
        }

        # Safety: avoid infinite loop
        if ($splitPoint -le $currentStart) {
            $splitPoint = [math]::Min($currentStart + $MaxLines, $totalLines)
        }

        $chunkLines = $lines[$currentStart..($splitPoint - 1)]
        $chunkLineCount = $chunkLines.Count

        $outputFileName = "${baseName}_part${partNumber}${extension}"
        $outputPath = Join-Path $directory $outputFileName

        if ($DryRun) {
            # Show the first non-blank line of the chunk for verification
            $preview = ""
            foreach ($cl in $chunkLines) {
                if ($cl.Trim() -ne '') {
                    $preview = $cl.Trim()
                    if ($preview.Length -gt 70) { $preview = $preview.Substring(0, 70) + "..." }
                    break
                }
            }
            Write-Host "  [DRY RUN] Would create: $outputFileName ($chunkLineCount lines) starts: $preview" -ForegroundColor Yellow
        } else {
            $chunkLines | Set-Content -Path $outputPath -Encoding UTF8
            Write-Host "  Created: $outputFileName ($chunkLineCount lines)" -ForegroundColor Green
        }

        $partNumber++
        $currentStart = $splitPoint
    }

    # Remove the original file
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
Write-Host " C++ File Splitter (Function-Boundary Aware)" -ForegroundColor White
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