//----------------------------------------------------------------------------
//  PIC Controller:
//  DonnaWare International LLP (C) 1958, All Rights Reserved
//----------------------------------------------------------------------------
#ifndef DOSeyUnit1H
#define DOSeyUnit1H
//----------------------------------------------------------------------------//
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Graphics.hpp>
#include <Dialogs.hpp>
#include <ExtDlgs.hpp>
#include <ComCtrls.hpp>
#include <Buttons.hpp>
#include <Menus.hpp>
#include <ToolWin.hpp>
#include <ImgList.hpp>
#include "JvHidControllerClass.h"
#include <OleCtrls.hpp>
#include "SHDocVw_OCX.h"
//----------------------------------------------------------------------------
#define     VersionNum      "1.1"           // Software version number
#define     ReportSize      64
//----------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
    TStatusBar *StatusBar1;
    TJvHidDeviceController *JvHidDeviceController1;
    TImageList *ImageList1;
    TPageControl *PageControl1;
    TTabSheet *TabSheet1;
    TPanel *PreviewPanel1;
    TCheckBox *LoggerCheckBox1;
    TTabSheet *TabSheet2;
    TPanel *Panel1;
    TCheckBox *MCULEDCheckBox1;
    TCheckBox *FPGALoadCheckBox1;
    TTabSheet *TabSheet3;
    TTabSheet *TabSheet4;
    TTabSheet *TabSheet5;
    TTabSheet *TabSheet6;
    TPanel *Panel2;
    TImage *AboutImage1;
    TLabel *Label3;
    TLabel *Label4;
    TLabel *Label5;
    TPanel *Panel3;
    TPanel *Panel4;
    TPanel *Panel5;
    TTabSheet *TabSheet7;
    TPanel *Panel6;
    TImage *Image2;
    TLabel *Label9;
    TLabel *Label10;
    TLabel *Label11;
    TLabel *Label12;
    TToolBar *ToolBar1;
    TToolButton *ToolButton1;
    TToolButton *ToolButton2;
	TToolButton *ToolButton3;
	TToolButton *ToolButton4;
	TToolButton *ToolButton5;
	TToolButton *ToolButton6;
	TToolButton *ToolButton7;
	TToolButton *ToolButton8;
	TToolButton *ToolButton9;
	TToolButton *ToolButton10;
	TToolButton *ToolButton11;
    TBitBtn *CheckDOSeyBitBtn1;
    TBitBtn *ConfigFPGABitBtn1;
    TBitBtn *RBFToFlashBitBtn1;
    TBitBtn *FLoppyToFlashBitBtn1;
    TStaticText *DOSeyFoundText1;
    TStaticText *DOSeyNotFoundText1;
    TBitBtn *SetRBFileBitBtn1;
    TBitBtn *SetFloppyFileBitBtn1;
    TOpenDialog *OpenRBFDialog1;
    TStaticText *FGPARBFText1;
    TStaticText *FloppyIMGText1;
    TOpenDialog *OpenIMGDialog1;
    TBitBtn *EraseFlashChipBitBtn1;
	TCppWebBrowser *CppWebBrowser1;
    TCheckBox *FilterMessagesCheckBox1;
    TBitBtn *FlashTestBitBtn1;
    TBitBtn *BIOSToFLASHBitBtn1;
    TBitBtn *BitBtn1;
    TStaticText *BIOSROMText1;
    TCheckBox *ShowToolBarCheckBox1;
    TBevel *Bevel1;
    TBitBtn *RTCTestBitBtn1;
    TBitBtn *BitBtn2;
    TCheckBox *EnableFlashCheckBox1;
    TLabel *Label1;
    TCheckBox *FPGAResetCheckBox1;
    TBitBtn *ShowFPGATestBitBtn1;
    TLabel *Label2;
    TLabel *Label6;
    TLabel *Label7;
    TBitBtn *FlashToFPGABitBtn1;
    TLabel *Label46;
    TLabel *Label8;
    TLabel *Label13;
    TLabel *Label14;
    TLabel *Label15;
    TBevel *Bevel2;
    TBevel *Bevel3;
    TCheckBox *FloppySelCheckBox1;
    TCheckBox *EnableFPGASPICheckBox1;
    TCheckBox *AutoBootCheckBox1;
    TBevel *Bevel4;
    bool __fastcall JvHidDeviceController1Enumerate(TJvHidDevice *HidDev,const int Idx);
    void __fastcall LoggerCheckBox1Click(TObject *Sender);
    void __fastcall AboutImage1Click(TObject *Sender);
    void __fastcall StatusBar1DrawPanel(TStatusBar *StatusBar,TStatusPanel *Panel, const TRect &Rect);
    void __fastcall TabSheet7Show(TObject *Sender);
    void __fastcall MCULEDCheckBox1Click(TObject *Sender);
	void __fastcall ToolButton9Click(TObject *Sender);
	void __fastcall ToolButton11Click(TObject *Sender);
	void __fastcall ShowToolBarCheckBox1Click(TObject *Sender);
	void __fastcall ToolButton8Click(TObject *Sender);
    void __fastcall CheckDOSeyBitBtn1Click(TObject *Sender);
    void __fastcall SetRBFileBitBtn1Click(TObject *Sender);
    void __fastcall SetFloppyFileBitBtn1Click(TObject *Sender);
    void __fastcall ConfigFPGABitBtn1Click(TObject *Sender);
    void __fastcall FPGALoadCheckBox1Click(TObject *Sender);
    void __fastcall TabSheet5Show(TObject *Sender);
    void __fastcall FlashTestBitBtn1Click(TObject *Sender);
    void __fastcall BIOSToFLASHBitBtn1Click(TObject *Sender);
    void __fastcall RTCTestBitBtn1Click(TObject *Sender);
    void __fastcall EnableFlashCheckBox1Click(TObject *Sender);
    void __fastcall FPGAResetCheckBox1Click(TObject *Sender);
    void __fastcall ShowFPGATestBitBtn1Click(TObject *Sender);
    void __fastcall RBFToFlashBitBtn1Click(TObject *Sender);
    void __fastcall FlashToFPGABitBtn1Click(TObject *Sender);
    void __fastcall FloppySelCheckBox1Click(TObject *Sender);
    void __fastcall EnableFPGASPICheckBox1Click(TObject *Sender);
    void __fastcall FLoppyToFlashBitBtn1Click(TObject *Sender);
    void __fastcall AutoBootCheckBox1Click(TObject *Sender);

private:	// User declarations


public:		// User declarations
	int VendorID;       //These are the vendor and product IDs to look for.
	int ProductID;      //Uses Lakeview Research's Vendor ID.

    bool Uploading;
    int Progress;
    AnsiString ProgressMsg;

    void __fastcall UpdateProgress(bool Progressing, int Progression);
    void __fastcall TurnLightOn(bool mculed);
    void __fastcall FPGAControl(bool fpgaload, bool fpgareset);


    TJvHidDevice *MyHidDev;
    int DevIndex;


    __fastcall TForm1(TComponent* Owner);

protected:


};
//----------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//----------------------------------------------------------------------------
#endif
