(Get-Content -path $args[0] -Raw) |
    ForEach-Object {
        $defstr="#define VERSION_BUILD ";
        $regex="$defstr(?<BuildVersion>\d*)";
        if($_ -match $regex) {
            $_ = $_ -replace $regex,"$($defstr)$(([int]$matches["BuildVersion"])+1)" 
        }
        $_
    } |
    Out-File $args[0] -encoding ascii -nonewline