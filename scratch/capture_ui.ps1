Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Start the application in the background
$proc = Start-Process -FilePath "./build/bin/orbital.exe" -PassThru -WorkingDirectory (Get-Location)

# Wait for it to open and render
Start-Sleep -Seconds 4

# Get screen bounds
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$graphics = [System.Drawing.Graphics]::FromImage($bmp)
$graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)

# Save to screenshots directory
$dir = Join-Path (Get-Location) "screenshots"
if (-not (Test-Path $dir)) { [void](New-Item -ItemType Directory -Path $dir) }
$bmp.Save((Join-Path $dir "ui_energy_curve.png"), [System.Drawing.Imaging.ImageFormat]::Png)

# Cleanup
$graphics.Dispose()
$bmp.Dispose()
Stop-Process -Id $proc.Id -Force
Write-Host "Screenshot saved successfully to screenshots/ui_after.png"
