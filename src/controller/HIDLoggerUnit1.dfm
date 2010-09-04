object LoggerForm1: TLoggerForm1
  Left = 250
  Top = 473
  Width = 422
  Height = 447
  Caption = ' HID Data Logger Panel'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 13
  object HidLoggerMemo1: TMemo
    Left = 0
    Top = 31
    Width = 414
    Height = 389
    Align = alClient
    Color = 14408663
    ScrollBars = ssVertical
    TabOrder = 0
  end
  object Panel5: TPanel
    Left = 0
    Top = 0
    Width = 414
    Height = 31
    Align = alTop
    BevelOuter = bvLowered
    TabOrder = 1
    object StartMonButton1: TButton
      Left = 2
      Top = 2
      Width = 74
      Height = 25
      Caption = 'Start Monitor'
      TabOrder = 0
      OnClick = StartMonButton1Click
    end
    object StopMonButton1: TButton
      Left = 78
      Top = 2
      Width = 72
      Height = 25
      Caption = 'Stop Monitor'
      TabOrder = 1
      OnClick = StopMonButton1Click
    end
    object SendReportButton1: TButton
      Left = 152
      Top = 2
      Width = 67
      Height = 25
      Caption = 'Send Data'
      TabOrder = 2
      OnClick = SendReportButton1Click
    end
    object ReadHidButton1: TButton
      Left = 221
      Top = 2
      Width = 67
      Height = 25
      Caption = 'Read Data'
      TabOrder = 3
      OnClick = ReadHidButton1Click
    end
    object Edit1: TEdit
      Left = 289
      Top = 4
      Width = 29
      Height = 21
      TabOrder = 4
      Text = '0x01'
    end
    object Edit2: TEdit
      Left = 319
      Top = 4
      Width = 34
      Height = 21
      TabOrder = 5
      Text = '0x00'
    end
    object CheckBox1: TCheckBox
      Left = 356
      Top = 6
      Width = 54
      Height = 17
      Caption = 'Monitor On'
      TabOrder = 6
      OnClick = CheckBox1Click
    end
  end
  object Timer2: TTimer
    Enabled = False
    OnTimer = Timer2Timer
    Left = 20
    Top = 40
  end
end
