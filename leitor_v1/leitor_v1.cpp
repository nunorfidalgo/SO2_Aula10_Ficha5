#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")


typedef struct {
	TCHAR nome[20];
	unsigned int x;
	unsigned int y;
} JOGADOR;

// Mensagem
typedef struct {
	int termina;
	JOGADOR jogador;
} MENSAGEM;

MENSAGEM mensagem;
DWORD nBytesRecebidos;


int _tmain(int argc, LPTSTR argv[]) {
	HANDLE hPipe;

	/*TCHAR buf[256];
	DWORD n;*/

	BOOL ret;
	int i = 0;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	_tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_NAME);
	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
		exit(-1);
	}
	_tprintf(TEXT("[LEITOR] Ligação ao pipe do escritor... (CreateFile)\n"));
	hPipe = CreateFile(PIPE_NAME, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPipe == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
		exit(-1);
	}
	_tprintf(TEXT("[LEITOR] Liguei-me...\n"));
	while (1) {
		ret = ReadFile(hPipe, &mensagem, sizeof(MENSAGEM), &nBytesRecebidos, NULL);
		//buf[n / sizeof(TCHAR)] = '\0';
		if (!ret || !nBytesRecebidos) {
			_tprintf(TEXT("[LEITOR] %d %d... (ReadFile)\n"), ret, nBytesRecebidos);
			break;
		}
		_tprintf(TEXT("[LEITOR] Recebi %d bytes: '%s' (%d, %d) termina=%d... (ReadFile)\n"), nBytesRecebidos, mensagem.jogador.nome, mensagem.jogador.x, mensagem.jogador.y, mensagem.termina);
	}
	CloseHandle(hPipe);
	Sleep(200);
	return 0;
}