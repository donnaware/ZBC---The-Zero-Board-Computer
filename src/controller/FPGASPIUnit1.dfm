object FPGASPIForm1: TFPGASPIForm1
  Left = 247
  Top = 466
  Width = 448
  Height = 309
  Caption = ' FPGA SPI Test Panel'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Splitter1: TSplitter
    Left = 0
    Top = 48
    Width = 440
    Height = 8
    Cursor = crVSplit
    Align = alTop
    Beveled = True
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 440
    Height = 48
    Align = alTop
    TabOrder = 0
    object Panel22: TPanel
      Left = 1
      Top = 1
      Width = 438
      Height = 43
      Align = alTop
      BevelOuter = bvLowered
      TabOrder = 0
      object Label46: TLabel
        Left = 1
        Top = 1
        Width = 436
        Height = 13
        Align = alTop
        Caption = 
          '  FPGA SPI Transfer                            EEPROM Read/Write' +
          '  Addr   Data'
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
      object FPGASPIButton1: TButton
        Left = 5
        Top = 17
        Width = 84
        Height = 22
        Caption = 'SPI  XFER'
        TabOrder = 0
        OnClick = FPGASPIButton1Click
      end
      object SPIDataEdit1: TEdit
        Left = 98
        Top = 17
        Width = 31
        Height = 21
        TabOrder = 1
        Text = '00'
      end
      object ReadEEButton1: TButton
        Left = 226
        Top = 17
        Width = 63
        Height = 20
        Caption = 'Read EE'
        TabOrder = 2
        OnClick = ReadEEButton1Click
      end
      object WriteEEButton1: TButton
        Left = 292
        Top = 17
        Width = 63
        Height = 20
        Caption = 'Write EE'
        TabOrder = 3
        OnClick = WriteEEButton1Click
      end
      object EEAddrEdit1: TEdit
        Left = 358
        Top = 17
        Width = 26
        Height = 21
        TabOrder = 4
        Text = '00'
      end
      object UpDown1: TUpDown
        Left = 384
        Top = 17
        Width = 16
        Height = 20
        Min = 0
        Max = 32767
        Position = 0
        TabOrder = 5
        Wrap = False
        OnClick = UpDown1Click
      end
      object EEDataEdit1: TEdit
        Left = 401
        Top = 17
        Width = 31
        Height = 21
        TabOrder = 6
        Text = '00'
      end
    end
  end
  object Panel23: TPanel
    Left = 0
    Top = 56
    Width = 440
    Height = 226
    Align = alClient
    TabOrder = 1
    object Label30: TLabel
      Left = 1
      Top = 1
      Width = 438
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
    object SPIDialogMemo1: TMemo
      Left = 1
      Top = 14
      Width = 438
      Height = 211
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
