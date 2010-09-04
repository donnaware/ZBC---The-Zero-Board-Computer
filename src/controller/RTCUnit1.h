//---------------------------------------------------------------------------
#ifndef RTCUnit1H
#define RTCUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
#include "DOSeyUnit1.h"
//---------------------------------------------------------------------------
class TRTCForm1 : public TForm
{
__published:	// IDE-managed Components
    TLabel *Label35;
    TPanel *Panel24;
    TPanel *Panel17;
    TMemo *DumpMemo1;
    TPanel *Panel18;
    TLabel *Label43;
    TPanel *Panel16;
    TLabel *Label36;
    TButton *ReadRTCButton1;
    TButton *SetTrickleButton1;
    TButton *WriteRTCButton1;
    TButton *RTCSyncButton1;
    TButton *ReadAllRTCButton1;
    TEdit *RTCAddrEdit1;
    TEdit *RTCDataEdit1;
    TUpDown *UpDown1;
    TSplitter *Splitter1;
    TPanel *Panel23;
    TLabel *Label30;
    TMemo *RTCDialogMemo1;
    TButton *WriteAllRTCButton1;
    void __fastcall SetTrickleButton1Click(TObject *Sender);
    void __fastcall ReadAllRTCButton1Click(TObject *Sender);
    void __fastcall WriteRTCButton1Click(TObject *Sender);
    void __fastcall RTCSyncButton1Click(TObject *Sender);
    void __fastcall UpDown1Click(TObject *Sender, TUDBtnType Button);

private:	// User declarations

    byte Report[ReportSize+10];
    byte Buffer[ReportSize+10];

    void __fastcall StartMon(void);
    void __fastcall StopMon(void);
    void __fastcall ReadReport(void);

    void __fastcall RTCWrite1Byte(byte address, byte data);
    void __fastcall DumpBuffer(void);



public:		// User declarations

    __fastcall TRTCForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TRTCForm1 *RTCForm1;
//---------------------------------------------------------------------------
#endif
