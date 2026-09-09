# SPDX-License-Identifier: GPL-3.0-or-later
param([switch]$Prepare, [switch]$Offline, [string]$Python = "python", [string]$CMake = "", [string]$Ninja = "")
$ErrorActionPreference = 'Stop'
$tc002Arguments = @((Join-Path $PSScriptRoot 'build-tc002.py'))
if ($Prepare) { $tc002Arguments += '--prepare' }
if ($Offline) { $tc002Arguments += '--offline' }
if ($CMake) { $tc002Arguments += @('--cmake', $CMake) }
if ($Ninja) { $tc002Arguments += @('--ninja', $Ninja) }
& $Python @tc002Arguments
if ($LASTEXITCODE -ne 0) { throw "TC002 build failed ($LASTEXITCODE)" }
