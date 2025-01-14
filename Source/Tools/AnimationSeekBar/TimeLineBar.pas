unit TimeLineBar;

interface

uses
  SysUtils, Classes, Controls, Types;

type
  TTimeLineBar = class(TGraphicControl)
//  KeyFrameWidth
  private
  m_nCurrentLeft      : Integer;
  m_nGraduationHeight : Integer;
    { Private declarations }
  protected
    { Protected declarations }
  procedure Paint; override;
  public
    { Public declarations }
  m_nRangeMin         : Integer;
  m_nRangeMax         : Integer;
  m_nGraduationWidth  : Integer;
  procedure SetGraduationWidth(nWidth : Integer);
  procedure SetGraduationHeight(nHeight : Integer);
  procedure SetViewPort(nLeft : Integer);
  function  GetViewPort(): Integer;
  function  GetFrameX(nFrame : Integer) : Integer;
  function GetFrameByX(nX : Integer) : Integer;
  constructor Create(AOwner: TComponent); override;
  published
    { Published declarations }
    property Anchors;
    property OnClick;
    property OnMouseMove;
    property OnMouseUp;
  end;

procedure Register;

implementation

constructor TTimeLineBar.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  m_nRangeMin := 0;
  m_nRangeMax := 1000;
  m_nGraduationHeight := 30;
  m_nCurrentLeft := 0;
  m_nGraduationWidth := 10;
end;

procedure Register;
begin
  RegisterComponents('Samples', [TTimeLineBar]);
end;

procedure TTimeLineBar.SetGraduationWidth(nWidth : Integer);
begin
  m_nGraduationWidth := nWidth;
end;

function  TTimeLineBar.GetViewPort(): Integer;
begin
  GetViewPort := m_nCurrentLeft;
end;

procedure TTimeLineBar.SetGraduationHeight(nHeight : Integer);
begin
  m_nGraduationHeight := nHeight;
end;

procedure TTimeLineBar.SetViewPort(nLeft : Integer);
begin
  if (nLeft < m_nRangeMin) then
    Exit;
  if (nLeft + Width > m_nRangeMax) then
  begin
    nLeft := m_nRangeMax - Width;
    if (nLeft < m_nRangeMin) then
      nLeft := m_nRangeMin;
  end;
  m_nCurrentLeft := nLeft;
  Paint();
end;

function  TTimeLineBar.GetFrameX(nFrame : Integer) : Integer;
var
  nReturn : Integer;
  nStartFrame : Integer;
begin
  nStartFrame := m_nCurrentLeft div m_nGraduationWidth;
  if nFrame < nStartFrame then
    nReturn := -1
  else
    nReturn := nFrame * m_nGraduationWidth - m_nCurrentLeft;
  GetFrameX := nReturn;
end;

function TTimeLineBar.GetFrameByX(nX : Integer) : Integer;
begin
    GetFrameByX := (m_nCurrentLeft + nX) div m_nGraduationWidth;
end;

procedure TTimeLineBar.Paint();
var
nFrameInView : Integer;
nFrameInRange : Integer;
i : Integer;
nHeight : Integer;
PositionMark : String;
PositionMarkWidth : Integer;
nStartFrame : Integer;
begin
  nFrameInRange := (m_nRangeMax - m_nCurrentLeft * m_nGraduationWidth) div m_nGraduationWidth;
  nFrameInView := Width div m_nGraduationWidth;
  Canvas.FillRect(ClientRect);
  Canvas.MoveTo(0, 0);
  Canvas.LineTo(Width, 0);
  nStartFrame := m_nCurrentLeft div m_nGraduationWidth;
  if nFrameInView > nFrameInRange then
    nFrameInView := nFrameInRange;
  for i := 0 to nFrameInView do
  begin
    if ((i + nStartFrame) mod 5 = 0) then
    begin
      nHeight := m_nGraduationHeight;
      PositionMark := Format('%d', [i + nStartFrame]);
      PositionMarkWidth := Canvas.TextWidth(PositionMark);
      Canvas.TextOut(i * m_nGraduationWidth + m_nGraduationWidth div 2 - PositionMarkWidth div 2, nHeight, PositionMark);
    end
    else
      nHeight := m_nGraduationHeight div 2;
    Canvas.MoveTo(i * m_nGraduationWidth + m_nGraduationWidth div 2, 0);
    Canvas.LineTo(i * m_nGraduationWidth + m_nGraduationWidth div 2, nHeight);
  end;
end;

end.
