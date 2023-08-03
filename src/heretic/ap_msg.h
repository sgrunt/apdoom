#ifndef __APMSG_H__
#define __APMSG_H__


int HULib_measureText(const char* text, int len);
void HU_AddAPMessage(const char* message);
void HU_DrawAPMessages();
void HU_TickAPMessages();
void HU_ClearAPMessages();


#endif /* __APMSG_H__ */
