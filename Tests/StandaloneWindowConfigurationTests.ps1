$ErrorActionPreference = 'Stop'

$headerPath = Join-Path $PSScriptRoot '..\JUCE-master\modules\juce_audio_plugin_client\Standalone\juce_StandaloneFilterWindow.h'
$header = Get-Content -LiteralPath $headerPath -Raw

if ($header -notmatch 'DocumentWindow::minimiseButton\s*\|\s*DocumentWindow::maximiseButton\s*\|\s*DocumentWindow::closeButton') {
    throw 'StandaloneFilterWindow must create a native maximise button.'
}

if ($header -notmatch 'setTitleBarButtonsRequired\s*\(\s*DocumentWindow::minimiseButton\s*\|\s*DocumentWindow::maximiseButton\s*\|\s*DocumentWindow::closeButton\s*,\s*false\s*\)') {
    throw 'StandaloneFilterWindow must retain the native maximise button in its title bar.'
}

if ($header -notmatch 'setResizable\s*\(\s*true\s*,\s*false\s*\)') {
    throw 'StandaloneFilterWindow must force the OpenTune standalone window to be resizable.'
}

Write-Output 'StandaloneWindowConfigurationTests passed'
