object FlashTestForm1: TFlashTestForm1
  Left = 247
  Top = 466
  Width = 549
  Height = 558
  Caption = ' Flash RAM Test Panel'
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
    Width = 541
    Height = 15
    Align = alTop
    Alignment = taCenter
    AutoSize = False
    Caption = 'ST Flash RAM'
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
    Width = 541
    Height = 8
    Cursor = crVSplit
    Align = alTop
    Beveled = True
  end
  object Panel24: TPanel
    Left = 0
    Top = 15
    Width = 541
    Height = 227
    Align = alTop
    Caption = 'Panel24'
    TabOrder = 0
    object Panel17: TPanel
      Left = 75
      Top = 1
      Width = 465
      Height = 225
      Align = alClient
      BevelOuter = bvLowered
      Caption = 'Panel3'
      TabOrder = 0
      object DumpMemo1: TMemo
        Left = 1
        Top = 27
        Width = 463
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
        Width = 463
        Height = 26
        Align = alTop
        BevelOuter = bvLowered
        Color = clBlack
        TabOrder = 1
        object Label43: TLabel
          Left = 1
          Top = 7
          Width = 461
          Height = 18
          Align = alBottom
          AutoSize = False
          Caption = 'Address Data....................  Text.....'
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
      Width = 74
      Height = 225
      Align = alLeft
      BevelInner = bvLowered
      BevelOuter = bvNone
      TabOrder = 1
      object Label36: TLabel
        Left = 2
        Top = 45
        Width = 70
        Height = 13
        Alignment = taCenter
        AutoSize = False
        Caption = 'Address'
        Color = clGray
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clSilver
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = False
        ParentFont = False
      end
      object Label42: TLabel
        Left = 2
        Top = 81
        Width = 71
        Height = 13
        Alignment = taCenter
        AutoSize = False
        Caption = 'Data'
        Color = clGray
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clSilver
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = False
        ParentFont = False
      end
      object STInitButton1: TButton
        Left = 2
        Top = 2
        Width = 70
        Height = 21
        Caption = 'Initialize'
        TabOrder = 0
        OnClick = STInitButton1Click
      end
      object GetStatusButton1: TButton
        Left = 2
        Top = 23
        Width = 70
        Height = 21
        Caption = 'Get Status'
        TabOrder = 1
        OnClick = GetStatusButton1Click
      end
      object EraseButton1: TButton
        Left = 2
        Top = 115
        Width = 70
        Height = 21
        Caption = 'EraseSector'
        TabOrder = 2
        OnClick = EraseButton1Click
      end
      object ReadSTButton1: TButton
        Left = 2
        Top = 136
        Width = 70
        Height = 21
        Caption = 'Read Block'
        TabOrder = 3
        OnClick = ReadSTButton1Click
      end
      object WriteSTButton1: TButton
        Left = 2
        Top = 157
        Width = 70
        Height = 21
        Caption = 'WriteBlock'
        TabOrder = 4
        OnClick = WriteSTButton1Click
      end
      object ChipIDButton1: TButton
        Left = 2
        Top = 199
        Width = 70
        Height = 21
        Caption = 'Chip ID'
        TabOrder = 5
        OnClick = ChipIDButton1Click
      end
      object BlockEdit1: TEdit
        Left = 1
        Top = 59
        Width = 53
        Height = 21
        TabOrder = 6
        Text = '000000'
      end
      object FlashDataEdit1: TEdit
        Left = 0
        Top = 94
        Width = 73
        Height = 21
        TabOrder = 7
        Text = '00 00 00 00'
      end
      object WriteStatButton1: TButton
        Left = 2
        Top = 178
        Width = 70
        Height = 21
        Caption = 'Set Status'
        TabOrder = 8
        OnClick = WriteStatButton1Click
      end
      object UpDown1: TUpDown
        Left = 55
        Top = 59
        Width = 17
        Height = 23
        Min = 0
        Max = 32767
        Position = 0
        TabOrder = 9
        Wrap = False
        OnClick = UpDown1Click
      end
    end
  end
  object Panel23: TPanel
    Left = 0
    Top = 250
    Width = 541
    Height = 281
    Align = alClient
    TabOrder = 1
    object Label30: TLabel
      Left = 1
      Top = 1
      Width = 539
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
    object STDialogMemo1: TMemo
      Left = 1
      Top = 14
      Width = 539
      Height = 266
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
