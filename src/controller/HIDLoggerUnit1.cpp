//---------------------------------------------------------------------------
#include <vcl.h>
#include <stdio.h>
#pragma hdrstop
#include "HIDLoggerUnit1.h"
#include "DOSeyUnit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TLoggerForm1 *LoggerForm1;
//---------------------------------------------------------------------------
__fastcall TLoggerForm1::TLoggerForm1(TComponent* Owner) : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TLoggerForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
    StopMonButton1Click(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TLoggerForm1::StartMonButton1Click(TObject *Sender)
{
    Form1->MyHidDev = NULL;
    Form1->JvHidDeviceController1->Enumerate();
    if(Form1->MyHidDev == NULL) {
        HidLoggerMemo1->Lines->Add("Could not find DOSey Target...");
        return;
    }

    if(Form1->MyHidDev->CheckOut()) {
        HidLoggerMemo1->Lines->Add("Checking out:");
        HidLoggerMemo1->Lines->Add("Vendor  = 0x" + IntToHex(Form1->MyHidDev->Attributes.VendorID,4));
        HidLoggerMemo1->Lines->Add("Product = 0x" + IntToHex(Form1->MyHidDev->Attributes.ProductID,4));
        HidLoggerMemo1->Lines->Add(Form1->MyHidDev->DeviceStrings[2]);
    }
    else {
        HidLoggerMemo1->Lines->Add("Check out failed");
    }
}
//---------------------------------------------------------------------------
void __fastcall TLoggerForm1::StopMonButton1Click(TObject *Sender)
{
    Form1->JvHidDeviceController1->CheckIn(Form1->MyHidDev);
    HidLoggerMemo1->Lines->Add("Device Checked back in.");
}
//---------------------------------------------------------------------------
void __fastcall TLoggerForm1::SendReportButton1Click(TObject *Sender)
{
    byte Report[64];
    memset(Report, 0, sizeof(Report));
    Report[0] = 0;

    AnsiString Tmp;
    int data;
    Tmp = Edit1->Text.SubString(3,2); sscanf(Tmp.c_str(),"%2x",&data);
    Report[1] = byte(data);
    Tmp = Edit2->Text.SubString(3,2); sscanf(Tmp.c_str(),"%2x",&data);
    Report[2] = byte(data);

    HidLoggerMemo1->Lines->Add("Rpt bytes: " + AnsiString(Form1->MyHidDev->Caps.OutputReportByteLength));

    if(Form1->MyHidDev->OpenFile()) {
        HidLoggerMemo1->Lines->Add("Opened");
        unsigned BytesWritten = 0;
        if(Form1->MyHidDev->WriteFile(Report, 41, BytesWritten)) HidLoggerMemo1->Lines->Add("Write bytes written: " + AnsiString(int(BytesWritten)));
        else                                                     HidLoggerMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
    }
    else {
        HidLoggerMemo1->Lines->Add("Open error, " + SysErrorMessage(GetLastError()));
    }
}
//---------------------------------------------------------------------------
void __fastcall TLoggerForm1::ReadHidButton1Click(TObject *Sender)
{
    byte Report[ReportSize+1];
    memset(Report, 0, sizeof(Report));
    Report[0] = 0;

    if(Form1->MyHidDev->OpenFile()) {
        HidLoggerMemo1->Lines->Add("Opened");
        unsigned BytesRead = 0;
        if(Form1->MyHidDev->ReadFile(Report, ReportSize+1, BytesRead)) HidLoggerMemo1->Lines->Add("Bytes Read: " + AnsiString(int(BytesRead)));
        else                                                           HidLoggerMemo1->Lines->Add("Read error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        AnsiString Tmp;
        for(int i=1; i< Form1->MyHidDev->Caps.InputReportByteLength; i++) {
            Tmp = Tmp + "0x" + IntToHex(int(Report[i]),2) + ", ";
        }
        HidLoggerMemo1->Lines->Add(Tmp);
    }
    else {
        HidLoggerMemo1->Lines->Add("Open error, " + SysErrorMessage(GetLastError()));
    }
}
//---------------------------------------------------------------------------
void __fastcall TLoggerForm1::CheckBox1Click(TObject *Sender)
{
    Timer2->Enabled = CheckBox1->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TLoggerForm1::Timer2Timer(TObject *Sender)
{
    ReadHidButton1Click(Sender);
}
//---------------------------------------------------------------------------


