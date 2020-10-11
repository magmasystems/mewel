#include <wprivate.h>
#include <window.h>
#include <wgraphic.h>

void * SEQ mwGrafAlloc( size_t size )
{
   return malloc( size );
}

void SEQ mwGrafFree( void *ptr )
{
   free(ptr);
}


#if defined(__WATCOMC__)
int SEQ mwClipRegion(region *mwRGN)
{
  (void) mwRGN;
  return 0;
}
region * SEQ mwRectListToRegion(int mwNUMRECTS, rect *mwRLIST)
{
  (void) mwNUMRECTS;
  (void) mwRLIST;
  return (region *) NULL;
}

void SEQ mwTextBold(int mwPIXBOLD )
{
  (void) mwPIXBOLD;
}

#endif

