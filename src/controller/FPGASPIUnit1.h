//---------------------------------------------------------------------------
#ifndef FPGASPIUnit1H
#define FPGASPIUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
#include "DOSeyUnit1.h"
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFPGASPIForm1 : public TForm
{
__published:	// IDE-managed Components
    TPanel *Panel1;
    TPanel *Panel22;
    TLabel *Label46;
    TSplitter *Splitter1;
    TPanel *Panel23;
    TLabel *Label30;
    TMemo *SPIDialogMemo1;
    TButton *FPGASPIButton1;
    TEdit *SPIDataEdit1;
    TButton *ReadEEButton1;
    TButton *WriteEEButton1;
    TEdit *EEAddrEdit1;
    TUpDown *UpDown1;
    TEdit *EEDataEdit1;
    void __fastcall FPGASPIButton1Click(TObject *Sender);
    void __fastcall ReadEEButton1Click(TObject *Sender);
    void __fastcall WriteEEButton1Click(TObject *Sender);
    void __fastcall UpDown1Click(TObject *Sender, TUDBtnType Button);

private:	// User declarations

    byte Report[ReportSize+10];
    byte Buffer[ReportSize+10];

    void __fastcall StartMon(void);
    void __fastcall StopMon(void);
    void __fastcall ReadReport(void);

    void __fastcall FPGA_SPI(byte Data);

public:		// User declarations

    void __fastcall EnableFPGASPI(bool enable);

    byte __fastcall ReadEE(byte Address);
    void __fastcall WriteEE(byte Address, byte Data);

    __fastcall TFPGASPIForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFPGASPIForm1 *FPGASPIForm1;
//---------------------------------------------------------------------------
#endif
