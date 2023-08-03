#include "ap_msg.h"
#include <inttypes.h>
#include "i_timer.h"


#define HU_APMSGTIMEOUT	(5*TICRATE)


int HULib_measureText(const char* text, int len)
{
#if 0
    int			i;
    int			w;
    int			x = 0;
    unsigned char	c;
	patch_t** f = hu_font;

	for (i=0;i<len;i++)
	{
		c = toupper(text[i]);
		if (c == '\n') return x;
		if (c == cr_esc)
		{
			if (text[i+1] >= '0' && text[i+1] <= '0' + CRMAX - 1)
			{
				i++;
			}
		}
		else if (c != ' ' && c <= '_')
		{
			w = SHORT(f[c - '!']->width);
			x += w;
		}
		else
		{
			x += 4;
		}
	}

	return x;
#endif

	return 10;
}


void HU_AddAPMessage(const char* message)
{
}


void HU_DrawAPMessages()
{
}


void HU_TickAPMessages()
{
}


void HU_ClearAPMessages()
{
}
