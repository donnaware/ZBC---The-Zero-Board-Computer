//---------------------------------------------------------------------------
#include <vcl.h>
#include <stdio.h>
#pragma hdrstop
#include "FPGASPIUnit1.h"
#include "HIDLoggerUnit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFPGASPIForm1 *FPGASPIForm1;
//---------------------------------------------------------------------------
__fastcall TFPGASPIForm1::TFPGASPIForm1(TComponent* Owner)  : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::StartMon(void)
{
    LoggerForm1->StartMonButton1->Click();
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::StopMon(void)
{
    LoggerForm1->StopMonButton1->Click();
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::UpDown1Click(TObject *Sender,TUDBtnType Button)
{
    int address = UpDown1->Position;
    EEAddrEdit1->Text = IntToHex(address, 2);
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::ReadReport(void)
{
    memset(Report, 0, sizeof(Report));
    Report[0] = 0;
    if(Form1->MyHidDev->OpenFile()) {
        SPIDialogMemo1->Lines->Add("Opened");
        unsigned BytesRead = 0;
        if(Form1->MyHidDev->ReadFile(Report, ReportSize+1, BytesRead)) SPIDialogMemo1->Lines->Add("Bytes Read: " + AnsiString(int(BytesRead)));
        else                                                           SPIDialogMemo1->Lines->Add("Read error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        AnsiString Tmp;
        for(int i=1; i< Form1->MyHidDev->Caps.InputReportByteLength; i++) {
            Tmp = Tmp + "0x" + IntToHex(int(Report[i]),2) + ", ";
            if(i > 7) break;
        }
        SPIDialogMemo1->Lines->Add(Tmp);
    }
    else {
        SPIDialogMemo1->Lines->Add("Open error, " + SysErrorMessage(GetLastError()));
    }
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::FPGA_SPI(byte Data)
{
    bool ret;
    StartMon();
    if(Form1->MyHidDev == NULL) {
        SPIDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0xB1;
        Report[2] = Data;

        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) SPIDialogMemo1->Lines->Add("Write Command sent");
        else    SPIDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        SPIDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) ReadReport();
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::FPGASPIButton1Click(TObject *Sender)
{
    int Data;
    sscanf(SPIDataEdit1->Text.c_str(), "%2x",&Data);
    FPGA_SPI(Data);
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::EnableFPGASPI(bool enable)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        SPIDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0xB2;
        if(enable) Report[2] = 0x01;
        else       Report[2] = 0x00;
        unsigned BytesWritten;
        bool ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) SPIDialogMemo1->Lines->Add("Write Command sent");
        else    SPIDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        SPIDialogMemo1->Lines->Add("Disconnected.");
    }
    StopMon();
}
//---------------------------------------------------------------------------
byte __fastcall TFPGASPIForm1::ReadEE(byte Address)
{
    int Data;
    StartMon();
    if(Form1->MyHidDev == NULL) {
        SPIDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return(0);
    }
    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x21;
        Report[2] = Address;
        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) SPIDialogMemo1->Lines->Add("Read Command sent");
        else    SPIDialogMemo1->Lines->Add("Writ ereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        SPIDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) {
        ReadReport();
        Data = Report[1];
    }
    StopMon();
    return(Data);
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::WriteEE(byte Address, byte Data)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        SPIDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x20;
        Report[2] = Address;
        Report[3] = Data;
        unsigned BytesWritten;
        bool ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) SPIDialogMemo1->Lines->Add("Write Command sent");
        else    SPIDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        SPIDialogMemo1->Lines->Add("Disconnected.");
    }
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::ReadEEButton1Click(TObject *Sender)
{
    int Address, Data;
    sscanf(EEAddrEdit1->Text.c_str(), "%2x",&Address);
    Data = ReadEE(Address);
    EEDataEdit1->Text = IntToHex(Data,2);
}
//---------------------------------------------------------------------------
void __fastcall TFPGASPIForm1::WriteEEButton1Click(TObject *Sender)
{
    int Address, Data;
    sscanf(EEAddrEdit1->Text.c_str(), "%2x",&Address);
    sscanf(EEDataEdit1->Text.c_str(), "%2x",&Data);
    WriteEE(Address, Data);
}
//---------------------------------------------------------------------------

