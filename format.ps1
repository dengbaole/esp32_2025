#Requires -Version 3.0
$srcfiles = @()
$Excludes = (".*", "build", ".git", "utils_bin")
$ExcludeFiles = ("assets.c", "assets.h")
Get-ChildItem -Exclude $Excludes |
Get-ChildItem -File -Recurse -exclude $ExcludeFiles |
where {$_ -notin $ExcludeFiles -and $_.extension -in ".c",".h"} |
	ForEach {
		$srcfiles += $_.FullName
	}
powershell "clang-format -style=file --verbose -i ${srcfiles}"
