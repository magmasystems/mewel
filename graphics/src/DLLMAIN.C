#include <windows.h>
#include <string.h>

extern int main(int argc, char **argv);

main(int argc, char **argv)
{
  char achCmdLine[128];
  int  i;
  HINSTANCE _hInst = 0;

  extern char **_argv;

  /*
    Open the resource file
  */
  strcpy(achCmdLine, _argv[0]);
  strcpy(strrchr(achCmdLine, '.'), ".RES");
  _hInst = OpenResourceFile(achCmdLine);

  /*
    Construct the command line which is going to be passed to WinMain
  */
  achCmdLine[0] = '\0';
  for (i = 1;  i < argc;  i++)
  {
    lstrcat(achCmdLine, argv[i]);
    if (i < argc-1)
      lstrcat(achCmdLine, " ");
  }

  /*
    Initialize MEWEL
  */
  WinInit();
  WinUseSysColors(NULLHWND, TRUE);
  SetWindowsCompatibility(WC_MAXCOMPATIBILITY);

  /*
    Load in the MDI stuff
  */
  MDIInitialize();

  /*
    Call WinMain
  */
  return WinMain((_hInst), 0, (LPSTR) achCmdLine, SW_SHOWNORMAL);
}

