Function Register-Watcher {
    param ($folder)
    $filter = "*.*" #all files
    $watcher = New-Object IO.FileSystemWatcher $folder, $filter -Property @{ 
        IncludeSubdirectories = $false
        EnableRaisingEvents = $true
    }

    $changeAction = [scriptblock]::Create('
        # This is the code which will be executed every time a file change is detected
        $path = $Event.SourceEventArgs.FullPath
        $name = $Event.SourceEventArgs.Name
        $changeType = $Event.SourceEventArgs.ChangeType
        $timeStamp = $Event.TimeGenerated
        Write-Host "Running run_tests.lua..."
        $iexResult = cmd /c lua.exe assets\l\run_tests.lua 2`>`&1
        Write-Host ($iexResult | Out-String)
        Write-Host "Finished running run_tests.lua!"
    ')

    Register-ObjectEvent $Watcher "Changed" -Action $changeAction
}

Register-Watcher "C:\w\src\github.com\lache\lo\sea-server\assets\l"
