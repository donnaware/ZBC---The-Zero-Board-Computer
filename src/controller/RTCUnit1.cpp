//---------------------------------------------------------------------------
#include <vcl.h>
#include <stdio.h>
#pragma hdrstop
#include "RTCUnit1.h"
#include "HIDLoggerUnit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TRTCForm1 *RTCForm1;
//---------------------------------------------------------------------------
__fastcall TRTCForm1::TRTCForm1(TComponent* Owner)  : TForm(Owner)
{
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//               REAL TIME CLOCK CONTROL SECTION
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  RTC Registers
//---------------------------------------------------------------------------
#define RTC_Sec      0x00               		// Seconds
#define RTC_Min      0x01               		// Minutes Write
#define RTC_Hrs      0x02               		// Hours Write
#define RTC_Dat      0x03               		// Date Write
#define RTC_Mon      0x04               		// Month Write
#define RTC_Day      0x05               		// Day Write
#define RTC_Yrs      0x06               		// Years Write
#define RTC_Ctl      0x07               		// Control Write
#define RTC_Trc      0x08               		// Trickle Charge Control Write
#define RTC_Bst      0x1F               		// RAM Burst Control Write
#define RTC_RAMS     0x20               		// Scratch Pad Start
//---------------------------------------------------------------------------
#define   TrickleCmd     0xA0   //0b10100000   // Command pattern, all others disables
#define   ResistorNO     0x00   //0b00000000   // No resistor, disables trickle charger
#define   Resistor2K     0x01   //0b00000001   // 1K resistor inserted in line
#define   Resistor4K     0x02   //0b00000010   // 4K resistor inserted in line
#define   Resistor8K     0x03   //0b00000011   // 8K resistor inserted in line
#define   DiodeNone      0x00   //0b00000000   // No diodes, disables trickle charger
#define   DiodeOne       0x04   //0b00000100   // One diode is inserted into circuit
#define   DiodeTwo       0x08   //0b00001000   // Two diodes are inserted in circuit
#define   DiodeOff       0x0C   //0b00001100   // No diodes, disables trickle charger
#define   ThreeVolts     TrickleCmd|DiodeNone|Resistor8K // Set up for 3.3v supply & 3V battery
//----------------------------------------------------------------------------//
#define Alm_ENB      0x20			       		// Flag to enable/disable alrms
#define Alm_RHr      0x21         				// Sunrise Time Hours
#define Alm_RMn      0x22       	  			// Sunrise Time Minutes
#define Alm_SHr      0x23   	      			// Sunset  Time Hours
#define Alm_SMn      0x24	      	   			// Sunset  Time Minutes
//---------------------------------------------------------------------------
char *days[]    = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
int  resistor[] = {0,2,4,8};
void __fastcall TRTCForm1::DumpBuffer(void)
{
    DumpMemo1->Clear();
    AnsiString Out,Tmp;
    int i = 1;
    Out = Tmp.sprintf("%02x:",i-1);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + "        ";
    Out = Out + Tmp.sprintf("%02x:%02x:%02x",Report[3],Report[2],Report[1]);
    DumpMemo1->Lines->Add(Out);

    Out = Tmp.sprintf("%02x:",i-1);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + "     ";
    Out = Out + Tmp.sprintf("%s %02x/%02x/%02x",days[Report[6]-1],Report[5],Report[4],Report[7]);
    DumpMemo1->Lines->Add(Out);

    Out = Tmp.sprintf("%02x:",i-1);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
    Out = Out + "              Control Byte";
    DumpMemo1->Lines->Add(Out);

    Out = Tmp.sprintf("%02x:",i-1);
    Out = Out + Tmp.sprintf(" %02x",Report[i++]);
//    Out = Out + "              Trickle Charge";
    int restr =  Report[9]     & 0x03;
    int diode = (Report[9]>>2) & 0x03;
//  Out = Out + "              ";
    Out = Out + "    (trickle) ";
    Out = Out + Tmp.sprintf("%d diode + %dK Resistor",diode, resistor[restr]);

    DumpMemo1->Lines->Add(Out);
    DumpMemo1->Lines->Add("");
    DumpMemo1->Lines->Add("Ram:");

    int cols = 8;
    int rows = 4;
    for(int l=0; l < rows; l++) {
        Out = Tmp.sprintf("%02x:",i-1);
        for(int x=0; x < cols; x++) Out = Out + Tmp.sprintf(" %02x",Report[i++]);
        Out = Out + "   ";
        for(int x=0; x < cols; x++) {
            byte ch = Report[i-cols + x];
            Tmp = "";
            if(ch < 0x20 || ch > 0x7f) ch = '.';
            Out = Out + Tmp.sprintf("%c",ch);
        }
        DumpMemo1->Lines->Add(Out);
    }
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::StartMon(void)
{
    LoggerForm1->StartMonButton1->Click();
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::StopMon(void)
{
    LoggerForm1->StopMonButton1->Click();
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::UpDown1Click(TObject *Sender, TUDBtnType Button)
{
    int address = UpDown1->Position;
    RTCAddrEdit1->Text = IntToHex(address, 2);
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::ReadReport(void)
{
    memset(Report, 0, sizeof(Report));
    Report[0] = 0;

    if(Form1->MyHidDev->OpenFile()) {
        RTCDialogMemo1->Lines->Add("Opened");
        unsigned BytesRead = 0;
        if(Form1->MyHidDev->ReadFile(Report, ReportSize+1, BytesRead)) RTCDialogMemo1->Lines->Add("Bytes Read: " + AnsiString(int(BytesRead)));
        else                                                           RTCDialogMemo1->Lines->Add("Read error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        AnsiString Tmp;
        for(int i=1; i< Form1->MyHidDev->Caps.InputReportByteLength; i++) {
            Tmp = Tmp + "0x" + IntToHex(int(Report[i]),2) + ", ";
        }
        DumpBuffer();
    }
    else {
        RTCDialogMemo1->Lines->Add("Open error, " + SysErrorMessage(GetLastError()));
    }
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::RTCWrite1Byte(byte address, byte data)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        RTCDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0xA3;
        Report[2] = address;
        Report[3] = data;

        unsigned BytesWritten;
        bool ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) RTCDialogMemo1->Lines->Add("Write Command sent");
        else    RTCDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        RTCDialogMemo1->Lines->Add("Disconnected.");
    }
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::WriteRTCButton1Click(TObject *Sender)
{
    int Address, Data;
    sscanf(RTCAddrEdit1->Text.c_str(), "%2x",&Address);
    sscanf(RTCDataEdit1->Text.c_str(), "%2x",&Data);
    RTCWrite1Byte(Address, Data);
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::SetTrickleButton1Click(TObject *Sender)
{
    RTCWrite1Byte(RTC_Ctl,0x00);             // Write protect off
    RTCWrite1Byte(RTC_Trc,ThreeVolts);
    RTCWrite1Byte(RTC_Ctl,0x80);             // Write Protect on
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::ReadAllRTCButton1Click(TObject *Sender)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        RTCDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0xA1;

        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) RTCDialogMemo1->Lines->Add("Read Command sent");
        else    RTCDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        RTCDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) {
        ReadReport();
    }
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TRTCForm1::RTCSyncButton1Click(TObject *Sender)
{
    RTCDialogMemo1->Lines->Add("Syncing PC clock to RTC");
    RTCWrite1Byte(RTC_Ctl,0x00);    // Write protect off
    RTCWrite1Byte(RTC_Sec,0x80);    // Stop clock to set it

    TDateTime curtime;
    TDateTime curdate;
    TDateTime Tmp;

    curtime = Tmp.CurrentTime();
    curdate = Tmp.CurrentDate();

    AnsiString ct = curtime.FormatString("hh:mm:ss");
    AnsiString cd = curdate.FormatString("mm/dd/yy");

    byte yy =  (cd[7]-'0')<<4  | (cd[8]-'0');
    byte dd =  (cd[4]-'0')<<4  | (cd[5]-'0');
    byte mm =  (cd[1]-'0')<<4  | (cd[2]-'0');
    byte ss =  (ct[7]-'0')<<4  | (ct[8]-'0');
    byte mn =  (ct[4]-'0')<<4  | (ct[5]-'0');
    byte hh =  (ct[1]-'0')<<4  | (ct[2]-'0');
    byte ww =   curdate.DayOfWeek();

    RTCWrite1Byte(RTC_Yrs, yy);    // Year
    RTCWrite1Byte(RTC_Day, ww);    // Day of week
    RTCWrite1Byte(RTC_Mon, mm);    // Month
    RTCWrite1Byte(RTC_Dat, dd);    // Date

    RTCWrite1Byte(RTC_Hrs, hh);    // Hours
    RTCWrite1Byte(RTC_Min, mn);    // Minutes
    RTCWrite1Byte(RTC_Sec, ss);    // Seconds

    RTCWrite1Byte(RTC_Ctl,0x80);   // Write Protect on
}
//---------------------------------------------------------------------------

