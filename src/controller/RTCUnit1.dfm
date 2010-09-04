object RTCForm1: TRTCForm1
  Left = 284
  Top = 498
  Width = 439
  Height = 485
  Caption = ' RTC Test Panel'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Label35: TLabel
    Left = 0
    Top = 0
    Width = 431
    Height = 15
    Align = alTop
    Alignment = taCenter
    AutoSize = False
    Caption = 'Real Time Clock'
    Color = clGray
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clSilver
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = [fsBold]
    ParentColor = False
    ParentFont = False
    Layout = tlCenter
  end
  object Splitter1: TSplitter
    Left = 0
    Top = 242
    Width = 431
    Height = 8
    Cursor = crVSplit
    Align = alTop
    Beveled = True
  end
  object Panel24: TPanel
    Left = 0
    Top = 15
    Width = 431
    Height = 227
    Align = alTop
    Caption = 'Panel24'
    TabOrder = 0
    object Panel17: TPanel
      Left = 72
      Top = 1
      Width = 358
      Height = 225
      Align = alClient
      BevelOuter = bvLowered
      Caption = 'Panel3'
      TabOrder = 0
      object DumpMemo1: TMemo
        Left = 1
        Top = 27
        Width = 356
        Height = 197
        Align = alClient
        Font.Charset = ANSI_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ReadOnly = True
        ScrollBars = ssVertical
        TabOrder = 0
        WordWrap = False
      end
      object Panel18: TPanel
        Left = 1
        Top = 1
        Width = 356
        Height = 26
        Align = alTop
        BevelOuter = bvLowered
        Color = clBlack
        TabOrder = 1
        object Label43: TLabel
          Left = 1
          Top = 7
          Width = 354
          Height = 18
          Align = alBottom
          AutoSize = False
          Caption = 'Addr Data.........  Interpretaion.....'
          Color = clGray
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clSilver
          Font.Height = -11
          Font.Name = 'Courier New'
          Font.Style = []
          ParentColor = False
          ParentFont = False
          Layout = tlCenter
        end
      end
    end
    object Panel16: TPanel
      Left = 1
      Top = 1
      Width = 71
      Height = 225
      Align = alLeft
      BevelInner = bvLowered
      BevelOuter = bvNone
      TabOrder = 1
      object Label36: TLabel
        Left = 2
        Top = 2
        Width = 67
        Height = 13
        Alignment = taCenter
        AutoSize = False
        Caption = 'Addr    Data'
        Color = clGray
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clSilver
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = False
        ParentFont = False
      end
      object ReadRTCButton1: TButton
        Left = 2
        Top = 38
        Width = 67
        Height = 21
        Caption = 'Read'
        TabOrder = 0
      end
      object SetTrickleButton1: TButton
        Left = 2
        Top = 200
        Width = 67
        Height = 23
        Caption = 'Set Trickle'
        TabOrder = 1
        OnClick = SetTrickleButton1Click
      end
      object WriteRTCButton1: TButton
        Left = 2
        Top = 60
        Width = 67
        Height = 21
        Caption = 'Write'
        TabOrder = 2
        OnClick = WriteRTCButton1Click
      end
      object RTCSyncButton1: TButton
        Left = 2
        Top = 134
        Width = 67
        Height = 21
        Caption = 'Sync to PC'
        TabOrder = 3
        OnClick = RTCSyncButton1Click
      end
      object ReadAllRTCButton1: TButton
        Left = 2
        Top = 90
        Width = 67
        Height = 21
        Caption = 'Read All'
        TabOrder = 4
        OnClick = ReadAllRTCButton1Click
      end
      object RTCAddrEdit1: TEdit
        Left = 1
        Top = 16
        Width = 26
        Height = 21
        TabOrder = 5
        Text = '00'
      end
      object RTCDataEdit1: TEdit
        Left = 39
        Top = 16
        Width = 30
        Height = 21
        TabOrder = 6
        Text = '00'
      end
      object UpDown1: TUpDown
        Left = 26
        Top = 16
        Width = 12
        Height = 21
        Min = 0
        Max = 32767
        Position = 0
        TabOrder = 7
        Wrap = False
        OnClick = UpDown1Click
      end
      object WriteAllRTCButton1: TButton
        Left = 2
        Top = 112
        Width = 67
        Height = 21
        Caption = 'Write All'
        TabOrder = 8
      end
    end
  end
  object Panel23: TPanel
    Left = 0
    Top = 250
    Width = 431
    Height = 208
    Align = alClient
    TabOrder = 1
    object Label30: TLabel
      Left = 1
      Top = 1
      Width = 429
      Height = 13
      Align = alTop
      Alignment = taCenter
      Caption = 'Dialog Window'
      Color = clGray
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clSilver
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsBold]
      ParentColor = False
      ParentFont = False
      Layout = tlCenter
    end
    object RTCDialogMemo1: TMemo
      Left = 1
      Top = 14
      Width = 429
      Height = 193
      Align = alClient
      Color = 14408663
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Courier New'
      Font.Style = []
      ParentFont = False
      ScrollBars = ssVertical
      TabOrder = 0
    end
  end
end
