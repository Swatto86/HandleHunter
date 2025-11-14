# create_icon.ps1 - Generate HandleHunter application icon
# This script creates a simple but professional icon for the application

Add-Type -AssemblyName System.Drawing

# Create a 256x256 bitmap (will be scaled down for ICO)
$size = 256
$bitmap = New-Object System.Drawing.Bitmap($size, $size)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAlias

# Fill background with gradient
$gradientBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    (New-Object System.Drawing.Point(0, 0)),
    (New-Object System.Drawing.Point($size, $size)),
    [System.Drawing.Color]::FromArgb(255, 41, 128, 185),  # Blue
    [System.Drawing.Color]::FromArgb(255, 22, 78, 137)    # Darker blue
)
$graphics.FillRectangle($gradientBrush, 0, 0, $size, $size)

# Draw a lock icon representation
$centerX = $size / 2
$centerY = $size / 2 + 10

# Lock body (rectangle)
$lockBodyWidth = 120
$lockBodyHeight = 100
$lockBodyX = $centerX - ($lockBodyWidth / 2)
$lockBodyY = $centerY - 10
$lockBodyRect = New-Object System.Drawing.Rectangle($lockBodyX, $lockBodyY, $lockBodyWidth, $lockBodyHeight)

# Draw lock body with rounded corners
$graphics.FillRectangle([System.Drawing.Brushes]::White, $lockBodyRect)

# Lock shackle (arc on top)
$shackleWidth = 80
$shackleHeight = 70
$shackleX = $centerX - ($shackleWidth / 2)
$shackleY = $lockBodyY - 50
$shacklePen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 18)
$graphics.DrawArc($shacklePen, $shackleX, $shackleY, $shackleWidth, $shackleHeight, 180, 180)

# Draw keyhole
$keyholeSize = 25
$keyholeX = $centerX - ($keyholeSize / 2)
$keyholeY = $centerY + 10
$keyholeBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 41, 128, 185))
$graphics.FillEllipse($keyholeBrush, $keyholeX, $keyholeY, $keyholeSize, $keyholeSize)
$keyholeRectHeight = 25
$keyholeRectWidth = 12
$keyholeRectX = $centerX - ($keyholeRectWidth / 2)
$keyholeRectY = $keyholeY + $keyholeSize - 5
$graphics.FillRectangle($keyholeBrush, $keyholeRectX, $keyholeRectY, $keyholeRectWidth, $keyholeRectHeight)
$keyholeBrush.Dispose()

# Save as PNG first (for verification)
$bitmap.Save("$PSScriptRoot\icon.png", [System.Drawing.Imaging.ImageFormat]::Png)

# Create ICO file with multiple sizes
$iconSizes = @(16, 32, 48, 64, 128, 256)
$iconImages = @()

foreach ($iconSize in $iconSizes) {
    $scaledBitmap = New-Object System.Drawing.Bitmap($iconSize, $iconSize)
    $scaledGraphics = [System.Drawing.Graphics]::FromImage($scaledBitmap)
    $scaledGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $scaledGraphics.DrawImage($bitmap, 0, 0, $iconSize, $iconSize)
    $iconImages += $scaledBitmap
    $scaledGraphics.Dispose()
}

# Save as ICO using .NET
$icoPath = "$PSScriptRoot\icon.ico"
$iconStream = [System.IO.File]::Create($icoPath)

# Write ICO header
$iconWriter = New-Object System.IO.BinaryWriter($iconStream)
$iconWriter.Write([UInt16]0)  # Reserved
$iconWriter.Write([UInt16]1)  # Type (1 = ICO)
$iconWriter.Write([UInt16]$iconImages.Count)  # Number of images

$imageOffset = 6 + (16 * $iconImages.Count)

# Write directory entries
foreach ($img in $iconImages) {
    $iconWriter.Write([Byte]($img.Width -band 0xFF))
    $iconWriter.Write([Byte]($img.Height -band 0xFF))
    $iconWriter.Write([Byte]0)  # Color palette
    $iconWriter.Write([Byte]0)  # Reserved
    $iconWriter.Write([UInt16]1)  # Color planes
    $iconWriter.Write([UInt16]32)  # Bits per pixel
    
    $memStream = New-Object System.IO.MemoryStream
    $img.Save($memStream, [System.Drawing.Imaging.ImageFormat]::Png)
    $imageData = $memStream.ToArray()
    $memStream.Dispose()
    
    $iconWriter.Write([UInt32]$imageData.Length)  # Image size
    $iconWriter.Write([UInt32]$imageOffset)  # Image offset
    
    $imageOffset += $imageData.Length
}

# Write image data
foreach ($img in $iconImages) {
    $memStream = New-Object System.IO.MemoryStream
    $img.Save($memStream, [System.Drawing.Imaging.ImageFormat]::Png)
    $imageData = $memStream.ToArray()
    $iconWriter.Write($imageData)
    $memStream.Dispose()
    $img.Dispose()
}

$iconWriter.Close()
$iconStream.Close()

# Cleanup
$graphics.Dispose()
$bitmap.Dispose()
$gradientBrush.Dispose()
$shacklePen.Dispose()

Write-Host "Icon created successfully!" -ForegroundColor Green
Write-Host "  - icon.png (preview)" -ForegroundColor Cyan
Write-Host "  - icon.ico (application icon)" -ForegroundColor Cyan

