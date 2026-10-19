# Starts the debate backend against one of the two model servers.
#
#   .\backend\start-server.ps1            # Qwen 3.8 (llm.new.env), the usual
#   .\backend\start-server.ps1 -Llm old   # Qwen 3.6 (llm.old.env)
#
# The two model servers share one GPU, so only one is ever up; pick the one
# that is running. Each llm.*.env file holds the DEBATE_LLM_* variables the
# backend reads (see src/utils/LlmClient.cc). GOOGLE_CLIENT_ID is taken from
# frontend/.env.local so Google sign-in works without a second copy, and
# the session-signing secret from backend/session.secret (made if missing).
#
# Run from any directory. The server keeps the console; Ctrl+C stops it.
param(
    [ValidateSet("new", "old")]
    [string]$Llm = "new"
)

$backend = $PSScriptRoot
$root = Split-Path $backend -Parent

$envFile = Join-Path $backend "llm.$Llm.env"
foreach ($line in Get-Content $envFile) {
    if ($line -match '^\s*#' -or $line -match '^\s*$') { continue }
    $name, $value = $line -split '=', 2
    Set-Item -Path "env:$($name.Trim())" -Value $value.Trim()
}

# Sessions are signed with DEBATE_SESSION_SECRET (src/utils/SessionToken.h).
# Kept in backend/session.secret, made once and never committed, so a
# restart does not sign everyone out.
$secretFile = Join-Path $backend "session.secret"
if (-not (Test-Path $secretFile)) {
    $bytes = New-Object byte[] 32
    [System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
    $hex = -join ($bytes | ForEach-Object { $_.ToString("x2") })
    [System.IO.File]::WriteAllText($secretFile, $hex)
}
$env:DEBATE_SESSION_SECRET = (Get-Content $secretFile -Raw).Trim()

$frontendEnv = Join-Path $root "frontend\.env.local"
if (Test-Path $frontendEnv) {
    $id = (Get-Content $frontendEnv | Where-Object { $_ -match '^VITE_GOOGLE_CLIENT_ID=' }) -replace '^VITE_GOOGLE_CLIENT_ID=', ''
    if ($id) { $env:GOOGLE_CLIENT_ID = $id.Trim() }
}

Write-Host "Model server: $env:DEBATE_LLM_HOST`:$env:DEBATE_LLM_PORT ($env:DEBATE_LLM_MODEL) from llm.$Llm.env"
Set-Location $backend
& (Join-Path $backend "build\bin\debate_server.exe")
