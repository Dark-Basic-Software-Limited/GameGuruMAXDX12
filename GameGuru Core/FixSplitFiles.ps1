# FixSplitFiles.ps1
# Two tasks:
# 1. Create wrapper .cpp files for split file groups where original was deleted
# 2. Copy include headers from _part0.cpp to _part1+.cpp files

param(
    [string]$RootDir = "D:\max\GameGuruMAXDX12\GameGuru Core"
)

$ErrorActionPreference = "Continue"

# Directories to skip (third-party code we don't modify)
$skipPatterns = @(
    '\\SDK\\BULLET\\bullet-3.19\\',
    '\\SDK\\OGG\\',
    '\\SDK\\OPTICK\\',
    '\\SDK\\DirectXTex\\',
    '\\SDK\\THEORA\\',
    '\\SDK\\ASSIMP\\'
)

function Should-Skip($path) {
    foreach ($pattern in $skipPatterns) {
        if ($path -match [regex]::Escape($pattern).Replace('\\\\','\\')) {
            return $true
        }
    }
    return $false
}

function Get-HeaderBlock($filePath) {
    # Read _part0.cpp and extract the header block (includes, defines, comments, etc.)
    # Stop at the first line that looks like actual code
    $lines = Get-Content -Path $filePath -Encoding UTF8
    $headerLines = @()
    $inMultiLineComment = $false

    foreach ($line in $lines) {
        $trimmed = $line.TrimStart()

        # Handle multi-line comments
        if ($inMultiLineComment) {
            $headerLines += $line
            if ($trimmed -match '\*/') {
                $inMultiLineComment = $false
            }
            continue
        }

        # Check if line starts a multi-line comment
        if ($trimmed -match '^\s*/\*') {
            $headerLines += $line
            if ($trimmed -notmatch '\*/') {
                $inMultiLineComment = $true
            }
            continue
        }

        # Header patterns
        if ($trimmed -eq '' -or                          # Empty line
            $trimmed -match '^//' -or                    # Single-line comment
            $trimmed -match '^\*' -or                    # Inside comment block
            $trimmed -match '^#' -or                     # Preprocessor directive
            $trimmed -match '^extern\s+"C"' -or          # extern "C" {
            $trimmed -eq '}' -or                         # closing brace (extern C)
            $trimmed -match '^using\s+' -or              # using directive
            $trimmed -match '^namespace\s+' -or          # namespace
            $trimmed -eq [char]0xFEFF -or                # BOM
            $trimmed -match '^\xEF\xBB\xBF') {          # BOM bytes
            $headerLines += $line
        } else {
            # Not a header line - stop collecting
            break
        }
    }

    return $headerLines
}

Write-Host "=== Phase 1: Creating wrapper files ==="
$wrappersCreated = 0

# Find all _part0.cpp files
$part0Files = Get-ChildItem -Path $RootDir -Recurse -Filter "*_part0.cpp"

foreach ($part0 in $part0Files) {
    if (Should-Skip $part0.FullName) { continue }

    $dir = $part0.DirectoryName
    $baseName = $part0.Name -replace '_part0\.cpp$', ''
    $originalFile = Join-Path $dir "$baseName.cpp"

    # Check if original file exists
    if (Test-Path $originalFile) {
        # Original exists - no wrapper needed
        continue
    }

    # Find all part files
    $partFiles = Get-ChildItem -Path $dir -Filter "${baseName}_part*.cpp" | Where-Object {
        $_.Name -match "^${baseName}_part(\d+)\.cpp$"
    } | Sort-Object { [int]($_.Name -replace ".*_part(\d+)\.cpp$", '$1') }

    if ($partFiles.Count -eq 0) { continue }

    # Check if there's only _part0 (GPUParticles case) - just rename concept
    # Actually, still create wrapper for consistency

    # Build wrapper content
    $wrapperContent = "// Auto-generated wrapper - includes split parts`r`n"
    foreach ($pf in $partFiles) {
        $wrapperContent += "#include `"$($pf.Name)`"`r`n"
    }

    # Write wrapper file
    Set-Content -Path $originalFile -Value $wrapperContent -Encoding UTF8 -NoNewline
    $wrappersCreated++
    Write-Host "WRAPPER: $originalFile ($($partFiles.Count) parts)"
}

Write-Host ""
Write-Host "=== Phase 2: Adding include headers to _part1+ files ==="
$filesFixed = 0
$filesSkipped = 0

foreach ($part0 in $part0Files) {
    if (Should-Skip $part0.FullName) { continue }

    $dir = $part0.DirectoryName
    $baseName = $part0.Name -replace '_part0\.cpp$', ''

    # Get header block from _part0
    $headerBlock = Get-HeaderBlock $part0.FullName

    if ($headerBlock.Count -eq 0) {
        Write-Host "SKIP (no header): $($part0.FullName)"
        continue
    }

    # Find _part1+ files
    $partFiles = Get-ChildItem -Path $dir -Filter "${baseName}_part*.cpp" | Where-Object {
        $_.Name -match "^${baseName}_part(\d+)\.cpp$" -and [int]$Matches[1] -gt 0
    } | Sort-Object { [int]($_.Name -replace ".*_part(\d+)\.cpp$", '$1') }

    foreach ($partFile in $partFiles) {
        # Read existing content
        $existingLines = Get-Content -Path $partFile.FullName -Encoding UTF8

        # Check if it already has #include directives near the top (first 5 non-empty lines)
        $firstContentLines = $existingLines | Where-Object { $_.Trim() -ne '' } | Select-Object -First 5
        $hasIncludes = $false
        foreach ($cl in $firstContentLines) {
            if ($cl.Trim() -match '^#include') {
                $hasIncludes = $true
                break
            }
        }

        if ($hasIncludes) {
            $filesSkipped++
            Write-Host "SKIP (has includes): $($partFile.FullName)"
            continue
        }

        # Prepend header block
        $newContent = $headerBlock + @("") + $existingLines
        Set-Content -Path $partFile.FullName -Value $newContent -Encoding UTF8
        $filesFixed++
        Write-Host "FIXED: $($partFile.FullName) (+$($headerBlock.Count) lines)"
    }
}

Write-Host ""
Write-Host "=== Summary ==="
Write-Host "Wrappers created: $wrappersCreated"
Write-Host "Part files fixed (includes added): $filesFixed"
Write-Host "Part files skipped (already had includes): $filesSkipped"
