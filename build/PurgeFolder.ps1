<#
.SYNOPSIS
    Purges a folder once it is no longer locked.

.DESCRIPTION
    Continuously attempts to remove the specified folder. If the folder
    is locked or in use, the script waits for a configurable interval and
    retries until either the folder is removed or a timeout is reached.

.PARAMETER Path
    The folder to remove.

.PARAMETER RetrySeconds
    Number of seconds to wait between retry attempts.

.PARAMETER TimeoutSeconds
    Maximum number of seconds to wait before giving up.

.EXAMPLE
    .\PurgeFolder.ps1 -Path "C:\Temp\MyFolder" -Verbose
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string]$Path,

    [Parameter()]
    [ValidateRange(1, 86400)]
    [int]$RetrySeconds = 5,

    [Parameter()]
    [ValidateRange(1, 86400)]
    [int]$TimeoutSeconds = 300
)

if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
    throw "'$Path' does not exist or is not a folder."
}

$stopTime = (Get-Date).AddSeconds($TimeoutSeconds)

while (Test-Path -LiteralPath $Path -PathType Container) {

    if ((Get-Date) -gt $stopTime) {
        throw "Timed out waiting for folder '$Path' to become available after $TimeoutSeconds seconds."
    }

    try {
        Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop

        Write-Verbose "Successfully purged folder '$Path'."
    }
    catch {
        Write-Verbose "Folder is locked or in use. Retrying in $RetrySeconds second(s)..."
        Start-Sleep -Seconds $RetrySeconds
    }
}
