#include <windows.h>
#include <tchar.h>
#include <cassert>
#include <objIdl.h>
#include <gdiplus.h>
#include "EyeXGaze.h"
#include "resource.h"

//EyeX Engine 상태메시지
#define WM_EYEX_HOST_STATUS_CHANGED		WM_USER + 0
#define WM_REGION_GOT_ACTIVATION_FOCUS	WM_USER + 1
#define WM_REGION_ACTIVATED				WM_USER + 2

// Memo 클릭 버튼 핸들러 
#define IDC_MEMO_BUTTON 1000

// 버튼 ID
#define OK_BUTTON_CLICKED				50

#pragma comment(lib, "Gdiplus.lib")
using namespace Gdiplus;

// constants 
static const TCHAR* g_szWindowClass = _T("GazePoint");

// global variables
HINSTANCE g_hInstance;
HWND hwnd;
HWND b_hwnd;// 메모 버튼을 위한 핸들러
RECT simdb[30];

// tobii global variable ( constructor )
static EyeXGaze g_EyeXGaze;	// 인스턴스 생성하면서 생성자 실행됨

// RECT 영역 초기화
void initSim(int num, int left, int right, int top, int bottom)
{
	POINT tmp;

	// 왼쪽 위 좌표
	tmp.x = (LONG)left;
	tmp.y = (LONG)top;

	ClientToScreen(hwnd, &tmp);
	simdb[num].left = tmp.x;
	simdb[num].top = tmp.y;

	// 오른쪽 아래 좌표
	tmp.x = (LONG)right;
	tmp.y = (LONG)bottom;

	ClientToScreen(hwnd, &tmp);
	simdb[num].right = tmp.x;
	simdb[num].bottom = tmp.y;
}

RECT GetScreenBounds(HWND hButton)
{
	POINT point = { 0, 0 };
	ClientToScreen(hButton, &point);

	RECT bounds;
	bounds.left = point.x;
	bounds.top = point.y;
	bounds.right = bounds.left + 80;
	bounds.bottom = bounds.top + 50;

	return bounds;
}

void UpdateActivatableRegions()	// 이부분 문제 생김, assert에서 문제생겼다고나옴// 해당 버튼 지역을 통과하면 오류발생
{
	std::vector<EyeXGaze::ActivatableRegion> regions; // 자료형이 EyeXHost 클래스의 ActivatableRegion구조체(int id,와 RECT)인 vector 생성.
	 
	regions.push_back(EyeXGaze::ActivatableRegion(49, GetScreenBounds(b_hwnd)));

	for (int i = 0; i < 29;i++)
	{
		regions.push_back(EyeXGaze::ActivatableRegion(50+i, simdb[i]));
	}

	g_EyeXGaze.SetActivatableRegions(regions); // 해당 지역을 ActivatableRegion으로 설정.

}

void OnStatusChanged(bool engineConnectionIsFunctional)
{
	// update the window title to reflect the engine connection state.
	if (engineConnectionIsFunctional)
	{
		SetWindowText(hwnd, _T("EyeKeyboard - Use Ur Eyez! :)"));
	}
	else
	{
		SetWindowText(hwnd, _T("EyeKeyboard - Mouse Only :("));
	}
}


void OnDraw(HDC hdc, HWND hWnd)
{
	Graphics graphics(hdc);

	SolidBrush gazeBrush3(Color(100, 204, 102, 153));
	

	//	Gaze Data Stream 을 나타내는 좌표
	//	Gaze stream을 얻고자하면 밑에 주석을 제거한다.

	//	POINT rectTL;
	//	rectTL.x = (LONG)g_EyeXGaze.getGazeEye_X();
	//	rectTL.y = (LONG)g_EyeXGaze.getGazeEye_Y();

	//	Fixation Data Stream 을 나타내는 좌표
	POINT rectFTL;
	rectFTL.x = (LONG)g_EyeXGaze.getFixEye_X();
	rectFTL.y = (LONG)g_EyeXGaze.getFixEye_Y();
	//rectFTL.x = 200;
	//rectFTL.y = 200;
	//	ScreenToClient(hWnd, &rectTL);
	ScreenToClient(hWnd, &rectFTL);
	
	graphics.FillEllipse(&gazeBrush3, rectFTL.x - 20, rectFTL.y - 20, 40, 40);

}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, MemDC;
	PAINTSTRUCT ps;
	HBITMAP MyBitmap, OldBitmap;

	HWND memo, memoChild;

	memo = FindWindow(L"Notepad", NULL);   // 메모장의 핸들
	memoChild = FindWindowEx(memo, NULL, L"edit", NULL);   // 메모장에서도 edit 부분의 핸들


	// client 영역 내의 x, y 좌표값
	static long x, y;
	POINT temp = { 0,0 };

	switch (message)
	{
	case WM_CREATE:
		b_hwnd = CreateWindow(L"button", L"MEMO", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 10, 80, 50, hwnd, (HMENU)IDC_MEMO_BUTTON, g_hInstance, NULL);
		break;


	// Trigger Activation발생시 오는 메시지 헨들 케이스
	// 해당 Event가 발생하면 PostMessage 기능을 통해서 hwnd(Client윈도우)에 직접 WM_LBUTTONDOWN
	// 메시지를 부른다. DOWN 후에 UP을 수행한다. ( 클릭 )
	case WM_REGION_ACTIVATED:

		PostMessage(hwnd, WM_LBUTTONDOWN, 0, 0);
		PostMessage(hwnd, WM_LBUTTONUP, 0, 0);
		break;

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDC_MEMO_BUTTON:

			SHELLEXECUTEINFO ExeInfo;
			ZeroMemory(&ExeInfo, sizeof(ExeInfo));
			ExeInfo.cbSize = sizeof(ExeInfo);
			ExeInfo.lpVerb = (L"open");
			ExeInfo.lpFile = (L"notepad.exe");
			ExeInfo.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
			ExeInfo.nShow = SW_SHOW;

			::ShellExecuteEx(&ExeInfo);

			break;
		}
		break;

	case WM_LBUTTONDOWN: // 여기에서 바로 화면에 그릴 수 없고 InvalidateRect함수를 불러야함
						 // WM_PAINT에서만 (BeginPaint, CreateCompatibleDC, LoadBitmap) 등등이 효력있음
		
		// EyeXEngine 에서 주는 Fixation 데이터를 받아온 뒤에 해당 x,y좌표를 임시
		//temp라는 POINT 변수에 할당 한 뒤, ScreenToClient 를 사용하여, Client영역의 좌표로 변환

		temp.x = g_EyeXGaze.getFixEye_X();
		temp.y = g_EyeXGaze.getFixEye_Y();

		ScreenToClient(hwnd, &temp);

		x = temp.x;
		y = temp.y;
	
		InvalidateRect(hwnd, NULL, TRUE);

		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		MemDC = CreateCompatibleDC(hdc);

		//OnDraw(MemDC, hwnd);

		//각 키 클릭했을 때 깜빡이게 함. ok, enter, back, a-z
		if ((212 < x && x < 263) && (94 < y && y <151)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_OK));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 97, 0);

			DeleteObject(MyBitmap);
			DeleteDC(MemDC);

			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((138 < x && x < 199) && (58 < y && y <114)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_Enter));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 13, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((211 < x && x < 262) && (31 < y && y <89)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_Back));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 8, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((199 < x && x < 238) && (235 < y && y <274)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_A));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 97, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((302 < x && x < 346) && (314 < y && y <358)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_B));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 98, 0);

			x = 0, y = 0;
			
			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((205 < x && x < 245) && (326 < y && y <375)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_C));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 99, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((251 < x && x < 295) && (331 < y && y <374)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_D));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 100, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((200 < x && x < 238) && (190 < y && y <250)) {

			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_E));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 101, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((130 < x && x < 183) && (189 < y && y <229)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_F));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 102, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((408 < x && x < 459) && (240 < y && y <296)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_G));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 103, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((353 < x && x < 404) && (224 < y && y <269)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_H));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);

			SendMessage(memoChild, WM_CHAR, 104, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((245 < x && x < 285) && (167 < y && y <207)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_I));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);
			
			SendMessage(memoChild, WM_CHAR, 105, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((70 < x && x < 120) && (172 < y && y <228)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_J));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 106, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((154 < x && x < 199) && (359 < y && y <415)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_K));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 107, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((115 < x && x < 175) && (254 < y && y <290)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_L));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 108, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((273 < x && x < 324) && (382 < y && y <439)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_M));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 109, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((205 < x && x < 257) && (381 < y && y <440)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_N));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 110, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);

			SetTimer(hwnd, 0, 300, NULL);
		}

		else if ((290 < x && x < 337) && (184 < y && y <228)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_O));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 111, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((398 < x && x < 444) && (307 < y && y <365)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_P));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 112, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((80 < x && x < 132) && (105 < y && y <162)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_Q));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 113, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);

			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((154 < x && x < 199) && (283 < y && y <336)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_R));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;
			
			SendMessage(memoChild, WM_CHAR, 114, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((346 < x && x < 398) && (161 < y && y <218)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_S));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 115, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((335 < x && x < 393) && (290 < y && y < 335)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_T));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 116, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((295 < x && x < 335) && (236 < y && y <274)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_U));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 117, 0);
			x = 0, y = 0;

			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}  
		else if ((98 < x && x < 150) && (308 < y && y <365)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_V));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 118, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((160 < x && x < 204) && (132 < y && y <183)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_W));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 119, 0);
			x = 0, y = 0;x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((339 < x && x < 387) && (364 < y && y <416)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_X));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 120, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((407 < x && x < 460) && (166 < y && y <232)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_Y));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 121, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else if ((67 < x && x < 130) && (230 < y && y <300)) {
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_Z));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);

			SelectObject(MemDC, OldBitmap);;

			SendMessage(memoChild, WM_CHAR, 122, 0);
			x = 0, y = 0;
			DeleteObject(MyBitmap);
			DeleteDC(MemDC);


			SetTimer(hwnd, 0, 300, NULL);
		}
		else { // 기본 화면
			MyBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_Keyboard));
			OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);
			OnDraw(MemDC, hwnd);
			
			BitBlt(hdc, 0, 0, 522, 600, MemDC, 0, 0, SRCCOPY);
			
			SelectObject(MemDC, OldBitmap);

			DeleteObject(MyBitmap);
			DeleteDC(MemDC);
		}

		//OnDraw(MemDC, hwnd);

		EndPaint(hwnd, &ps);
		return 0;

	case WM_EYEX_HOST_STATUS_CHANGED:
		OnStatusChanged(wParam != FALSE);
		break;

	case WM_WINDOWPOSCHANGED:

		UpdateActivatableRegions();
		break;

	case WM_TIMER:
	
		InvalidateRect(hwnd, NULL, TRUE);
		//return 0;
		break;

	case WM_KEYDOWN:
		// trigger an activation command when space is pressed.
		if (VK_SPACE == wParam)
		{
			g_EyeXGaze.TriggerActivation();
			// 스페이스를 누를 때, 호스트에 activation 수행을 하라고 보낸다. 
		}
		break;

	case WM_DESTROY:
	{
		KillTimer(hwnd, 0);
		PostQuitMessage(0);
		return 0;
	}
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));	// #000000 검정색으로 배경색 칠한다.
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = g_szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	wcex.lpszMenuName = 0;

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInstance = hInstance;

	GdiplusStartupInput m_GdiplusStartInput;
	ULONG_PTR m_GdiplusToken;
	GdiplusStartup(&m_GdiplusToken, &m_GdiplusStartInput, NULL);
	hwnd = CreateWindow(g_szWindowClass, L"Window Caption", WS_OVERLAPPEDWINDOW,
		100, 100, 522, 600, 0, 0, hInstance, 0);

	if (!hwnd)
	{

		return FALSE;
	}

	// HWND 값이 구해진 다음 영역좌표 초기화
	initSim(0, 212, 263, 94, 151);
	initSim(1, 138, 199, 58, 114);
	initSim(2, 211, 262, 31, 89);
	initSim(3, 199, 238, 235, 274);
	initSim(4, 302, 346, 314, 358);
	initSim(5, 205, 245, 326, 375);
	initSim(6, 251, 295, 331, 374);
	initSim(7, 200, 238, 190, 250);
	initSim(8, 130, 183, 189, 229);
	initSim(9, 408, 459, 240, 296);
	initSim(10, 353, 404, 224, 269);
	//initSim(9, 408, 459, 240, 296);
	//initSim(10, 353, 404, 224, 269);
	initSim(11, 245, 285, 167, 207);
	initSim(12, 70, 120, 172, 228);
	initSim(13, 154, 199, 359, 415);
	initSim(14, 115, 175, 254, 290);
	initSim(15, 273, 324, 382, 439);
	initSim(16, 205, 257, 381, 440);
	initSim(17, 290, 337, 184, 228);
	initSim(18, 398, 444, 307, 365);
	initSim(19, 80, 132, 105, 162);
	initSim(20, 154, 199, 283, 336);
	initSim(21, 346, 398, 161, 218);
	initSim(22, 335, 393, 290, 335);
	initSim(23, 295, 335, 236, 274);
	initSim(24, 98, 150, 308, 365);
	initSim(25, 160, 204, 132, 183);
	initSim(26, 339, 387, 364, 416);
	initSim(27, 407, 460, 166, 232);
	initSim(28, 67, 130, 230, 300);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	//Initialize the EyeXGaze
	g_EyeXGaze.Init(hwnd, WM_EYEX_HOST_STATUS_CHANGED, WM_REGION_GOT_ACTIVATION_FOCUS, WM_REGION_ACTIVATED); // 3가지 상태 메시지 초기화
	UpdateActivatableRegions();

	return TRUE;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	// 선언되지않은 변수 
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	MyRegisterClass(hInstance);


	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	MSG msg;

	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
