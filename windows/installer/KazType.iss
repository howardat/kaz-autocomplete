; KazType installer (Inno Setup 6.3+). Built by build.ps1 from dist\:
;   iscc /DAppVersion=0.1.0 installer\KazType.iss  ->  dist\KazType-Setup-0.1.0.exe
; Installs the native DLL (x64 or arm64) and the x86 DLL for 32-bit apps into
; Program Files\KazType, registers them, and adds KazType to the user's languages.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif

[Setup]
AppId={{8E3B6C1A-5F2D-4B7E-9A41-2C6D0F7B9E35}
AppName=KazType
AppVersion={#AppVersion}
AppVerName=KazType {#AppVersion}
AppPublisher=KazType
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\KazType
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible or arm64
ArchitecturesInstallIn64BitMode=x64compatible or arm64
MinVersion=10.0
SetupIconFile=..\Resources\KazType.ico
UninstallDisplayIcon={app}\x86\KazType.dll
UninstallDisplayName=KazType
OutputDir=..\dist
OutputBaseFilename=KazType-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Apps keep the DLL loaded; CurStepChanged moves it aside instead of asking to close them.
CloseApplications=no
RestartIfNeededByRun=no

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
en.AddingInputMethod=Adding KazType to your input methods...
ru.AddingInputMethod=Добавление KazType в способы ввода...
en.Finished=Switch to "Kazakh - KazType" with Win+Space. Apps that were already open pick it up after a restart.
ru.Finished=Переключитесь на «Казахский - KazType» сочетанием Win+Пробел. Уже открытые приложения увидят его после перезапуска.

[Messages]
en.FinishedLabelNoIcons=Setup has installed KazType.%n%n{cm:Finished}
ru.FinishedLabelNoIcons=Программа установила KazType.%n%n{cm:Finished}

[Files]
; Each architecture's DLL serves apps of that architecture. x64 apps emulated on ARM64 are not supported.
Source: "..\dist\x64\KazType.dll"; DestDir: "{app}\x64"; Check: not IsArm64; Flags: ignoreversion regserver uninsrestartdelete
Source: "..\dist\x64\kaztype.bin"; DestDir: "{app}\x64"; Check: not IsArm64; Flags: ignoreversion uninsrestartdelete
Source: "..\dist\arm64\KazType.dll"; DestDir: "{app}\arm64"; Check: IsArm64; Flags: ignoreversion regserver uninsrestartdelete
Source: "..\dist\arm64\kaztype.bin"; DestDir: "{app}\arm64"; Check: IsArm64; Flags: ignoreversion uninsrestartdelete
Source: "..\dist\x86\KazType.dll"; DestDir: "{app}\x86"; Flags: ignoreversion regserver 32bit uninsrestartdelete
Source: "..\dist\x86\kaztype.bin"; DestDir: "{app}\x86"; Flags: ignoreversion uninsrestartdelete
Source: "..\languages.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Run]
; The language list is per user: run as the user who started setup, not the elevated admin.
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; \
  Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\languages.ps1"""; \
  StatusMsg: "{cm:AddingInputMethod}"; Flags: runhidden runasoriginaluser waituntilterminated

[UninstallRun]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; \
  Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\languages.ps1"" -Remove"; \
  RunOnceId: "RemoveInputMethod"; Flags: runhidden waituntilterminated

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
var
  Moved: Integer;

{ Apps that use KazType keep the old DLL loaded; renaming it still works. }
procedure MoveAside(const Path: String);
var
  Old: String;
begin
  if FileExists(Path) then begin
    Moved := Moved + 1;
    Old := Path + '.' + GetDateTimeString('yyyymmddhhnnss', #0, #0) + IntToStr(Moved) + '.old';
    if RenameFile(Path, Old) then
      DeleteFile(Old);
  end;
end;

procedure PrepareDir(const Dir: String);
var
  R: TFindRec;
begin
  { Leftovers from earlier upgrades whose apps have since closed. }
  if FindFirst(Dir + '\*.old', R) then begin
    try
      repeat
        DeleteFile(Dir + '\' + R.Name);
      until not FindNext(R);
    finally
      FindClose(R);
    end;
  end;
  MoveAside(Dir + '\KazType.dll');
  MoveAside(Dir + '\kaztype.bin');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    PrepareDir(ExpandConstant('{app}\x64'));
    PrepareDir(ExpandConstant('{app}\arm64'));
    PrepareDir(ExpandConstant('{app}\x86'));
  end;
end;
