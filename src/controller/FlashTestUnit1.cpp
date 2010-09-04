//---------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Flash RAM Read Write Routines
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <vcl.h>
#include <stdio.h>
#pragma hdrstop
//---------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Flash RAM Memory Map 32Mb (4Mbyte) Chip:
//------------------------------------------------------------------------------
// REMEMBER - Actual Virtual Floppy File size = 1,474,560 = 0x16_8000
//
//     Start      End      Start       End      File       Hex   64k
//   Address   Address   Address   Address      Size      Size Blcks Description
// --------- --------- --------- --------- --------- --------- ----- -----------
//         0   131,071 0x00_0000 0x01_FFFF   131,071 0x02_0000   2.0 BIOS ROM
//   131,072 1,605,631 0x02_0000 0x18_7FFF 1,474,560 0x16_8000  22.5 Floppy
// 1,605,632 1,638,399 0x18_8000 0x18_FFFF    32,768 0x00_8000    .5 Round to 64k block
// 1,571,072 2,097,151 0x19_0000 0x1F_FFFF   458,752 0x07_0000   7.0 RBF, actual size varies
//
// 2,097,152 2,228,223 0x20_0000 0x21_FFFF   131,071 0x02_0000   2.0 BIOS ROM#2
// 2,228,223 3,702,783 0x22_0000 0x38_7FFF 1,474,560 0x16_8000  22.5 Floppy #2
// 3,702,784 3,735,551 0x38_8000 0x38_FFFF    32,768 0x00_8000    .5 Round to 64k block
// 3,735,552 4,097,151 0x39_0000 0x3F_FFFF   458,752 0x07_0000   7.0 RBF, actual size varies
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define FLASH_S_1_BIOS      0x000000        // Start address for BIOS #1
#define FLASH_E_1_BIOS      0x020000        // End address for BIOS #1 (plus 1)
#define FLASH_S_1_FLOPPY    0x020000        // Start address for Floppy #1
#define FLASH_E_1_FLOPPY    0x188000        // End address for Floppy #1 (plus 1)
#define FLASH_S_1_RBF       0x190000        // Start address for RBF #1
#define FLASH_E_1_RBF       0x200000        // End address for RBF #1 (plus 1)

#define FLASH_S_2_BIOS      0x200000        // Start address for BIOS #2
#define FLASH_E_2_BIOS      0x220000        // End address for BIOS #2 (plus 1)
#define FLASH_S_2_FLOPPY    0x220000        // Start address for Floppy #2
#define FLASH_E_2_FLOPPY    0x388000        // End address for Floppy #2 (plus 1)
#define FLASH_S_2_RBF       0x390000        // Start address for RBF #2
#define FLASH_E_2_RBF       0x400000        // End address for RBF #2 (plus 1)

#define FLASH_SZ_BIOS       0x020000        // File size of BIOS, exact
#define FLASH_SZ_FLOPPY     0x168000        // File size of Floppy, exact
#define FLASH_SZ_RBF        0x070000        // File size of RBF, Less than

//---------------------------------------------------------------------------
//------------------------------------------------------------------------------
// EEPROM Memory Map :
// PIC EEPROM is non-volital memory used store various parameters and 
// prescriptions for the boot up and control of ZBC.
//------------------------------------------------------------------------------
//   Start    End  Size Description
// ------- ------- ---- ------------------------------------------------------
//    0x00    0x02    3 Start address of Bios File
//    0x03    0x05    3 End   address of Bios File
//    0x06    0x08    3 Start address of Floppy File
//    0x09    0x0B    3 End   address of Floppy File
//    0x0C    0x0E    3 Start address of RBF File
//    0x0F    0x11    3 End   address of RBF File
//
//    0x12    0x12    1 Boot type, 0= No boot (debug), 1=HD, 2= Floppy, 
//------------------------------------------------------------------------------
#define EEPROM_S_ADDR_BIOS   0x00  // Start address of Bios File pointer in EEPROM
#define EEPROM_E_ADDR_BIOS   0x03  // End   address of Bios File pointer in EEPROM
#define EEPROM_S_ADDR_FLOPPY 0x06  // Start address of Floppy File pointer in EEPROM
#define EEPROM_E_ADDR_FLOPPY 0x09  // End   address of Floppy File pointer in EEPROM
#define EEPROM_S_ADDR_RBF    0x0C  // Start address of RBF File pointer in EEPROM
#define EEPROM_E_ADDR_RBF    0x0F  // End   address of RBF File pointer in EEPROM

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "FlashTestUnit1.h"
#include "HIDLoggerUnit1.h"
#include "FPGASPIUnit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
TFlashTestForm1 *FlashTestForm1;
//---------------------------------------------------------------------------
__fastcall TFlashTestForm1::TFlashTestForm1(TComponent* Owner) : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::UpDown1Click(TObject *Sender, TUDBtnType Button)
{
    int Address;
    sscanf(BlockEdit1->Text.c_str(),"%6x",&Address);
    if(Button == btNext) Address += 64;
    if(Button == btPrev) Address -= 64;
    BlockEdit1->Text = IntToHex(Address, 6);
}
//----------------------------------------------------------------------------
void __fastcall TFlashTestForm1::DumpBuffer(void)
{
    DumpMemo1->Clear();
    AnsiString Out,Tmp;
    int i = 1;
    int cols = 8;
    int rows = 8;
    for(int l=0; l < rows; l++) {
        Out = Tmp.sprintf("%06x:",i);
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
void __fastcall TFlashTestForm1::StartMon(void)
{
    LoggerForm1->StartMonButton1->Click();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::StopMon(void)
{
    LoggerForm1->StopMonButton1->Click();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::ReadReport(void)
{
    memset(Report, 0, sizeof(Report));
    Report[0] = 0;

    if(Form1->MyHidDev->OpenFile()) {
        STDialogMemo1->Lines->Add("Opened");
        unsigned BytesRead = 0;
        if(Form1->MyHidDev->ReadFile(Report, ReportSize+1, BytesRead)) STDialogMemo1->Lines->Add("Bytes Read: " + AnsiString(int(BytesRead)));
        else                                                           STDialogMemo1->Lines->Add("Read error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        AnsiString Tmp;
        for(int i=1; i< Form1->MyHidDev->Caps.InputReportByteLength; i++) {
            Tmp = Tmp + "0x" + IntToHex(int(Report[i]),2) + ", ";
        }
//        STDialogMemo1->Lines->Add(Tmp);
        DumpBuffer();
    }
    else {
        STDialogMemo1->Lines->Add("Open error, " + SysErrorMessage(GetLastError()));
    }
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::WriteStatButton1Click(TObject *Sender)
{
    StartMon();

    int Data;
    sscanf(FlashDataEdit1->Text.c_str(), "%2x",&Data);

    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }

    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x95;
        Report[2] = Data;
        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Write Status Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }

    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::STInitialize(void)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }

    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x90;
        Report[2] = 0x00;
        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Intitialize Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) ReadReport();
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::STUnInitialize(void)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x9F;
        Report[2] = 0x00;
        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Un-Intitialize Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::STInitButton1Click(TObject *Sender)
{
    STInitialize();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::GetStatusButton1Click(TObject *Sender)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }

    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x91;
        Report[2] = 0x00;
        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Get Status Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) ReadReport();
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::EraseButton1Click(TObject *Sender)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }
    int Address;
    sscanf(BlockEdit1->Text.c_str(),"%6x",&Address);
    Erase64KSector(Address);
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::ReadSTButton1Click(TObject *Sender)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }

    int Address;
    sscanf(BlockEdit1->Text.c_str(),"%6x",&Address);

    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x93;
        Report[2] = (Address >> 24) & 0xFF;
        Report[3] = (Address >> 16) & 0xFF;
        Report[4] = (Address >>  8) & 0xFF;
        Report[5] = (Address      ) & 0xFF;

        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Read Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) {
        ReadReport();
    }
    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::WriteSTButton1Click(TObject *Sender)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }

    int Address, Data;
    sscanf(BlockEdit1->Text.c_str(),     "%6x",&Address);
    sscanf(FlashDataEdit1->Text.c_str(), "%2x",&Data);

    /*
    TMemoryStream *rom = new TMemoryStream();
    rom->LoadFromFile(Form1->BIOSROMText1->Caption);
    int filesize = rom->Size;
    if(filesize != 131072) {
        STDialogMemo1->Lines->Add("Wrong Bios File");
        return;
    }
    rom->Position = Address;
    rom->Read(Buffer, ReportSize);
    delete rom;
    */
    
    int n = 0;
    for(int i=0; i<32; i++) Buffer[n++] = Data;   // fill in some phoney data
    for(int i=0; i<32; i++) Buffer[n++] = i;      // fill in some phoney data

    Write64Bytes(Address);

    StopMon();
}
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::ChipIDButton1Click(TObject *Sender)
{
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }

    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x96;
        Report[2] = 0x00;
        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Get Status Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) ReadReport();
    StopMon();
}
//---------------------------------------------------------------------------
// Set Status to enable writing
//---------------------------------------------------------------------------
bool __fastcall TFlashTestForm1::EnableWriting(void)
{
    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x95;
        Report[2] = 0x00;       // Allow Writing
        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Write Status Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    return(ret);
}

//---------------------------------------------------------------------------
// Erase 1st sector
//---------------------------------------------------------------------------
bool __fastcall TFlashTestForm1::Erase64KSector(int Address)
{
    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x92;
        Report[2] = (Address >> 24) & 0xFF;
        Report[3] = (Address >> 16) & 0xFF;
        Report[4] = (Address >>  8) & 0xFF;
        Report[5] = (Address      ) & 0xFF;

        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Erase Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    return(ret);
}
//---------------------------------------------------------------------------
// Write a 64 byte buffer at address
//---------------------------------------------------------------------------
bool __fastcall TFlashTestForm1::Write64Bytes(int Address)
{
    bool ret;
    if(Form1->MyHidDev->OpenFile()) {
        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");
        Report[0] = 0;
        Report[1] = 0x94;
        Report[2] = (Address >> 24) & 0xFF;
        Report[3] = (Address >> 16) & 0xFF;
        Report[4] = (Address >>  8) & 0xFF;
        Report[5] = (Address      ) & 0xFF;

        unsigned BytesWritten;
        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Write Command sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
        if(!ret) return(ret);

        Sleep(50);

        Form1->StatusBar1->Panels->Items[0]->Text = "Connected";
        LoggerForm1->HidLoggerMemo1->Lines->Add("Connected.");

        Report[0] = 0;
        int n = 1;
        for(int i=0; i<ReportSize; i++) Report[n++] = Buffer[i];

        ret = Form1->MyHidDev->WriteFile(Report, ReportSize+1, BytesWritten);
        if(ret) STDialogMemo1->Lines->Add("Write Data sent");
        else    STDialogMemo1->Lines->Add("Writereport error, " + SysErrorMessage(GetLastError()));
        Form1->MyHidDev->CloseFile();
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        STDialogMemo1->Lines->Add("Disconnected.");
    }
    if(ret) ReadReport();
    return(ret);
}
//---------------------------------------------------------------------------
// Check for Blank Block
//---------------------------------------------------------------------------
bool __fastcall TFlashTestForm1::CheckNotBlank(void)
{
    bool ret = false;
    for(int i=0; i<ReportSize; i++) {
        if(Buffer[i] != 0xFF) ret = true;
    }
    return(ret);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Write Bios To Flash Ram
//
//     Start      End      Start       End      File       Hex   64k
//   Address   Address   Address   Address      Size      Size Blcks Description
// --------- --------- --------- --------- --------- --------- ----- -----------
//         0   131,071 0x00_0000 0x01_FFFF   131,071 0x02_0000   2.0 BIOS ROM
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::UploadBIOStoFlash(void)
{
    //-----------------------------------------------------------------------
    // Make PIC MCU the SPI Master
    //-----------------------------------------------------------------------
    Form1->EnableFPGASPICheckBox1->Checked = false;
    Sleep(100);
    Form1->EnableFlashCheckBox1->Checked   = true;
    Sleep(100);

    //-----------------------------------------------------------------------
    // Load Bios Rom file into memory
    //-----------------------------------------------------------------------
    TMemoryStream *rom = new TMemoryStream();
    rom->LoadFromFile(Form1->BIOSROMText1->Caption);
    int filesize = rom->Size;
    if(filesize != FLASH_SZ_BIOS) {
        STDialogMemo1->Lines->Add("Wrong Bios File");
        delete rom;
        return;
    }
    rom->Position = 0;

    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        return;
    }

    bool ret;

    //-----------------------------------------------------------------------
    // Erase first Sector
    //-----------------------------------------------------------------------
    ret = EnableWriting();
    if(!ret) {
        STDialogMemo1->Lines->Add("BIOS Error enabling writing ");
        delete rom;
        StopMon();
        return;
    }

    //-----------------------------------------------------------------------
    // Erase first Sector
    //-----------------------------------------------------------------------
    int Address = FLASH_S_1_BIOS;
    ret = Erase64KSector(Address);    // Erase first sector
    if(!ret) {
        STDialogMemo1->Lines->Add("BIOS Error erasing sector");
        delete rom;
        StopMon();
        return;
    }
    Sleep(50);
    //-----------------------------------------------------------------------
    // Erase Second Sector
    //-----------------------------------------------------------------------
    Address = FLASH_S_1_BIOS + 0x010000;
    ret = Erase64KSector(Address);    // Erase second sector
    if(!ret) {
        STDialogMemo1->Lines->Add("BIOS Error erasing sector");
        delete rom;
        StopMon();
        return;
    }
    Sleep(50);

    //-----------------------------------------------------------------------
    // Start programming
    //-----------------------------------------------------------------------
    Form1->ProgressMsg =  "Uploading BIOS";
    Form1->UpdateProgress(true, 0);
    Address = FLASH_S_1_BIOS;
    int numBlocks = filesize / ReportSize;
    for(int i = 0; i < numBlocks; i++) {
        rom->Read(Buffer, ReportSize);
        if(CheckNotBlank()) {
            ret = Write64Bytes(Address);
            if(!ret) break;
        }
        Address += ReportSize;
        Form1->UpdateProgress(true, float(i)/float(numBlocks) * 100);
    }
    Form1->UpdateProgress(false, 0);

    //-----------------------------------------------------------------------
    // Flash Programing completed
    //-----------------------------------------------------------------------
    STDialogMemo1->Lines->Add("BIOS Flash programming completed");
    delete rom;

    //-----------------------------------------------------------------------
    // Store start and end addresses into EEPROM
    //-----------------------------------------------------------------------
    STDialogMemo1->Lines->Add("Storing BIOS Pointer Addresses in EEPROM");
    byte EE_address;
    int start = FLASH_S_1_BIOS;
    int end   = start +  filesize;

    EE_address = EEPROM_S_ADDR_BIOS;
    FPGASPIForm1->WriteEE(EE_address++, ((start >> 16) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address++, ((start >>  8) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address  , ((start      ) & 0xFF));

    EE_address = EEPROM_E_ADDR_BIOS;
    FPGASPIForm1->WriteEE(EE_address++, ((end   >> 16) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address++, ((end   >>  8) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address  , ((end        ) & 0xFF));

    //-----------------------------------------------------------------------
    StopMon();

    //-----------------------------------------------------------------------
    // Make PIC MCU the SPI Slave
    //-----------------------------------------------------------------------
    Form1->EnableFlashCheckBox1->Checked   = false;
    Sleep(100);
    Form1->EnableFPGASPICheckBox1->Checked = true;
    Sleep(100);

    STDialogMemo1->Lines->Add("BIOS Flash programming completed");
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Write RBF  To Flash Ram
//
//     Start      End      Start       End      File       Hex   64k
//   Address   Address   Address   Address      Size      Size Blcks Description
// --------- --------- --------- --------- --------- --------- ----- -----------
// 1,571,072 2,097,151 0x19_0000 0x1F_FFFF   458,752 0x07_0000   7.0 RBF, actual size varies
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::UploadRBFtoFlash(void)
{
    //-----------------------------------------------------------------------
    // Make PIC MCU the SPI Master
    //-----------------------------------------------------------------------
    Form1->EnableFPGASPICheckBox1->Checked = false;
    Sleep(100);
    Form1->EnableFlashCheckBox1->Checked   = true;
    Sleep(100);

    //-----------------------------------------------------------------------
    // Load RBF Rom file into memory
    //-----------------------------------------------------------------------
    TMemoryStream *rbf = new TMemoryStream();
    rbf->LoadFromFile(Form1->FGPARBFText1->Caption);
    int filesize = rbf->Size;
    if(filesize > FLASH_SZ_RBF) {
        STDialogMemo1->Lines->Add("RBF File too large for allocated space, expand space");
        delete rbf;
        return;
    }
    rbf->Position = 0;
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        delete rbf;
        return;
    }
    bool ret;

    //-----------------------------------------------------------------------
    // Enable Writing to the Flash
    //-----------------------------------------------------------------------
    ret = EnableWriting();
    if(!ret) {
        STDialogMemo1->Lines->Add("RBF Error enabling writing ");
        delete rbf;
        StopMon();
        return;
    }

    //-----------------------------------------------------------------------
    // Erase all eight Sectors
    //-----------------------------------------------------------------------
    int Address = FLASH_S_1_RBF;             // Starting address of first sector
    for(int i=0; i<8; i++) {
        ret = Erase64KSector(Address);    // Erase sector at Address
        if(!ret)  break;                  // unspecifed error occured
        Sleep(50);                  // wait for that block to erase (spec says 18ms)
        Address += 0x10000;          // fo next 64k block
    }
    if(!ret) {                          // unspecifed error occured
        STDialogMemo1->Lines->Add("RBF Error erasing sector");
        delete rbf;
        StopMon();
        return;
    }

    //-----------------------------------------------------------------------
    // Start programming
    //-----------------------------------------------------------------------
    Form1->ProgressMsg =  "Uploading and Programming RBF";
    Form1->UpdateProgress(true, 0);
    Address = FLASH_S_1_RBF;                        // Starting address
    int numBlocks = filesize/ReportSize;
    int remainder = filesize - (numBlocks*ReportSize);
    for(int i = 0; i < numBlocks; i++) {
        rbf->Read(Buffer, ReportSize);
        ret = Write64Bytes(Address);
        if(!ret) break;
        Address += ReportSize;
        Form1->UpdateProgress(true, float(i)/float(numBlocks) * 100);
    }
    if(remainder) {             // If there is a remainder (almost always)
        rbf->Read(Buffer, remainder);
        ret = Write64Bytes(Address);
    }
    Form1->UpdateProgress(false, 0);

    //-----------------------------------------------------------------------
    // Flash Programing completed
    //-----------------------------------------------------------------------
    STDialogMemo1->Lines->Add("RBF Flash programming completed");
    delete rbf;

    //-----------------------------------------------------------------------
    // Store start and end addresses into EEPROM
    //-----------------------------------------------------------------------
    STDialogMemo1->Lines->Add("Storing RBF Pointer Addresses in EEPROM");
    byte EE_address;
    int start = FLASH_S_1_RBF;
    int end   = start +  filesize;

    EE_address = EEPROM_S_ADDR_RBF;
    FPGASPIForm1->WriteEE(EE_address++, ((start >> 16) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address++, ((start >>  8) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address  , ((start      ) & 0xFF));

    EE_address = EEPROM_E_ADDR_RBF;
    FPGASPIForm1->WriteEE(EE_address++, ((end   >> 16) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address++, ((end   >>  8) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address  , ((end        ) & 0xFF));

    //-----------------------------------------------------------------------
    StopMon();

    //-----------------------------------------------------------------------
    // Make PIC MCU the SPI Slave
    //-----------------------------------------------------------------------
    Form1->EnableFlashCheckBox1->Checked   = false;
    Sleep(100);
    Form1->EnableFPGASPICheckBox1->Checked = true;
    Sleep(100);

    STDialogMemo1->Lines->Add("All RBF Programming tasks completed.");
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Write Floppy IMG To Flash Ram
// REMEMBER - Actual Virtual Floppy File size = 1,474,560 = 0x168000
//
//     Start      End      Start       End      File    Actual
//   Address   Address   Address   Address     Space      Size Comment
// --------- --------- --------- --------- --------- --------- -----------------
//   131,072 1,571,071 0x02_0000 0x17_F8FF 1,440,000 1,440,000 Virtual Floppy
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TFlashTestForm1::UploadIMGtoFlash(void)
{
    //-----------------------------------------------------------------------
    // Make PIC MCU the SPI Master
    //-----------------------------------------------------------------------
    Form1->EnableFPGASPICheckBox1->Checked = false;
    Sleep(100);
    Form1->EnableFlashCheckBox1->Checked   = true;
    Sleep(100);

    //-----------------------------------------------------------------------
    // Load IMG file into memory
    //-----------------------------------------------------------------------
    TMemoryStream *img = new TMemoryStream();
    img ->LoadFromFile(Form1->FloppyIMGText1->Caption);
    int filesize = img->Size;
    if(filesize != FLASH_SZ_FLOPPY) {
        STDialogMemo1->Lines->Add("Not corrent Floppy IMG File, wrong size");
        delete img;
        return;
    }
    img->Position = 0;
    StartMon();
    if(Form1->MyHidDev == NULL) {
        STDialogMemo1->Lines->Add("Attempt to connect aborted.");
        Form1->StatusBar1->Panels->Items[0]->Text = "Not Connected";
        delete img;
        return;
    }
    bool ret;

    //-----------------------------------------------------------------------
    // Enable Writing to the Flash
    //-----------------------------------------------------------------------
    ret = EnableWriting();
    if(!ret) {
        STDialogMemo1->Lines->Add("Floppy IMG FILE Error enabling writing ");
        delete img;
        StopMon();
        return;
    }

    //-----------------------------------------------------------------------
    // Erase all Sectors
    //-----------------------------------------------------------------------
    int Address = FLASH_S_1_FLOPPY;         // Starting address of first sector
    int Sectors = FLASH_SZ_FLOPPY/0x10000;  // Compute number of sectors to erase
    if(FLASH_SZ_FLOPPY%0x10000) Sectors++;
    STDialogMemo1->Lines->Add("Erasing " + AnsiString(Sectors) + " Sectors");

    for(int i=0; i<23; i++) {
        ret = Erase64KSector(Address);    // Erase sector at Address
        if(!ret)  break;                  // unspecifed error occured
        Sleep(50);                        // wait for that block to erase (spec says 18ms)
        Address += 0x10000;               // for next 64k block
    }
    if(!ret) {                            // unspecifed error occured
        STDialogMemo1->Lines->Add("IMG FILE Error erasing sector");
        delete img;
        StopMon();
        return;
    }

    //-----------------------------------------------------------------------
    // Start programming
    //-----------------------------------------------------------------------
    Form1->ProgressMsg =  "Uploading IMG";
    Form1->UpdateProgress(true, 0);
    Address = FLASH_S_1_FLOPPY;                        // Starting address
    int numBlocks = filesize/ReportSize;
    int remainder = filesize - (numBlocks*ReportSize);
    for(int i = 0; i < numBlocks; i++) {
        img->Read(Buffer, ReportSize);
        if(CheckNotBlank()) {
            ret = Write64Bytes(Address);
            if(!ret) break;
        }
        Address += ReportSize;
        Form1->UpdateProgress(true, float(i)/float(numBlocks) * 100);
    }
    if(remainder) {             // If there is a remainder (almost always)
        img->Read(Buffer, remainder);
        ret = Write64Bytes(Address);
    }
    Form1->UpdateProgress(false, 0);

    //-----------------------------------------------------------------------
    // Flash Programing completed
    //-----------------------------------------------------------------------
    STDialogMemo1->Lines->Add("FLOPPY IMG Flash programming completed");
    delete img;

    //-----------------------------------------------------------------------
    // Store start and end addresses into EEPROM
    //-----------------------------------------------------------------------
    STDialogMemo1->Lines->Add("Storing Floppy IMG Pointer Addresses in EEPROM");
    byte EE_address;
    int start = FLASH_S_1_FLOPPY;
    int end   = start +  filesize;

    EE_address = EEPROM_S_ADDR_FLOPPY;
    FPGASPIForm1->WriteEE(EE_address++, ((start >> 16) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address++, ((start >>  8) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address  , ((start      ) & 0xFF));

    EE_address = EEPROM_E_ADDR_FLOPPY;
    FPGASPIForm1->WriteEE(EE_address++, ((end   >> 16) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address++, ((end   >>  8) & 0xFF));
    FPGASPIForm1->WriteEE(EE_address  , ((end        ) & 0xFF));

    //-----------------------------------------------------------------------
    StopMon();

    //-----------------------------------------------------------------------
    // Make PIC MCU the SPI Slave
    //-----------------------------------------------------------------------
    Form1->EnableFlashCheckBox1->Checked   = false;
    Sleep(100);
    Form1->EnableFPGASPICheckBox1->Checked = true;
    Sleep(100);

    STDialogMemo1->Lines->Add("All Floppy IMG Programming tasks completed.");
}
//---------------------------------------------------------------------------


