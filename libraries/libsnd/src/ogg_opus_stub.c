/* Stub: Opus disabled in this build. Provides the symbol ogg.c references. */
#include "sfconfig.h"
#include "sndfile.h"
#include "common.h"

int ogg_opus_open (SF_PRIVATE *psf) ;

int
ogg_opus_open (SF_PRIVATE *psf)
{	(void) psf ;
	return SFE_UNIMPLEMENTED ;
}
