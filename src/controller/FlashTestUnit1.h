//---------------------------------------------------------------------------
#ifndef FlashTestUnit1H
#define FlashTestUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
#include "DOSeyUnit1.h"
//---------------------------------------------------------------------------
class TFlashTestForm1 : public TForm
{
__published:	// IDE-managed Components
    TPanel *Panel24;
    TPanel *Panel17;
    TMemo *DumpMemo1;
    TPanel *Panel18;
    TLabel *Label43;
    TPanel *Panel16;
    TLabel *Label36;
    TLabel *Label42;
    TButton *STInitButton1;
    TButton *GetStatusButton1;
    TButton *EraseButton1;
    TButton *ReadSTButton1;
    TButton *WriteSTButton1;
    TButton *ChipIDButton1;
    TEdit *BlockEdit1;
    TEdit *FlashDataEdit1;
    TPanel *Panel23;
    TLabel *Label30;
    TMemo *STDialogMemo1;
    TLabel *Label35;
    TSplitter *Splitter1;
    TButton *WriteStatButton1;
    TUpDown *UpDown1;
    void __fastcall STInitButton1Click(TObject *Sender);
    void __fastcall GetStatusButton1Click(TObject *Sender);
    void __fastcall WriteStatButton1Click(TObject *Sender);
    void __fastcall EraseButton1Click(TObject *Sender);
    void __fastcall ReadSTButton1Click(TObject *Sender);
    void __fastcall WriteSTButton1Click(TObject *Sender);
    void __fastcall UpDown1Click(TObject *Sender, TUDBtnType Button);
    void __fastcall ChipIDButton1Click(TObject *Sender);

private:	// User declarations

    int block;
    int address;
    byte Report[ReportSize+10];
    byte Buffer[ReportSize+10];

    void __fastcall DumpBuffer(void);
    void __fastcall StartMon(void);
    void __fastcall StopMon(void);
    void __fastcall ReadReport(void);

public:		// User declarations

    void __fastcall STInitialize(void);
    void __fastcall STUnInitialize(void);
    bool __fastcall EnableWriting(void);
    bool __fastcall Erase64KSector(int Address);
    bool __fastcall Write64Bytes(int Address);
    bool __fastcall CheckNotBlank(void);

    void __fastcall UploadBIOStoFlash(void);
    void __fastcall UploadRBFtoFlash(void);
    void __fastcall UploadIMGtoFlash(void);


    __fastcall TFlashTestForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFlashTestForm1 *FlashTestForm1;
//---------------------------------------------------------------------------
#endif
