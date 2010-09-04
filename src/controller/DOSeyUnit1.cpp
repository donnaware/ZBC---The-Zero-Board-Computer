//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//  DOSey Controller:                                    DOSeyController.CPP
//  This is the USB controller program for the DOSey-2000.
//  DonnaWare International LLP (C) 1958, All Rights Reserved
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include <vcl.h>
#include <stdio.h>
#include <wtypes.h>
#include <Clipbrd.hpp>
#pragma hdrstop
//----------------------------------------------------------------------------
#include "DOSeyUnit1.h"
#include "FlashTestUnit1.h"
#include "RTCUnit1.h"
#include "FPGASPIUnit1.h"
#include "HIDLoggerUnit1.h"
//----------------------------------------------------------------------------
#define DEBUGMODE 1                     // Set to 1 to comile in debug mode
//----------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "JvHidControllerClass"
//#pragma link "SHDocVw_OCX"
#pragma resource "*.dfm"
//----------------------------------------------------------------------------
TForm1 *Form1;
//----------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)  : TForm(Owner)
{
    Uploading   = false;            // We are not uploading anything yet
    Progress    = 0;                // So of course our progress is nothing
    ProgressMsg = "Ready";          // Default Progress Message
    MyHidDev    = NULL;             // No HID device instantiated
    DevIndex    = -1;
	PageControl1->ActivePage = TabSheet1;
}
//---------------------------------------------------------------------------
// Exit the program
//---------------------------------------------------------------------------
void __fastcall TForm1::TabSheet7Show(TObject *Sender)
{
    Update();
    Sleep(500);
    Close();
}
//---------------------------------------------------------------------------
// On Show Help
//---------------------------------------------------------------------------
void __fastcall TForm1::TabSheet5Show(TObject *Sender)
{
//     WideString url = Edit1->Text;
//    TVariantT <(int *)VARIANT> f; f = 0;
//    TVariantT <(wchar_t* )VARIANT> u;
//     WideString url = "E:\\Dev1\\DOS\\Zet\\ZetBoard\\rtl\\Controller\\HTML\\index.html";
//     u = url.c_bstr();
//     CppWebBrowser1->Navigate2(u, f);

     WideString url = ExtractFilePath(Application->ExeName) + Application->HelpFile;
     CppWebBrowser1->Navigate(url.c_bstr());
}
//---------------------------------------------------------------------------
// Exit the program
//----------------------------------------------------------------------------
void __fastcall TForm1::ToolButton9Click(TObject *Sender)
{
	PageControl1->ActivePage = TabSheet7;
}
//---------------------------------------------------------------------------
// Ye ole' about box
//---------------------------------------------------------------------------
void __fastcall TForm1::AboutImage1Click(TObject *Sender)
{
    MessageDlgPos("DOSey Configuritizer,\n DonnaWare International LLP\n(C)1958 All Rights Reserved",mtInformation, TMsgDlgButtons() << mbOK, 0, Left+60, Top+80);
}
//---------------------------------------------------------------------------
// Help Tab
//----------------------------------------------------------------------------
void __fastcall TForm1::ToolButton8Click(TObject *Sender)
{
	PageControl1->ActivePage = TabSheet5;
}
//---------------------------------------------------------------------------
// About Tab
//----------------------------------------------------------------------------
void __fastcall TForm1::ToolButton11Click(TObject *Sender)
{
	PageControl1->ActivePage = TabSheet6;
}
//---------------------------------------------------------------------------
// Show Tool Bar option
//----------------------------------------------------------------------------
void __fastcall TForm1::ShowToolBarCheckBox1Click(TObject *Sender)
{
	ToolBar1->Visible = ShowToolBarCheckBox1->Checked;
    if(ShowToolBarCheckBox1->Checked) Height = 360;
    else                              Height = 360 - ToolBar1->Height;
}
//---------------------------------------------------------------------------
// Show/Hider USBHID Logger Window
//---------------------------------------------------------------------------
void __fastcall TForm1::LoggerCheckBox1Click(TObject *Sender)
{
    if(LoggerCheckBox1->Checked) LoggerForm1->Show();
    else                         LoggerForm1->Hide();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UpdateProgress(bool Progressing, int Progression)
{
    Uploading = Progressing;
    Progress  = Progression;
    StatusBar1->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StatusBar1DrawPanel(TStatusBar *StatusBar, TStatusPanel *Panel, const TRect &Rect)
{
    if(Panel->Index == 1) {
        TCanvas *pCanvas = StatusBar->Canvas;
        AnsiString Tmp = ProgressMsg;
        int tx = Rect.left + 80;
        int ty = Rect.top  +  1;
        if(Uploading) {
            TRect l = (Rect);
            TRect r = (Rect);

            float w = Rect.Width();
            l.Right = l.Left + float(Progress)/100 * w;
            r.Left  = r.Right - (1 - float(Progress)/100) * w;

            pCanvas->Brush->Color = clNavy;
            pCanvas->Font->Color  = clYellow;
            pCanvas->TextRect(l, tx, ty, Tmp);

            pCanvas->Brush->Color = clBtnFace;
            pCanvas->Font->Color  = clNavy;
            pCanvas->TextRect(r, tx, ty, Tmp);
        }
        else {
            pCanvas->Brush->Color = clBtnFace;
            pCanvas->Font->Color  = clBlack;
            pCanvas->TextOut(tx, ty, Tmp);
        }
    }
}
//---------------------------------------------------------------------------
//  Set the Rbf File
//---------------------------------------------------------------------------
void __fastcall TForm1::SetRBFileBitBtn1Click(TObject *Sender)
{
    if(OpenRBFDialog1->Execute()) FGPARBFText1->Caption = OpenRBFDialog1->FileName;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SetFloppyFileBitBtn1Click(TObject *Sender)
{
    if(OpenIMGDialog1->Execute()) FloppyIMGText1->Caption = OpenIMGDialog1->FileName;
}
//---------------------------------------------------------------------------
// Turn MCU Test LED On and Off
//---------------------------------------------------------------------------
void __fastcall TForm1::MCULEDCheckBox1Click(TObject *Sender)
{
    TurnLightOn(MCULEDCheckBox1->Checked);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FPGALoadCheckBox1Click(TObject *Sender)
{
    FPGAControl(FPGALoadCheckBox1->Checked, FPGAResetCheckBox1->Checked);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FPGAResetCheckBox1Click(TObject *Sender)
{
    FPGAControl(FPGALoadCheckBox1->Checked, FPGAResetCheckBox1->Checked);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FloppySelCheckBox1Click(TObject *Sender)
{
    LoggerForm1->StartMonButton1->Click();
    if(MyHidDev == NULL) {
        LoggerForm1->HidLoggerMemo1->Lines->Add("Attempt to connect aborted.");
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    byte Report[256];
    if(MyHidDev->OpenFile()) {
        StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x0B;
        Report[2] = ~FloppySelCheckBox1->Checked;
        unsigned BytesWritten;
        if(MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten)) LoggerForm1->HidLoggerMemo1->Lines->Add("Floppy Command sent");
        else                                                        LoggerForm1->HidLoggerMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        MyHidDev->CloseFile();
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Disconnected.");
    }
    LoggerForm1->StopMonButton1->Click();
}
//---------------------------------------------------------------------------
// Check for enumeration of the DOSey
//---------------------------------------------------------------------------
void __fastcall TForm1::CheckDOSeyBitBtn1Click(TObject *Sender)
{
    DOSeyNotFoundText1->Visible = false;
    DOSeyFoundText1->Visible    = false;
    MyHidDev = NULL;
    JvHidDeviceController1->Enumerate();
    if(MyHidDev == NULL) DOSeyNotFoundText1->Visible = true;
    else                 DOSeyFoundText1->Visible    = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RTCTestBitBtn1Click(TObject *Sender)
{
    RTCForm1->Show();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ShowFPGATestBitBtn1Click(TObject *Sender)
{
    FPGASPIForm1->Show();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
//  HID Controller section
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Enumerate the USB endpoints
//---------------------------------------------------------------------------
bool __fastcall TForm1::JvHidDeviceController1Enumerate(TJvHidDevice *HidDev, const int Idx)
{
    if(!FilterMessagesCheckBox1->Checked) {
        LoggerForm1->HidLoggerMemo1->Lines->Add("idx= " + AnsiString(Idx));
        LoggerForm1->HidLoggerMemo1->Lines->Add("Vendor  = 0x" + IntToHex(HidDev->Attributes.VendorID,4));
        LoggerForm1->HidLoggerMemo1->Lines->Add("Product = 0x" + IntToHex(HidDev->Attributes.ProductID,4));
    }
    if((HidDev->Attributes.VendorID == 0x0461) && (HidDev->Attributes.ProductID == 0x0021)) {
        MyHidDev = HidDev;
        DevIndex = Idx;

        LoggerForm1->HidLoggerMemo1->Lines->Add("Found DOSey");
        LoggerForm1->HidLoggerMemo1->Lines->Add("Selecting:");
        LoggerForm1->HidLoggerMemo1->Lines->Add("Vendor  = 0x" + IntToHex(MyHidDev->Attributes.VendorID,4));
        LoggerForm1->HidLoggerMemo1->Lines->Add("Product = 0x" + IntToHex(MyHidDev->Attributes.ProductID,4));
        LoggerForm1->HidLoggerMemo1->Lines->Add(MyHidDev->DeviceStrings[2]);
    }
    return(true);
}
//---------------------------------------------------------------------------
// Send command to turn on the MCU test lamp
//---------------------------------------------------------------------------
void __fastcall TForm1::TurnLightOn(bool mculed)
{
    LoggerForm1->StartMonButton1->Click();
    if(MyHidDev == NULL) {
        LoggerForm1->HidLoggerMemo1->Lines->Add("Attempt to connect aborted.");
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    byte Report[256];
    if(MyHidDev->OpenFile()) {
        StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x09;
        if(mculed) Report[2] = 0x03;
        else       Report[2] = 0x00;
        unsigned BytesWritten;
        bool ret = MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) LoggerForm1->HidLoggerMemo1->Lines->Add("LED Command sent");
        else    LoggerForm1->HidLoggerMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        MyHidDev->CloseFile();
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Disconnected.");
    }
    LoggerForm1->StopMonButton1->Click();
}
//---------------------------------------------------------------------------
// Send command to turn on or off the FPGA nConfig line
//---------------------------------------------------------------------------
void __fastcall TForm1::FPGAControl(bool fpgaload, bool fpgareset)
{
    byte control = 0x00;
    if(fpgaload)  control |= 0x01;
    if(fpgareset) control |= 0x02;

    LoggerForm1->StartMonButton1->Click();
    if(MyHidDev == NULL) {
        LoggerForm1->HidLoggerMemo1->Lines->Add("Attempt to connect aborted.");
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    byte Report[256];
    if(MyHidDev->OpenFile()) {
        StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x0F;
        Report[2] = control;
        unsigned BytesWritten;
        if(MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten)) LoggerForm1->HidLoggerMemo1->Lines->Add("FPGA Command sent");
        else                                                        LoggerForm1->HidLoggerMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        MyHidDev->CloseFile();
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Disconnected.");
    }
    LoggerForm1->StopMonButton1->Click();
}
//---------------------------------------------------------------------------
// Configure FPGA
//---------------------------------------------------------------------------
void __fastcall TForm1::ConfigFPGABitBtn1Click(TObject *Sender)
{
    StatusBar1->Panels->Items[0]->Text = "Loading";
    StatusBar1->Panels->Items[1]->Text = FGPARBFText1->Caption;
    TMemoryStream *rbf = new TMemoryStream();
    rbf->LoadFromFile(FGPARBFText1->Caption);

    LoggerForm1->StartMonButton1Click(Sender);

    if(MyHidDev == NULL) {
        LoggerForm1->HidLoggerMemo1->Lines->Add("Aborting...");
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    StatusBar1->Panels->Items[0]->Text = "Connected";
    LoggerForm1->HidLoggerMemo1->Lines->Add("Uploading and RBF File to FPGA...");
    ProgressMsg =  "Uploading RBF";

    byte Report[256]; bool ret;
    int blksize = ReportSize;
    if(MyHidDev->OpenFile()) {
        UpdateProgress(true, 0);
        StatusBar1->Panels->Items[0]->Text = "Uploading";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Opened USB Connection");

        int Blocks = rbf->Size / blksize;          // 26095, blks= 3261
        int remainder = rbf->Size - (Blocks * blksize);  // rem = 7
        Blocks++;

        for(int t = 0; t < 64; t++) Report[t] = 0; // clear out the buffer
        Report[0] = 0;
        Report[1] = 0x10;                 // Start config Command
        Report[2] = byte(Blocks >>   8);  // Start config Command
        Report[3] = byte(Blocks & 0xFF);  // Start config Command
        Report[4] = byte(remainder);      // Start config Command

        unsigned BytesWritten;
        ret = MyHidDev->WriteFile(Report, blksize+1, BytesWritten);
        if(ret) LoggerForm1->HidLoggerMemo1->Lines->Add("Config Command sent");
        else    LoggerForm1->HidLoggerMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));

        LoggerForm1->HidLoggerMemo1->Lines->Add("Writing byte :");
        for(int i = 0; i < Blocks; i++) {    // do blocks
            if(i == (Blocks-1)) rbf->ReadBuffer(&Report[1],remainder);
            else                rbf->ReadBuffer(&Report[1],blksize);
            Report[0] = 0;
            ret = MyHidDev->WriteFile(Report, blksize+1, BytesWritten);
            if(ret) LoggerForm1->HidLoggerMemo1->Lines->Strings[LoggerForm1->HidLoggerMemo1->Lines->Count-1] = AnsiString(i);
            else    LoggerForm1->HidLoggerMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
            UpdateProgress(true, float(i)/float(Blocks) * 100);
        }
        MyHidDev->CloseFile();
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        ProgressMsg = "Ready";          // Default Progress Message
        UpdateProgress(false, 0);
    }
    else {
        LoggerForm1->HidLoggerMemo1->Lines->Add("Open error, " + SysErrorMessage(GetLastError()));
    }
    StatusBar1->Panels->Items[0]->Text = "Uploading Done";
    LoggerForm1->HidLoggerMemo1->Lines->Add("RBF Upload Completed");
    LoggerForm1->StopMonButton1Click(Sender);
    StatusBar1->Panels->Items[0]->Text = "Idle";
    delete rbf;

    FlashTestForm1->STUnInitialize();
    FPGASPIForm1->EnableFPGASPI(true);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FlashTestBitBtn1Click(TObject *Sender)
{
    FlashTestForm1->Show();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::EnableFlashCheckBox1Click(TObject *Sender)
{
    if(EnableFlashCheckBox1->Checked) FlashTestForm1->STInitialize();
    else                              FlashTestForm1->STUnInitialize();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::EnableFPGASPICheckBox1Click(TObject *Sender)
{
    if(EnableFPGASPICheckBox1->Checked) FPGASPIForm1->EnableFPGASPI(true);
    else                                FPGASPIForm1->EnableFPGASPI(false);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Upload the BIOS to Flash
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TForm1::BIOSToFLASHBitBtn1Click(TObject *Sender)
{
    StatusBar1->Panels->Items[0]->Text = "Loading BIOS to Flash";
    StatusBar1->Panels->Items[1]->Text = BIOSROMText1->Caption;
    FlashTestForm1->UploadBIOStoFlash();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Upload the RBF to Flash
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TForm1::RBFToFlashBitBtn1Click(TObject *Sender)
{
    StatusBar1->Panels->Items[0]->Text = "Loading RBF to Flash";
    StatusBar1->Panels->Items[1]->Text = FGPARBFText1->Caption;
    FlashTestForm1->UploadRBFtoFlash();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FlashToFPGABitBtn1Click(TObject *Sender)
{
    LoggerForm1->StartMonButton1->Click();
    if(MyHidDev == NULL) {
        LoggerForm1->HidLoggerMemo1->Lines->Add("Attempt to connect aborted.");
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    byte Report[256];
    if(MyHidDev->OpenFile()) {
        StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x11;
        unsigned BytesWritten;
        bool ret = MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) LoggerForm1->HidLoggerMemo1->Lines->Add("Boot from RBF Command sent");
        else    LoggerForm1->HidLoggerMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        MyHidDev->CloseFile();
        StatusBar1->Panels->Items[0]->Text = "Not Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Disconnected.");
    }
    LoggerForm1->StopMonButton1->Click();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Upload the Virtual Floppy IMG File to Flash
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TForm1::FLoppyToFlashBitBtn1Click(TObject *Sender)
{
    StatusBar1->Panels->Items[0]->Text = "Loading Floppy IMG to Flash";
    StatusBar1->Panels->Items[1]->Text = FloppyIMGText1->Caption;
    FlashTestForm1->UploadIMGtoFlash();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Set auto boot flag
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#define BOOT_TYPE     0x12      // Boot type indicator
//---------------------------------------------------------------------------
void __fastcall TForm1::AutoBootCheckBox1Click(TObject *Sender)
{
    int Data;
    if(AutoBootCheckBox1->Checked) Data = 0x01;
    else                           Data = 0x00;
    FPGASPIForm1->WriteEE(BOOT_TYPE, Data);
}
//---------------------------------------------------------------------------

