# Define maximum lines per file
$maxLines = 2000

# Function to split a single .cpp file while respecting function boundaries
function Split-CppFile {
    param (
        [string]$filePath
    )

    # Read the entire content of the file
    $content = Get-Content -Path $filePath -Raw

    # Define regular expression pattern for function declarations
    # This pattern assumes the opening brace { is on a new line after the function name
    $functionPattern = '(?smi)(\b(?:template\s+<.*?>)?\s*(?:static\s+|inline\s+|virtual\s+|override\s+|explicit\s+|friend\s+)*(?:class\b\s*\w+\s*:\s*public\s|\w+)\s*\w+\s*$[^)]*$\s*\{[^}]*\})'

    # Find all function boundaries
    $functionBoundaries = [regex]::Matches($content, $functionPattern).Groups[0].Index

    # Add the end of file as a boundary
    $functionBoundaries += @($content.Length)

    # Determine the number of chunks needed
    $numChunks = [math]::Ceiling($functionBoundaries.Count / $maxLines)

    for ($i = 0; $i -lt $numChunks; $i++) {
        $startBoundaryIndex = $i * $maxLines
        $endBoundaryIndex = [math]::Min(($i + 1) * $maxLines, $functionBoundaries.Count) - 1

        # Get the start and end boundaries for the current chunk
        $start = $functionBoundaries[$startBoundaryIndex]
        $end = $functionBoundaries[$endBoundaryIndex]

        # Extract the chunk based on the boundaries
        $chunk = $content.Substring($start, $end - $start)

        # Determine the output file name
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($filePath)
        $extension = [System.IO.Path]::GetExtension($filePath)
        $outputFilePath = "$([System.IO.Path]::GetDirectoryName($filePath))\$baseName_$i$extension"

        # Write the chunk to a new file
        Set-Content -Path $outputFilePath -Value $chunk
    }

    Write-Host "File processed: $filePath"
}

# Recursively find all .cpp files in the current directory and subdirectories
Get-ChildItem -Path . -Recurse -Filter *.cpp | ForEach-Object {
    Split-CppFile -filePath $_.FullName
}
