param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [int]$Frames = 8,
    [string]$Scene,
    [int]$Demo = 0,
    [switch]$All
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$executable = Join-Path $root "Build\x64\$Configuration\SandboxApp\SandboxApp.exe"
$outputDirectory = Join-Path $root "Build\Validation\$Configuration"
if (!(Test-Path -LiteralPath $executable)) {
    throw "Build $Configuration|x64 before running example checks."
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

# 개발 중인 장면. 기본 실행은 이 목록만 검사한다. 장면 작업을 시작하거나 끝낼 때 갱신한다.
$devScenes = @('MousePickingScene')

$scenes = @('SimpleScene', 'InstancingScene', 'MultiVertexStreamScene', 'TextureMappingScene', 'PBRScene', 'DeferredScene', 'DebugNormalScene', 'ShadowMappingScene', 'StencilScene', 'CullingScene', 'TerrainScene', 'SSAOScene', 'SSRScene', 'ParticleScene', 'MousePickingScene', 'ComputeScene', 'SkinningScene')

# 검사 대상: -Scene 지정 > -All(전체) > 개발 중인 장면
$targets = if ($Scene) { @($Scene) } elseif ($All) { $scenes } else { $devScenes }

$cases = @()
foreach ($name in $scenes) {
    if ($targets -contains $name) {
        if ($name -in @('ComputeScene', 'ParticleScene')) {
            $lastDemo = if ($name -eq 'ComputeScene') { 17 } else { 2 }
            $demos = if ($Demo -gt 0) { @($Demo) } else { 1..$lastDemo }
            foreach ($index in $demos) {
                $cases += @{ Name = "$name-Demo$index"; Arguments = "--scene $name --demo $index" }
            }
        }
        elseif ($name -eq 'MousePickingScene' -and $Demo -eq 0) {
            $cases += @{ Name = $name; Arguments = "--scene $name"; ExpectedPick = -1 }
            $cases += @{ Name = "$name-Selected"; Arguments = "--scene $name --pick-x 800 --pick-y 500"; ExpectedPick = 5 }
            $cases += @{ Name = "$name-Background"; Arguments = "--scene $name --pick-x 20 --pick-y 100"; ExpectedPick = 0 }
            $cases += @{ Name = "$name-IDs"; Arguments = "--scene $name --show-ids"; ExpectedPick = -1 }
        }
        elseif ($Demo -eq 0) {
            $cases += @{ Name = $name; Arguments = "--scene $name" }
        }
    }
}
if ($cases.Count -eq 0) {
    throw "Unknown scene: $Scene"
}

$failures = @()
foreach ($case in $cases) {
    $logPath = Join-Path $outputDirectory ($case.Name + '.log')
    $imagePath = Join-Path $outputDirectory ($case.Name + '.png')
    if (Test-Path -LiteralPath $imagePath) {
        Remove-Item -LiteralPath $imagePath
    }
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $executable
    $info.WorkingDirectory = Join-Path $root 'SandboxApp'
    $info.Arguments = $case.Arguments + " --frames $Frames --screenshot `"$imagePath`""
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.WindowStyle = 'Hidden'
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.EnvironmentVariables['JAM_NO_ERROR_DIALOG'] = '1'

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    $null = $process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $completed = $process.WaitForExit(45000)
    if (!$completed) {
        $process.Kill()
    }
    $process.WaitForExit()
    $output = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText($logPath, $output)

    $pickValid = $true
    if ($case.ContainsKey('ExpectedPick')) {
        $picks = [regex]::Matches($output, 'Picked object ID: (\d+)')
        $pickValid = if ($case.ExpectedPick -lt 0) { $picks.Count -eq 0 } else {
            $picks.Count -eq 1 -and [int]$picks[0].Groups[1].Value -eq $case.ExpectedPick
        }
    }
    if ($completed -and $process.ExitCode -eq 0 -and (Test-Path -LiteralPath $imagePath) -and $output -notmatch '\[(critical|error)\]' -and $pickValid) {
        Write-Output "PASS $($case.Name)"
    }
    else {
        Write-Output "FAIL $($case.Name) (exit $($process.ExitCode), completed $completed): $logPath"
        $failures += $case.Name
    }
    $process.Dispose()
}

Write-Output "$($cases.Count - $failures.Count)/$($cases.Count) examples passed ($Configuration)."
if ($failures.Count -gt 0) {
    exit 1
}
