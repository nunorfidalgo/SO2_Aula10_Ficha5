#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")
#define TAM 256
#define N 10

/*

NOTA para ex6:

write  read
E --> L
	  ! -> muda para Uppercase;
 <-----
 Read  Write

 PIPE ACCESS DUPLEX!!!

*/

typedef struct {
	HANDLE hInstance;
	OVERLAPPED overlap;
	BOOL activo;
} PIPEDATA;

DWORD WINAPI ThreadConsola(LPVOID param);

PIPEDATA hPipes[N];
HANDLE hEvent[N];
HANDLE hMutex;

TCHAR buf[TAM];
DWORD n;
int i, end = 0, numClientes = 0;

HANDLE hT;
DWORD x;
OVERLAPPED a[N];

int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	// mutex
	hMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		TEXT("ex3-escritor"));      // unnamed mutex
	if (hMutex == NULL)
	{
		_tprintf(TEXT("CreateMutex error: %d\n"), GetLastError());
		return 1;
	}

	for (i = 0; i < N; i++) {
		_tprintf(TEXT("[ESCRITOR] Criar uma cópia do pipe '%s' ... (CreateNamedPipe)\n"), PIPE_NAME);

		hEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);

		hPipes[i].hInstance = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT |
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, sizeof(TCHAR), TAM * sizeof(TCHAR), 1000, NULL);

		if (hPipes[i].hInstance == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)\n"));
			exit(-1);
		}

		ZeroMemory(&hPipes[i].overlap, sizeof(hPipes[i].overlap));
		hPipes[i].overlap.hEvent = hEvent[i];

		_tprintf(TEXT("[ESCRITOR] Esperar ligação overlapped de um leitor...(ConnectNamedPipe)\n"));

		if (ConnectNamedPipe(&hPipes[i].hInstance, &hPipes[i].overlap) != 0) {
			_tprintf(TEXT("[ERRO] Ligação ao leitor! (ConnectNamedPipe\n"));
			break;
		}

		hPipes[i].activo = FALSE;

	}

	hT = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadConsola, NULL, 0, NULL);
	if (hT != NULL)
		_tprintf(TEXT("Lancei uma thread...\n"));
	else
		_tprintf(TEXT("Erro ao criar Thread\n"));

	while (!end && numClientes < N) {

		_tprintf(TEXT("[ESCRITOR] Esperar ligação de um leitor...\n"));

		x = WaitForMultipleObjects(N, hEvent, FALSE, INFINITE);
		i = x - WAIT_OBJECT_0;

		_tprintf(TEXT("[ESCRITOR] New i = %d \n"), i);

		if (i >= 0 && i < N)
		{
			if (!GetOverlappedResult(&hPipes[i].hInstance, &hPipes[i].overlap, &n, FALSE)) {
				_tprintf(TEXT("[ERRO] obter resultados de %d \n"), i);
				continue;
			}
		}
		ResetEvent(hEvent[i]);
		WaitForSingleObject(hMutex, INFINITE);
		hPipes[i].activo = TRUE;
		ReleaseMutex(hMutex);
		numClientes++;

	}

	WaitForSingleObject(hT, INFINITE);

	_tprintf(TEXT("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
	for (i = 0; i < N; i++) {
		SetEvent(hEvent[i]);
		if (!DisconnectNamedPipe(hPipes[i].hInstance)) {
			_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)\n"));
			exit(-1);
		}
		CloseHandle(hPipes[i].hInstance);
		CloseHandle(hEvent[i]);
	}

	Sleep(2000);
	exit(0);
}

DWORD WINAPI ThreadConsola(LPVOID param) {
	int i;
	DWORD n;
	do {
		_tprintf(TEXT("[ESCRITOR] Frase: "));
		_fgetts(buf, 256, stdin);
		buf[_tcslen(buf) - 1] = '\0';

		WaitForSingleObject(hMutex, INFINITE);
		// regiao critica
		for (i = 0; i < N; i++) {
			if (hPipes[i].activo) {
				if (!WriteFile(hPipes[i].hInstance, buf, _tcslen(buf) * sizeof(TCHAR), &n, NULL)) {
					_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
					exit(-1);
				}
			}
		}
		ReleaseMutex(hMutex);

		/*_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor...(WriteFile)\n"), n);*/
	} while (_tcscmp(buf, TEXT("fim")));

	end = 1;

	for (i = 0; i < N; i++)
		SetEvent(hEvent[i]);

	return 0;
}

