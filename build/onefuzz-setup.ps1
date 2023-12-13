function Execute-Setup {
    # Temporary work-around while OneFuzz does not run script from setup dir
    Set-Location -Path $PSScriptRoot

    Write-Log "Executing custom setup script in $(pwd)"

    # Exclude any uploaded DLL from known DLLs
    gci -filter '*.dll' | Exclude-Library

    # Done. Useful to know that the script did not prematurely error out
    Write-Log 'Setup script finished successfully'
}

# Write log statements into the event log.
function Write-Log {
    Param(
        [Parameter(Position=0,
                   Mandatory,
                   ValueFromPipeline,
                   ValueFromRemainingArguments)]
        [String[]] $Messages,
        [Parameter()] [Int] $EventId=0)
    Begin {
        $EventSource = 'onefuzz-setup.ps1'
        $EventLog = 'Application'
        if (![System.Diagnostics.EventLog]::SourceExists($EventSource)) {
            New-EventLog -LogName $EventLog -Source $EventSource
        }
    }
    Process {
        $Messages.ForEach({
            Write-EventLog -LogName $EventLog -Source $EventSource -EventId $EventId -EntryType Information -Message $_
        })
    }
    End {}
}

# This function is used to exclude DLL's that the fuzzer is dependent on. The dependent DLL's
# have been built with ASan and copied into the setup directory along with the fuzzer. 
function Exclude-Library {
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
        if ($ExistingProperty -ne $null) {
            $ExistingExclusions = $ExistingProperty.$Name.ForEach("ToLower")
        }

        # Normalize DLL name to lowercase for comparison, remove duplicates
        $Libs = $Libraries.ForEach("ToLower") | Select-Object -Unique
        Write-Log "Excluding libraries $Libs"

        # Discard empty strings and libraries already excluded
        $Libs = $Libs.Where({$_.Length -gt 0 -and !($ExistingExclusions.Contains($_))})

        # If anything remains either add or update registry key
        if ($Libs.Length -gt 0) {
            if ($ExistingProperty -eq $null) {
                # Create registry key to exclude DLLs
                New-ItemProperty -Path $Path -Name $Name -PropertyType MultiString -Value $Libs
                Write-Log "Created known DLLs exclusions with $Libs"
            } else {
                # Update registry key to exclude DLLs
                Set-ItemProperty -Path $Path -Name $Name -Value ($ExistingProperty.$Name + $Libs)
                Write-Log "Updated known DLLs exclusions with $Libs"
            }
        } else {
            # DLLs already excluded
            Write-Log "Known DLL exclusions already exist for $Libraries"
        }
    }
    End {}
}

Execute-Setup
