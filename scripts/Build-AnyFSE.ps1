param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
$tasks = Get-Content -LiteralPath '.vscode/tasks.json' -Raw | ConvertFrom-Json
$label = if ($Configuration -eq 'Release') { 'Build AnyFSE Release Unsigned' } else { 'Build AnyFSE Debug' }
function Expand-TaskText([string]$Text) {
    return $Text.Replace('${workspaceFolder}', $projectRoot)
}
function Quote-TaskArgument([string]$Text) {
    $expanded = Expand-TaskText $Text
    if ($expanded.Contains('"')) { throw "Argumento com aspas internas nao suportado: $Text" }
    return '"' + $expanded + '"'
}
function Invoke-BuildTask([string]$TaskLabel) {
    $task = @($tasks.tasks | Where-Object label -eq $TaskLabel)
    if ($task.Count -ne 1) { throw "Tarefa ausente ou duplicada: $TaskLabel" }
    $task = $task[0]
    foreach ($dependency in @($task.dependsOn)) {
        if ($dependency) { Invoke-BuildTask $dependency }
    }
    $shell = $tasks.windows.options.shell
    if (-not $shell.executable -or -not $shell.args) { throw "Ambiente ausente para a tarefa: $TaskLabel" }
    $parts = @($shell.args | ForEach-Object { Expand-TaskText $_ })
    $parts += Quote-TaskArgument $task.command
    $parts += @($task.args | ForEach-Object { Quote-TaskArgument $_ })
    $workingDirectory = $projectRoot
    if ($task.options.cwd) { $workingDirectory = Expand-TaskText $task.options.cwd }
    Push-Location -LiteralPath $workingDirectory
    try {
        Write-Host "Executando tarefa: $TaskLabel"
        & (Expand-TaskText $shell.executable) ($parts -join ' ')
        if ($LASTEXITCODE -ne 0) { throw "Tarefa '$TaskLabel' falhou (codigo $LASTEXITCODE)." }
    } finally { Pop-Location }
}
try {
    Invoke-BuildTask $label
    Write-Host "Compilacao concluida: build\$Configuration" -ForegroundColor Green
    exit 0
} catch {
    Write-Host $_ -ForegroundColor Red
    exit 1
}
