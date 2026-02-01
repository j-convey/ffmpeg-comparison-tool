# PowerShell script to deploy all required DLLs for VidMetric

$BinDir = "build\bin"
$QtBinDir = "C:\msys64\mingw64\bin"

Write-Host "Copying required DLLs to $BinDir..." -ForegroundColor Green

# List of DLLs to copy
$DllsToCopy = @(
    # ICU libraries
    "libicuin*.dll",
    "libicuuc*.dll", 
    "libicudt*.dll",
    # Qt dependencies
    "libdouble-conversion.dll",
    "libb2-*.dll",
    "libpcre2-16-0.dll",
    "libzstd.dll",
    "libharfbuzz-0.dll",
    "libpng16-16.dll",
    "libfreetype-6.dll",
    "libbz2-1.dll",
    "libbrotlidec.dll",
    "libbrotlicommon.dll",
    "libgraphite2.dll",
    "libmd4c.dll",
    "zlib1.dll"
)

$CopiedCount = 0
$NotFoundCount = 0

foreach ($Pattern in $DllsToCopy) {
    $Files = Get-ChildItem -Path $QtBinDir -Filter $Pattern -ErrorAction SilentlyContinue
    
    if ($Files) {
        foreach ($File in $Files) {
            $DestPath = Join-Path $BinDir $File.Name
            if (-not (Test-Path $DestPath)) {
                Copy-Item $File.FullName -Destination $BinDir -Force
                Write-Host "  Copied: $($File.Name)" -ForegroundColor Cyan
                $CopiedCount++
            } else {
                Write-Host "  Already exists: $($File.Name)" -ForegroundColor Gray
            }
        }
    } else {
        Write-Host "  Not found: $Pattern" -ForegroundColor Yellow
        $NotFoundCount++
    }
}

Write-Host "`nDeployment complete!" -ForegroundColor Green
Write-Host "Copied: $CopiedCount DLLs" -ForegroundColor Green
if ($NotFoundCount -gt 0) {
    Write-Host "Not found: $NotFoundCount patterns" -ForegroundColor Yellow
}
Write-Host "`nYou can now run: .\build\bin\VidMetric.exe" -ForegroundColor Cyan
