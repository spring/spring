unit main;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls;

type
  TForm1 = class(TForm)
    Button1: TButton;
    Memo1: TMemo;
    Button2: TButton;
    Memo2: TMemo;
    Button3: TButton;
    Memo3: TMemo;
    Edit1: TEdit;
    Button4: TButton;
    Edit2: TEdit;
    Button5: TButton;
    Edit3: TEdit;
    Edit4: TEdit;
    Button6: TButton;
    Memo4: TMemo;
    Button7: TButton;
    Memo5: TMemo;
    Button8: TButton;
    Button9: TButton;
    procedure Button1Click(Sender: TObject);
    procedure FormActivate(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure Button6Click(Sender: TObject);
    procedure Button7Click(Sender: TObject);
    procedure Button8Click(Sender: TObject);
    procedure Button9Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

//function Init(isServer :boolean) :integer; stdcall; external '..\Release\unitsync.dll';

type
  TInit = function(id :integer; isServer :boolean) :integer; stdcall;
  TGetCurrentList = function() :PChar; stdcall;
  TProcessUnits = function :integer; stdcall;
  TAddClient = procedure(id :integer; list :PChar); stdcall;
  TRemoveClient = procedure(id :integer); stdcall;
  TGetClientDiff = function(id :integer) :PChar; stdcall;
  TInstallClientDiff = procedure(list :PChar); stdcall;

  TGetUnitCount = function :integer; stdcall;
  TGetUnitName = function(id :integer) :PChar; stdcall;
  TIsUnitDisabled = function(id :integer) :integer; stdcall;
  TIsUnitDisabledByClient = function(id, client :integer) :integer; stdcall;

  TInitArchiveScanner = function :integer; stdcall;
  TAddAllArchives = procedure(root :PChar); stdcall;
  TGetPrimaryModCount = function :integer; stdcall;
  TGetPrimaryModName = function(id :integer) :PChar; stdcall;
  TGetPrimaryModArchiveCount = function(id :integer) :integer; stdcall;
  TGetPrimaryModArchiveList = function(arnr :integer) :PChar; stdcall;
  TGetArchiveChecksum = function(name :PChar) :longword; stdcall;

  TGetSideCount = function :longword; stdcall;
  TGetSideName = function(id :integer) :PChar; stdcall;

var
  Init :TInit;
  GetCurrentList :TGetCurrentList;
  ProcessUnits :TProcessUnits;
  AddClient :TAddClient;
  RemoveClient :TRemoveClient;
  GetClientDiff :TGetClientDiff;
  InstallClientDiff :TInstallClientDiff;

  GetUnitCount :TGetUnitCount;
  GetUnitName :TGetUnitName;
  IsUnitDisabled :TIsUnitDisabled;
  IsUnitDisabledByClient :TIsUnitDisabledByClient;

  InitArchiveScanner :TInitArchiveScanner;
  AddAllArchives :TAddAllArchives;
  GetPrimaryModCount :TGetPrimaryModCount;
  GetPrimaryModName :TGetPrimaryModName;
  GetPrimaryModArchiveCount :TGetPrimaryModArchiveCount;
  GetPrimaryModArchiveList :TGetPrimaryModArchiveList;
  GetArchiveChecksum :TGetArchiveChecksum;

  GetSideCount :TGetSideCount;
  GetSideName :TGetSideName;

{$R *.dfm}

procedure TForm1.Button1Click(Sender: TObject);
var
  buf :pchar;
begin
  buf := GetCurrentList;
  memo1.Lines.add(inttostr(strlen(buf)));
  memo1.Lines.add(buf);
end;

procedure TForm1.FormActivate(Sender: TObject);
var
  lib :integer;
  r :integer;
begin
//  ChDir('..\..\rts\bagge');
  ChDir('c:\projekt\projekt\taspring\rts\bagge');
  //showmessage(GetCurrentDir);

//  lib := Loadlibrary('..\..\unitsync\Release\unitsync.dll');
  lib := LoadLibrary('unitsync.dll');
  //lib := Loadlibrary('c:\projekt\projekt\taspring\unitsync\Release\unitsync.dll');

  Init := GetProcAddress(lib, 'Init');
  GetCurrentList := GetProcAddress(lib, 'GetCurrentList');
  ProcessUnits := GetProcAddress(lib, 'ProcessUnitsNoChecksum');
  AddClient := GetProcAddress(lib, 'AddClient');
  RemoveClient := GetProcAddress(lib, 'RemoveClient');
  GetClientDiff := GetProcAddress(lib, 'GetClientDiff');
  InstallClientDiff := GetProcAddress(lib, 'InstallClientDiff');

  GetUnitCount := GetProcAddress(lib, 'GetUnitCount');
  GetUnitName := GetProcAddress(lib, 'GetUnitName');
  IsUnitDisabled := GetProcAddress(lib, 'IsUnitDisabled');
  IsUnitDisabledByClient := GetProcAddress(lib, 'IsUnitDisabledByClient');

  InitArchiveScanner := GetProcAddress(lib, 'InitArchiveScanner');
  AddAllArchives := GetProcAddress(lib, 'AddAllArchives');
  GetArchiveChecksum := GetProcAddress(lib, 'GetArchiveChecksum');
  GetPrimaryModCount := GetProcAddress(lib, 'GetPrimaryModCount');
  GetPrimaryModName := GetProcAddress(lib, 'GetPrimaryModName');
  GetPrimaryModArchiveCount := GetProcAddress(lib, 'GetPrimaryModArchiveCount');
  GetPrimaryModArchiveList := GetProcAddress(lib, 'GetPrimaryModArchiveList');

  GetSideCount := GetProcAddress(lib, 'GetSideCount');
  GetSideName := GetProcAddress(lib, 'GetSideName');

  r := Init(1, true);
end;

procedure TForm1.Button2Click(Sender: TObject);
var
  num :integer;
begin
  InitArchiveScanner;
  AddAllArchives('xta_se_v060.sdz');
  num := ProcessUnits;
  while num > 0 do
  begin
    memo1.Lines.Add(inttostr(num));

    num := ProcessUnits;
    application.ProcessMessages;
  end;
end;

procedure TForm1.Button3Click(Sender: TObject);
var
  s :String;
  id :integer;
begin
  s := memo2.text;
  id := strtoint(edit1.text);
  AddClient(id, @s[1]);
end;

procedure TForm1.Button4Click(Sender: TObject);
var
  p :Pchar;
  id :integer;
begin
  id := strtoint(edit2.text);
  p := GetClientDiff(id);
  memo3.lines.add(p);
end;

procedure TForm1.Button5Click(Sender: TObject);
var
  p :pchar;
  id :integer;
begin
  id := strtoint(edit3.text);
  p := GetUnitName(id);
  edit4.text := p;
end;

procedure TForm1.Button6Click(Sender: TObject);
var
  s :String;
begin
  s := memo3.text;
  InstallClientDiff(@s[1]);
end;

procedure TForm1.Button7Click(Sender: TObject);
var
  tot :integer;
  i :integer;
begin
  tot := GetUnitCount();
  memo4.lines.Add(inttostr(tot));

  for i := 0 to tot - 1 do
  begin
    if (IsUnitDisabled(i) = 1) then
      memo4.lines.add(GetUnitName(i));
  end;
end;

procedure TForm1.Button8Click(Sender: TObject);
var
  i, j :integer;
  k, l :integer;
  s :string;
begin
  InitArchiveScanner;
  i := GetPrimaryModCount;
  for j := 0 to i - 1 do
  begin
    memo5.lines.add(GetPrimaryModName(j));
    k := GetPrimaryModArchiveCount(j);
    for l := 0 to k - 1 do
    begin
      s := GetPrimaryModArchiveList(l);
      memo5.lines.add('--' + s + ' -- ' + inttostr(GetArchiveChecksum(pchar(s))));
    end;
  end;
end;

procedure TForm1.Button9Click(Sender: TObject);
var
  i, j :integer;
  k, l :integer;
  s :string;
begin
  InitArchiveScanner;
  i := GetSideCount;
  for j := 0 to i - 1 do
  begin
    memo5.lines.add(GetSideName(j));
{    k := GetPrimaryModArchiveCount(j);
    for l := 0 to k - 1 do
    begin
      s := GetPrimaryModArchiveList(l);
      memo5.lines.add('--' + s + ' -- ' + inttostr(GetArchiveChecksum(pchar(s))));
    end;}
  end;
end;

end.
