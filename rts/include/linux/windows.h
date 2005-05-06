/**
This file is here only for fast porting. 
It has to be removed so that no #include <windows.h>
appears in the common code

@author gnibu

*/

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#warning TODO abstract code from windows code => remove windows.h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define LARGE_INTEGER long int
#define __int64 long int

typedef void *HDC; //FIXME ugly glich to remove

#define MessageBox(hWnd, lpText, lpCaption, uType) fprintf(stderr,lpText)


//FIXME read cpuinfo only one time for many calls (store value in static var)
#define NOMFICH_CPUINFO "/proc/cpuinfo" 
inline bool QueryPerformanceFrequency(LARGE_INTEGER* frequence)
{
  const char* prefixe_cpu_mhz = "cpu MHz";
  FILE* F;
  char ligne[300+1];
  char *pos;
  int ok=0;

  // Ouvre le fichier
  F = fopen(NOMFICH_CPUINFO, "r");
  if (!F) return false;

  // Lit une ligne apres l'autre
  while (!feof(F))
  {
    // Lit une ligne de texte
    fgets (ligne, sizeof(ligne), F);

    // C'est la ligne contenant la frequence?
    if (!strncmp(ligne, prefixe_cpu_mhz, strlen(prefixe_cpu_mhz)))
    {
      // Oui, alors lit la frequence
      pos = strrchr (ligne, ':') +2;
      if (!pos) break;
      if (pos[strlen(pos)-1] == '\n') pos[strlen(pos)-1] = '\0';
      strcpy (ligne, pos);
      strcat (ligne,"e6");
      *frequence = (long int)(atof (ligne)*1000000);
      ok = 1;
      break;
    }
  }
  fclose (F);
  return true;
}




#endif //__WINDOWS_H__
