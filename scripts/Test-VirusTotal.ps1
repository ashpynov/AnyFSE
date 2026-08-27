[CmdletBinding()]
param(
    [string] $FilePath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Net.Http

$apiUrl = 'https://www.virustotal.com/api/v3'
$apiKey = $env:VIRUSTOTAL_API_KEY
$pollIntervalSeconds = 30
$timeoutMinutes = 20
$repositoryRoot = Split-Path -Parent $PSScriptRoot

function Read-VirusTotalResponse {
    param([System.Net.Http.HttpResponseMessage] $Response)

    $body = $Response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
    if (-not $Response.IsSuccessStatusCode) {
        throw "VirusTotal returned HTTP $([int]$Response.StatusCode): $body"
    }
    return $body | ConvertFrom-Json
}

if ([string]::IsNullOrWhiteSpace($apiKey)) {
    throw 'Set the VIRUSTOTAL_API_KEY environment variable first.'
}

if ([string]::IsNullOrWhiteSpace($FilePath)) {
    $installer = Get-ChildItem (Join-Path $repositoryRoot 'build\Release') -Filter 'AnyFSE.Installer.Offline.*.exe' -File |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
}
else {
    if (-not [System.IO.Path]::IsPathRooted($FilePath)) {
        $FilePath = Join-Path $repositoryRoot $FilePath
    }
    $installer = Get-Item -LiteralPath $FilePath
}

if ($null -eq $installer) {
    throw "Offline installer was not found in 'build\Release'."
}

$sha256 = (Get-FileHash $installer.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Host "Uploading $($installer.Name) to VirusTotal..."
Write-Host "SHA-256: $sha256"

$client = New-Object System.Net.Http.HttpClient
$client.DefaultRequestHeaders.Add('x-apikey', $apiKey)

try {
    $stream = [System.IO.File]::OpenRead($installer.FullName)
    $fileContent = New-Object System.Net.Http.StreamContent($stream)
    $form = New-Object System.Net.Http.MultipartFormDataContent
    $form.Add($fileContent, 'file', $installer.Name)

    try {
        $response = $client.PostAsync("$apiUrl/files", $form).GetAwaiter().GetResult()
        try {
            $upload = Read-VirusTotalResponse $response
        }
        finally {
            $response.Dispose()
        }
    }
    finally {
        $form.Dispose()
    }

    $analysisId = [Uri]::EscapeDataString($upload.data.id)
    $deadline = [DateTime]::UtcNow.AddMinutes($timeoutMinutes)

    do {
        Start-Sleep -Seconds $pollIntervalSeconds
        $response = $client.GetAsync("$apiUrl/analyses/$analysisId").GetAwaiter().GetResult()
        try {
            $analysis = Read-VirusTotalResponse $response
        }
        finally {
            $response.Dispose()
        }
        Write-Host "Analysis status: $($analysis.data.attributes.status)"
    } while ($analysis.data.attributes.status -ne 'completed' -and [DateTime]::UtcNow -lt $deadline)

    if ($analysis.data.attributes.status -ne 'completed') {
        throw "VirusTotal analysis did not complete within $timeoutMinutes minutes."
    }

    $stats = $analysis.data.attributes.stats
    $detections = [int]$stats.malicious + [int]$stats.suspicious
    Write-Host "Malicious: $($stats.malicious); suspicious: $($stats.suspicious); undetected: $($stats.undetected)"
    Write-Host "Report: https://www.virustotal.com/gui/file/$sha256"

    if ($detections -gt 0) {
        $analysis.data.attributes.results.PSObject.Properties |
            Where-Object { $_.Value.category -in @('malicious', 'suspicious') } |
            ForEach-Object { Write-Host "$($_.Name): $($_.Value.category) - $($_.Value.result)" }
        throw "VirusTotal found $detections malicious or suspicious result(s)."
    }

    Write-Host 'VirusTotal check passed.'
}
finally {
    $client.Dispose()
}
