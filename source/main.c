#include <stdio.h>
#include <string.h>

#include <3ds.h>

static Handle frdaHandle;

void ToUtf16(u16* out, const char* in, size_t max)
{
	if (!in || !*in)
	{
		out[0] = 0;
		return;
	}

	ssize_t units = utf8_to_utf16(out, (const uint8_t*)in, max-1);
	if (units < 0)
	{
		out[0] = 0;
		return;
	}

	out[units] = 0;
}

Result FRD_SetPersonalComment(const char *comment, u32 maxLength)
{
  Result ret = 0;
  u32 *cmdbuf = getThreadCommandBuffer();
  u16 commentUtf16[FRIEND_COMMENT_SIZE];

  maxLength = (maxLength > FRIEND_COMMENT_SIZE) ? FRIEND_COMMENT_SIZE : maxLength;

  ToUtf16(commentUtf16, comment, maxLength);

  cmdbuf[0] = 0x40F0240;
  memcpy(&cmdbuf[1], commentUtf16, FRIEND_COMMENT_SIZE);

  if (R_FAILED(ret = svcSendSyncRequest(frdaHandle))) return ret;

  return 0;
}

void StartSetComment(void)
{
  SwkbdState swkbd;
  char currentComment[0x100];
  char newComment[0x100];

  // Initialize
  memset(currentComment, 0, 0x100);
  memset(newComment, 0, 0x100);
  swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, FRIEND_COMMENT_SIZE / 2);
  swkbdSetHintText(&swkbd, "Enter your comment");
  swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT | SWKBD_FIXED_WIDTH);

  // Set current comment
  FRD_GetMyComment(currentComment, 0x100);
  swkbdSetInitialText(&swkbd, currentComment);

  // Show keyboard
  if(swkbdInputText(&swkbd, newComment, 0x100) == SWKBD_BUTTON_RIGHT)
  {
    if(R_SUCCEEDED(FRD_SetPersonalComment(newComment, 0x100)))
      printf("Successfully set comment!\n");
    else
      printf("Failed to set comment!\n");
  }
  else
    printf("Cancelled!\n");
}

int main(void)
{
  Result ret = 0;
  bool frdFailed = false;

  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);
  frdInit();

  printf("Comment setter                      by Tekito_256\n\n");
  printf("Press A to set your comment\n\n\n");
  printf("---LOGS---\n");

  if(R_FAILED(ret = srvGetServiceHandle(&frdaHandle, "frd:a")))
  {
    printf("Failed to get friend handle!\n");
    printf("Error code: %08lX\n", ret);
    printf("Press Start to exit\n");
    frdFailed = true;
  }

  while (aptMainLoop())
  {
    // Update the screen
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();

    // Update the input
    hidScanInput();
    u32 kDown = hidKeysDown();

    if((kDown & KEY_A) && !frdFailed)
    {
      StartSetComment();
    }
    if(kDown & KEY_START)
      break;
  }

  if(!frdFailed)
    svcCloseHandle(frdaHandle);
  gfxExit();
  frdExit();
  return 0;
}
