$url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.12/SDL2-devel-2.30.12-mingw.zip"
$output = "$env:TEMP\SDL2-mingw.zip"
Write-Host "Downloading SDL2 for MinGW from $url ..."
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $url -OutFile $output -UseBasicParsing
if (Test-Path $output) {
    Write-Host "Download complete: $((Get-Item $output).Length) bytes"
    Write-Host "Extracting..."
    $extractDir = "$env:USERPROFILE\SDL2-mingw"
    Expand-Archive -Path $output -DestinationPath $extractDir -Force
    Write-Host "Extracted to $extractDir"
    Remove-Item $output
    Write-Host "SDL2 ready!"
} else {
    Write-Host "Download failed"
}
