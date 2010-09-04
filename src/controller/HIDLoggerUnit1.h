//---------------------------------------------------------------------------
#ifndef HIDLoggerUnit1H
#define HIDLoggerUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TLoggerForm1 : public TForm
{
__published:	// IDE-managed Components
    TMemo *HidLoggerMemo1;
    TTimer *Timer2;
    TPanel *Panel5;
    TButton *StartMonButton1;
    TButton *StopMonButton1;
    TButton *SendReportButton1;
    TButton *ReadHidButton1;
    TEdit *Edit1;
    TEdit *Edit2;
    TCheckBox *CheckBox1;
    void __fastcall StartMonButton1Click(TObject *Sender);
    void __fastcall StopMonButton1Click(TObject *Sender);
    void __fastcall SendReportButton1Click(TObject *Sender);
    void __fastcall ReadHidButton1Click(TObject *Sender);
    void __fastcall CheckBox1Click(TObject *Sender);
    void __fastcall Timer2Timer(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);

private:	// User declarations

public:		// User declarations

    __fastcall TLoggerForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TLoggerForm1 *LoggerForm1;
//---------------------------------------------------------------------------
#endif
