#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")
#define N_PIPES 4

typedef struct {
	TCHAR nome[20];
	unsigned int x;
	unsigned int y;
} JOGADOR;

typedef struct {
	int termina;
	JOGADOR jogador;
} MENSAGEM;

typedef struct {
	HANDLE hInstance;
	OVERLAPPED overlap;
	BOOL activo;
} PIPEDATA;

typedef struct {
	PIPEDATA hPipes[N_PIPES];
	HANDLE hEvent[N_PIPES];
	HANDLE hMutex;
	DWORD nBytesEnviados;
	DWORD nBytesRecebidos;
	DWORD pipeNumber;
	int termina = 0;
	int numClientes = 0;
} SincPipes;

int i;
HANDLE hT;

MENSAGEM mensagem;
SincPipes sincPipes;

DWORD WINAPI ThreadConsola(LPVOID param);

int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	sincPipes.hMutex = CreateMutex(
		NULL,					// default security attributes
		FALSE,					// initially not owned
		TEXT("ex3-escritor"));	// unnamed mutex
	if (sincPipes.hMutex == NULL)
	{
		_tprintf(TEXT("CreateMutex error: %d\n"), GetLastError());
		return 1;
	}

	for (i = 0; i < N_PIPES; i++) {
		_tprintf(TEXT("[ESCRITOR] Criar uma c�pia do pipe '%s' ... (CreateNamedPipe)\n"), PIPE_NAME);

		sincPipes.hEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);

		sincPipes.hPipes[i].hInstance = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT |
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, N_PIPES, sizeof(MENSAGEM), sizeof(MENSAGEM), 1000, NULL);

		if (sincPipes.hPipes[i].hInstance == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)\n"));
			exit(-1);
		}

		ZeroMemory(&sincPipes.hPipes[i].overlap, sizeof(sincPipes.hPipes[i].overlap));
		sincPipes.hPipes[i].overlap.hEvent = sincPipes.hEvent[i];

		_tprintf(TEXT("[ESCRITOR] Esperar liga��o overlapped de um leitor...(ConnectNamedPipe)\n"));

		if (ConnectNamedPipe(sincPipes.hPipes[i].hInstance, &sincPipes.hPipes[i].overlap) != 0) {
			_tprintf(TEXT("[ERRO] Liga��o ao leitor! (ConnectNamedPipe\n"));
			break;
		}

		sincPipes.hPipes[i].activo = FALSE;

	}

	hT = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadConsola, NULL, 0, NULL);
	if (hT != NULL)
		_tprintf(TEXT("Lancei uma thread...\n"));
	else
		_tprintf(TEXT("Erro ao criar Thread\n"));

	while (!sincPipes.termina && sincPipes.numClientes < N_PIPES) {

		_tprintf(TEXT("[ESCRITOR] Esperar liga��o de um leitor...\n"));

		sincPipes.pipeNumber = WaitForMultipleObjects(N_PIPES, sincPipes.hEvent, FALSE, INFINITE);
		i = sincPipes.pipeNumber - WAIT_OBJECT_0;

		_tprintf(TEXT("[ESCRITOR] New i = %d \n"), i);

		if (i >= 0 && i < N_PIPES)
		{
			if (!GetOverlappedResult(sincPipes.hPipes[i].hInstance, &sincPipes.hPipes[i].overlap, &sincPipes.nBytesRecebidos, FALSE)) {
				_tprintf(TEXT("[ERRO] obter resultados de %d \n"), i);
				continue;
			}

			ResetEvent(sincPipes.hEvent[i]);
			WaitForSingleObject(sincPipes.hMutex, INFINITE);
			sincPipes.hPipes[i].activo = TRUE;
			ReleaseMutex(sincPipes.hMutex);
			sincPipes.numClientes++;
		}

	}

	WaitForSingleObject(hT, INFINITE);

	_tprintf(TEXT("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
	for (i = 0; i < N_PIPES; i++) {
		SetEvent(sincPipes.hEvent[i]);
		if (!DisconnectNamedPipe(sincPipes.hPipes[i].hInstance)) {
			_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)\n"));
			exit(-1);
		}
		CloseHandle(sincPipes.hPipes[i].hInstance);
		CloseHandle(sincPipes.hEvent[i]);
	}

	Sleep(2000);
	exit(0);
}

DWORD WINAPI ThreadConsola(LPVOID param) {
	mensagem.jogador.x = 123;
	mensagem.jogador.y = 321;
	mensagem.termina = 0;

	while (_tcscmp(mensagem.jogador.nome, TEXT("fim"))) {

		WaitForSingleObject(sincPipes.hMutex, INFINITE);

		for (i = 0; i < N_PIPES; i++) {
			if (sincPipes.hPipes[i].activo) {

				if (!ReadFile(sincPipes.hPipes[i].hInstance, &mensagem, sizeof(MENSAGEM), &sincPipes.nBytesRecebidos, NULL)) {
					_tprintf(TEXT("[ERRO] Ler do pipe! (ReadFile)\n"));
					exit(-1);
				}

				_tprintf(TEXT("[ESCRITOR] Recebi %d bytes do i=%d: '%s' (%d, %d) termina=%d... (ReadFile)\n"), sincPipes.nBytesRecebidos, i, mensagem.jogador.nome, mensagem.jogador.x, mensagem.jogador.y, mensagem.termina);

			}
		}

		ReleaseMutex(sincPipes.hMutex);

	}

	for (i = 0; i < N_PIPES; i++)
		SetEvent(sincPipes.hEvent[i]);

	return 0;
}