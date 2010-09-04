//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
USEFORM("DOSeyUnit1.cpp", Form1);
USEFORM("HIDLoggerUnit1.cpp", LoggerForm1);
USEFORM("FlashTestUnit1.cpp", FlashTestForm1);
USEFORM("RTCUnit1.cpp", RTCForm1);
USEFORM("FPGASPIUnit1.cpp", FPGASPIForm1);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
         Application->Initialize();
         Application->Title = "DOSey Configurtizer";
         Application->HelpFile = "DOSeyHelp.mht";
		Application->CreateForm(__classid(TForm1), &Form1);
         Application->CreateForm(__classid(TLoggerForm1), &LoggerForm1);
         Application->CreateForm(__classid(TFlashTestForm1), &FlashTestForm1);
         Application->CreateForm(__classid(TRTCForm1), &RTCForm1);
         Application->CreateForm(__classid(TFPGASPIForm1), &FPGASPIForm1);
         Application->Run();
    }
    catch (Exception &exception)
    {
         Application->ShowException(&exception);
    }
    catch (...)
    {
         try
         {
             throw Exception("");
         }
         catch (Exception &exception)
         {
             Application->ShowException(&exception);
         }
    }
    return 0;
}
//---------------------------------------------------------------------------
