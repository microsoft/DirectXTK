<#
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.
#>

function Invoke-Setup {
    # Temporary work-around while OneFuzz does not run script from setup dir
    Set-Location -Path $PSScriptRoot

    Write-OneFuzzLog "Executing custom setup script in $(Get-Location)"

    # Exclude any uploaded DLL from known DLLs
    Get-ChildItem -filter '*.dll' | Add-Exclude-Library

    # Done. Useful to know that the script did not prematurely error out
    Write-OneFuzzLog 'Setup script finished successfully'
}

# Write log statements into the event log.
function Write-OneFuzzLog {
    Param(
        [Parameter(Position=0,
                   Mandatory,
                   ValueFromPipeline,
                   ValueFromRemainingArguments)]
        [String[]] $Messages)
    Begin {
        $EventSource = 'onefuzz-setup.ps1'
        $EventLog = 'Application'
        if (![System.Diagnostics.EventLog]::SourceExists($EventSource)) {
            New-EventLog -LogName $EventLog -Source $EventSource
        }
    }
    Process {
        $Messages.ForEach({
            Write-EventLog -LogName $EventLog -Source $EventSource -EventId 0 -EntryType Information -Message $_
        })
    }
    End {}
}

# This function is used to exclude DLL's that the fuzzer is dependent on. The dependent DLL's
# have been built with ASan and copied into the setup directory along with the fuzzer.
function Add-Exclude-Library {
    Param(
        [Parameter(Position=0,
                   Mandatory,
                   ValueFromPipeline,
                   ValueFromRemainingArguments)]
        [String[]] $Libraries)
    Begin {
        $Path = 'HKLM:System\CurrentControlSet\Control\Session Manager'
        $Name = 'ExcludeFromKnownDLLs'
    }
    Process {
        # Get existing excluded libraries
        $ExistingProperty = Get-ItemProperty -Path $Path -Name $Name -ErrorAction SilentlyContinue
        $ExistingExclusions = @()

        # Normalize DLL name to lowercase for comparison
        if ($null -ne $ExistingProperty) {
            $ExistingExclusions = $ExistingProperty.$Name.ForEach("ToLower")
        }

        # Normalize DLL name to lowercase for comparison, remove duplicates
        $Libs = $Libraries.ForEach("ToLower") | Select-Object -Unique
        Write-OneFuzzLog "Excluding libraries $Libs"

        # Discard empty strings and libraries already excluded
        $Libs = $Libs.Where({$_.Length -gt 0 -and !($ExistingExclusions.Contains($_))})

        # If anything remains either add or update registry key
        if ($Libs.Length -gt 0) {
            if ($null -eq $ExistingProperty) {
                # Create registry key to exclude DLLs
                New-ItemProperty -Path $Path -Name $Name -PropertyType MultiString -Value $Libs
                Write-OneFuzzLog "Created known DLLs exclusions with $Libs"
            } else {
                # Update registry key to exclude DLLs
                Set-ItemProperty -Path $Path -Name $Name -Value ($ExistingProperty.$Name + $Libs)
                Write-OneFuzzLog "Updated known DLLs exclusions with $Libs"
            }
        } else {
            # DLLs already excluded
            Write-OneFuzzLog "Known DLL exclusions already exist for $Libraries"
        }
    }
    End {}
}

Invoke-Setup
