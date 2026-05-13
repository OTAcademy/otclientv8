param(
    [ValidateSet("OpenGL", "DirectX", "Both")]
    [string]$Configuration = "OpenGL",

    [ValidateSet("x64", "Win32")]
    [string]$Platform = "x64",

    [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg",
    [string]$OutputDir = ".\dist\realots-client-windows"
)

$ErrorActionPreference = "Stop"

function Require-Command($Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Missing command: $Name"
    }
}

function Find-MSBuild {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
        if ($path -and (Test-Path $path)) {
            return $path
        }
    }

    $cmd = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    throw "MSBuild.exe not found. Install Visual Studio 2022 or Build Tools 2022 with Desktop development with C++."
}

function Ensure-Vcpkg {
    if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
        Write-Host "Cloning vcpkg into $VcpkgRoot"
        git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
        & "$VcpkgRoot\bootstrap-vcpkg.bat"
    }

    & "$VcpkgRoot\vcpkg.exe" integrate install
}

Require-Command git
$msbuild = Find-MSBuild
Ensure-Vcpkg

$configs = if ($Configuration -eq "Both") { @("OpenGL", "DirectX") } else { @($Configuration) }

foreach ($cfg in $configs) {
    Write-Host "Building $cfg|$Platform"
    & $msbuild ".\vc17\otclient.sln" `
        /m `
        /restore `
        /p:Configuration=$cfg `
        /p:Platform=$Platform `
        /p:VcpkgRoot="$VcpkgRoot\" `
        /p:BUILD_REVISION=1
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Copy-Item ".\init.lua" $OutputDir -Force
Copy-Item ".\data" "$OutputDir\data" -Recurse -Force
Copy-Item ".\modules" "$OutputDir\modules" -Recurse -Force
Copy-Item ".\mods" "$OutputDir\mods" -Recurse -Force
Copy-Item ".\layouts" "$OutputDir\layouts" -Recurse -Force
Copy-Item ".\REALOTS.md" "$OutputDir\REALOTS.md" -Force

Get-ChildItem -Path "." -Filter "otclient*.exe" | Copy-Item -Destination $OutputDir -Force

Write-Host ""
Write-Host "Done. Windows client package:"
Write-Host (Resolve-Path $OutputDir)
Write-Host ""
Write-Host "Add Tibia 7.7 assets before login:"
Write-Host "$OutputDir\data\things\770\Tibia.dat"
Write-Host "$OutputDir\data\things\770\Tibia.spr"
