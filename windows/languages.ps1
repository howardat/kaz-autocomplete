# Add KazType under Kazakh in this user's language list, or remove it (-Remove).
# Per user: run it as the user who will type, elevation is not needed.
param([switch]$Remove)
$ErrorActionPreference = 'Stop'

$Tip = '043F:{D47F4271-B272-4D65-A2EB-E6D7D4CE8652}{CE05D587-91CD-44C2-BE3A-BCCAD5A33BEE}'
$list = Get-WinUserLanguageList

if ($Remove) {
    foreach ($lang in $list) { [void]$lang.InputMethodTips.Remove($Tip) }
    # Drop Kazakh entirely if KazType was its only input method.
    $empty = @($list | Where-Object { $_.LanguageTag -like 'kk*' -and $_.InputMethodTips.Count -eq 0 })
    foreach ($lang in $empty) { [void]$list.Remove($lang) }
} else {
    $kk = $list | Where-Object { $_.LanguageTag -like 'kk*' } | Select-Object -First 1
    if (-not $kk) {
        $list.Add('kk-KZ')
        $kk = $list[$list.Count - 1]
        $kk.InputMethodTips.Clear()  # only KazType, not the stock Kazakh keyboard
    }
    if ($kk.InputMethodTips -notcontains $Tip) { $kk.InputMethodTips.Add($Tip) }
}
Set-WinUserLanguageList $list -Force
