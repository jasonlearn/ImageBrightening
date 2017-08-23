/*******************************************************************
Purpose: Image brighten with C, assembly, MMX
*******************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <Strsafe.h>
HBITMAP hBitmap;
BITMAP Bitmap;
BITMAPFILEHEADER bmfh;
BITMAPINFO     * pbmi;
BYTE* pBits;
HWND hWnd;

LRESULT CALLBACK HelloWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCMLine, int iCmdShow) {
	static TCHAR szAppName[] = TEXT("HelloApplication");//name of app
	HWND	hwnd;//holds handle to the main window
	MSG		msg;//holds any message retrieved from the msg queue
	WNDCLASS wndclass;//wnd class for registration

					  //defn wndclass attributes for this application
	wndclass.style = CS_HREDRAW | CS_VREDRAW;//redraw on refresh both directions
	wndclass.lpfnWndProc = HelloWndProc;//wnd proc to handle windows msgs/commands
	wndclass.cbClsExtra = 0;//class space for expansion/info carrying
	wndclass.cbWndExtra = 0;//wnd space for info carrying
	wndclass.hInstance = hInstance;//application instance handle
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);//set icon for window
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);//set cursor for window
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//set background
	wndclass.lpszMenuName = NULL;//set menu
	wndclass.lpszClassName = szAppName;//set application name

									   //register wndclass to O/S so approp. wnd msg are sent to application
	if (!RegisterClass(&wndclass)) {
		MessageBox(NULL, TEXT("This program requires Windows 95/98/NT"),
			szAppName, MB_ICONERROR);//if unable to be registered
		return 0;
	}
	//create the main window and get it's handle for future reference
	hwnd = CreateWindow(szAppName,		//window class name
		TEXT("Hello World for Windows"), // window caption
		WS_OVERLAPPEDWINDOW,	//window style
		CW_USEDEFAULT,		//initial x position
		CW_USEDEFAULT,		//initial y position
		CW_USEDEFAULT,		//initial x size
		CW_USEDEFAULT,		//initial y size
		NULL,				//parent window handle
		NULL,				//window menu handle
		hInstance,			//program instance handle
		NULL);				//creation parameters
	ShowWindow(hwnd, iCmdShow);//set window to be shown
	UpdateWindow(hwnd);//force an update so window is drawn
	hWnd = hwnd;
					   //messgae loop
	while (GetMessage(&msg, NULL, 0, 0)) {//get message from queue
		TranslateMessage(&msg);//for keystroke translation
		DispatchMessage(&msg);//pass msg back to windows for processing
							  //note that this is to put windows o/s in control, rather than this app
	}

	return msg.wParam;
}
void nonAsMbrighten(BITMAP* bitmap, INT brighten, BYTE* temppBits) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsperPixel = bitmap->bmBitsPixel;
	BYTE value = 0;//temp location of brighten value

	for (int i = 0; i < height * 3; i++) {
		for (int j = 0; j < width; j++) {
			value = temppBits[i*width + j];
			value = min(255, value + brighten);//deal with rollover via saturation
			pBits[i*width + j] = value;
		}
	}
}

/**
mmx registers: mm0-mm7 - general purpose
pxor - packed xor between two registers
movd - moves a double word (32 bits) between memory and reg or reg to reg
por - packed "or" between two registers
psllq - packed shift left quad (4 DWORDS)
movq - move quad (4 DWORDS or 64 bits) between memory and reg or reg to reg
paddusb - adds unsigned bytes (8 bytes) with saturation between two reg

*/
void mmx_brighten(BITMAP* bitmap, INT brighten, BYTE* buffer) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsPerPixel = bitmap->bmBitsPixel;

	__int64 startTime = 0, endTime = 0, elapsed = 0, f = 0;

	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	QueryPerformanceFrequency((LARGE_INTEGER*)&f);

	__asm {
		//save all reg values to stack
		push eax
		push ebx
		push ecx
		push edx
		push esi
		push edi
		push ebp

		xor eax, eax
		//number of pixels in image (w*h) in reg eax
		mov eax, width
		mul height

		//number of bytes in image (bitsPerPixel/8*pixels)
		xor ebx, ebx // clear ebx
		xor ecx, ecx

		mov ebx, bitsPerPixel
		shr ebx, 3

		mul ebx

		//divide eax by 8 as each mmx reg holds 8 bytes

		//store buffer in reg ebx
		mov ebx, buffer

		//clear mm2 reg
		pxor mm2, mm2
		pxor mm1, mm1 
		pxor mm0, mm0

		//store brighten value
		movd mm2, brighten

		//brighten value needs to be in each byte of an mmx reg
		//loop and shift and load brighten value and "or"
		//until each byte in an mmx reg holds brighten value
		//use mm0 to hold value


		por mm1, mm2
		psllq mm1, 8

		por mm1, mm2
		psllq mm1, 8

		por mm1, mm2
		psllq mm1, 8

		por mm1, mm2
		psllq mm1, 8

		por mm1, mm2
		psllq mm1, 8

		por mm1, mm2
		psllq mm1, 8

		por mm1, mm2
		psllq mm1, 8

		por mm1, mm2

		start: cmp eax, ecx			
					jle end



		//note: can't use mm2 as work (calc) can only be done
		//using mmx reg. Only loading in a value can be done using
		//memory and mmx reg

		//clear ecx reg to use as counter
		//start a loop
		//end loop if number of loops is greater than bytes
		//in image/8

		//load 8 bytes into mm1 

		//add brighten value with saturation

		//copy brighten value back to buffer
		movq mm0, [ebx]
		paddusb mm0, mm1
		movq [ebx], mm0
		//move the buffer pointer position by 8
		add ebx, 8
		//since we are grabbing 8 bytes at once

		//inc our counter (ecx)
		add ecx, 8

		//loop back to repeat
		jmp start

		end:

		//return reg values from stack
		pop ebp
		pop edi
		pop esi
		pop edx
		pop ecx
		pop ebx
		pop eax	

		//end mmx (emms)
	}
	
	QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
	elapsed = endTime - startTime;
	elapsed /= f;
	TCHAR pszDest[1000];
	size_t cbDest = 1000 * sizeof(TCHAR);
	LPCTSTR pszFormat = TEXT("%lld");
	pszDest[elapsed];

	TCHAR* pszTxt = TEXT(pszDest);
	StringCbPrintf(pszDest, cbDest, pszFormat, pszTxt);
	
	MessageBox(hWnd, pszDest, "Time taken", MB_OK);
}

void assembly_brighten(BITMAP* bitmap, INT brighten, BYTE* buffer) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsPerPixel = bitmap->bmBitsPixel;
	//REGISTERS

	//EAX, EBX, ECX, EDX are general purpose registers (32 bits per register)
	//ESI, EDI, EBP are also available as general purpose registers
	//AX, BX, CX, DX are the lower 16-bit of the above registers (think E as extended) (get lower 16 bits of register)
	//AH, AL, BH, BL, CH, CL, DH, DL are 8-bit high/low registers of the above (AX, BX, etc) (get 8 bits of high and low)
	//Typical use:
	//EAX accumulator for operands and results (accumulator - alu, EAX is the default of where it gets stored)
	//EBX base pointer to data in the data segment
	//ECX counter for loops
	//EDX data pointer and I/O pointer
	//EBP frame pointer for stack frames
	//ESP stack pointer hardcoded into PUSH/POP operations
	//ESI source index for array operations
	//EDI destination index for array operations [e.g. copying arrays]
	//EIP instruction pointer
	//EFLAGS results flag hardcoded into conditional operations

	//SOME INSTRUCTIONS

	//MOV <destination>, <source>: mov reg, reg;		//copy src reg to dest reg
	//							   mov reg, immediate;  //set dest reg with imm
	//							   mov reg, memory;     //set memory into dest reg
	//							   mov mem, reg; 
	//							   mov mem, imm
	//INC and DEC on registers or memory
	//ADD destination, source
	//SUB destination, source
	//CMP destination, source : sets the appropriate flag after performing (destination) - (source)
	//JMP label - jumps unconditionally ie. always to location marked by "label"  // label as in where you're jumping to.
																				  // 
	//JE - jump if equal, JG/JL - jump if greater/less, JGE/JLE if greater or equal/less or equal, JNE - not equal, JZ - zero flag set
	//LOOP target: uses ECX to decrement and jump while ECX>0
	//logical instructions: AND, OR, XOR, NOT - performs bitwise logical operations. Note TEST is non-destructive AND instruction
	//SHL destination, count : shift left, SHR destination, count :shift right - carry flag (CF) and zero (Z) bits used, CL register often used if shift known
	//ROL - rotate left, ROR rotate right, RCL (rotate thru carry left), RCR (rotate thru carry right)
	//EQU - used to elimate hardcoding to create constants
	//MUL destination, source : multiplication
	//PUSH <source> - pushes source onto the stack
	//POP <destination> - pops off the stack into destination
	__asm // trigger assembly
	{
		//D1. save all registers you will be using onto stack
		push eax
		push ebx
		push ecx
		push edx
		push esi
		push edi
		push ebp

		xor eax, eax
		//D2. calculate the number of pixels
		//1. Choose a register
		//2. eax chosen
		//3. mov width to eax
		mov eax, width
		//4. mul height to eax
		mul height

		xor ebx, ebx // clear ebx

		//5. need to convert bit to byte
		//6. divide by 8, but in computer wise, shift right by 3
		mov ebx, bitsPerPixel
		shr ebx, 3

		//D3. calculate the number of bytes in image (pixels*bitsperpixel)
		mul ebx

		//D4. store the address of the buffer into a register (e.g. ebx)
		mov ebx, buffer

		//D5. setup counter register
		xor ecx, ecx //counter

		//D6. create a loop
		//D7. loop while still have pixels to brighten
		//D8. jump out of loop if done
		start: cmp eax, ecx //doing subtraction eax - ecx, doesn't store result but sets flag
			   jle end //jump left and end

		//D9. load a pixel into a register
		mov edx, [ebx] // [ ] means to treat it as a pointer/address to get the value.  equivalance to defrerencing
		mov esi, [ebx]
		mov edi, [ebx]
		mov ebp, [ebx]

		//D10. need to work with each colour plane: R G B A
		//treat each R, G, B, A as individual channel
		
		shr esi, 8
		shr edi, 16
		shr ebp, 24

		and edx, 255 // mask out
		and esi, 255
		and edi, 255
		and ebp, 255
		
		
		add edx, 10
		add esi, 10
		add edi, 10
		add ebp, 10

		cmp edx, 255
			jl unsaturate
		mov edx, 255

		unsaturate:
		cmp esi, 255
			jl esiunsaturate
		mov esi, 255

		esiunsaturate:
		cmp edi, 255
			jl ediunsaturate
		mov edi, 255

		ediunsaturate :
		cmp ebp, 255
			jl ebpunsaturate
		mov ebp, 255


		//D11. load same pixel then into 3 more registers
		//D12. shift bits down for each channel
		//D13. clear out other bits


		//D14. add brighten value to each pixel


		//D15. check each pixel to see if saturated
		//D16. if greater than 255, set to 255
		ebpunsaturate:

		//D17. put pixel back together again
		shl ebp, 24
		shl edi, 16
		shl esi, 8

		//D18. shift each channel amount needed
		//D19. add each channel
		add edx, ebp
		add edx, edi
		add edx, esi

		//D20. store back into buffer
		mov [ebx], edx
		
		//D21. increment loop counter by 4
		add ebx, 4
		add ecx, 4

		//D22. loop back up
		jmp start
		
		end: 

		//D23. restore registers to original values before leaving
		//function
		pop ebp
		pop edi
		pop esi
		pop edx
		pop ecx
		pop ebx
		pop eax
	}

}

/**
Purpose: To handle windows messages for specific cases including when
the window is first created, refreshing (painting), and closing
the window.

Returns: Long - any error message (see Win32 API for details of possible error messages)
Notes:	 CALLBACK is defined as __stdcall which defines a calling
convention for assembly (stack parameter passing)
**/
LRESULT CALLBACK HelloWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HDC		hdc;
	PAINTSTRUCT ps;
	RECT	rect;
	BITMAP* bitmap;
	HDC hdcMem;
	BOOL bSuccess;
	DWORD dwBytesRead, dwInfoSize;
	HANDLE hFile;
	int error = 0;
	BYTE *temppBits;


	switch (message) {
	case WM_CREATE://additional things to do when window is created


		hFile = CreateFile(TEXT("D:\\School\\Fall 2016\\COMP - 3770 - Windows Programming\\Assignment5\\Asn5\\splash.bmp"), GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE) {
			error = GetLastError();
			return 0;
		}

		error = sizeof(BITMAPFILEHEADER);

		bSuccess = ReadFile(hFile, &bmfh, sizeof(BITMAPFILEHEADER),
			&dwBytesRead, NULL);

		if (!bSuccess || (dwBytesRead != sizeof(BITMAPFILEHEADER))
			|| (bmfh.bfType != *(WORD *) "BM"))
		{
			//			CloseHandle(hFile);
			return NULL;
		}
		dwInfoSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);

		pbmi = (BITMAPINFO*)malloc(dwInfoSize);

		bSuccess = ReadFile(hFile, pbmi, dwInfoSize, &dwBytesRead, NULL);

		if (!bSuccess || (dwBytesRead != dwInfoSize))
		{
			free(pbmi);
			CloseHandle(hFile);
			return NULL;
		}
		hBitmap = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (VOID**)&pBits, NULL, 0);
		ReadFile(hFile, pBits, bmfh.bfSize - bmfh.bfOffBits, &dwBytesRead, NULL);



		GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		return 0;
	case WM_LBUTTONDOWN:
		hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		temppBits = (BYTE*)malloc(Bitmap.bmWidth*Bitmap.bmHeight * 3);
		memcpy(temppBits, pBits, Bitmap.bmWidth*Bitmap.bmHeight * 3);
		hBitmap = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (VOID**)&pBits, NULL, 0);
		//nonAsMbrighten(&Bitmap, 30, temppBits);
		//brighten(&Bitmap, 30, temppBits);
		//assembly_brighten(&Bitmap, 30, temppBits);
		mmx_brighten(&Bitmap, 30, temppBits);
		memcpy(pBits, temppBits, Bitmap.bmWidth*Bitmap.bmHeight * 3);
		SelectObject(hdcMem, hBitmap);

		BitBlt(hdc, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
			hdcMem, 0, 0, SRCCOPY);
		return 0;
	case WM_RBUTTONDOWN:
		hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		temppBits = (BYTE*)malloc(Bitmap.bmWidth*Bitmap.bmHeight * 3);
		memcpy(temppBits, pBits, Bitmap.bmWidth*Bitmap.bmHeight * 3);
		hBitmap = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (VOID**)&pBits, NULL, 0);
		nonAsMbrighten(&Bitmap, 30, temppBits);
		SelectObject(hdcMem, hBitmap);

		BitBlt(hdc, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
			hdcMem, 0, 0, SRCCOPY);
		return 0;
	case WM_PAINT://what to do when a paint msg occurs
		hdc = BeginPaint(hwnd, &ps);//get a handle to a device context for drawing
		GetClientRect(hwnd, &rect);//define drawing area for clipping
								   //GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
		hdcMem = CreateCompatibleDC(hdc);
		SelectObject(hdcMem, hBitmap);

		BitBlt(hdc, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
			hdcMem, 0, 0, SRCCOPY);


		EndPaint(hwnd, &ps);//release the device context
		return 0;

	case WM_DESTROY://how to handle a destroy (close window app) msg
		PostQuitMessage(0);
		return 0;
	}
	//return the message to windows for further processing
	return DefWindowProc(hwnd, message, wParam, lParam);
}
