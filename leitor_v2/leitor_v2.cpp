#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")

typedef struct {
	TCHAR nome[20];
	unsigned int x;
	unsigned int y;
} JOGADOR;

typedef struct {
	int termina;
	JOGADOR jogador;
} MENSAGEM;

MENSAGEM mensagem;
DWORD nBytesEnviados;


int _tmain(int argc, LPTSTR argv[]) {
	HANDLE hPipe;
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
	hPipe = CreateFile(PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPipe == NULL) {
		_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
		exit(-1);
	}
	_tprintf(TEXT("[LEITOR] Liguei-me...\n"));

	do {

		_tprintf(TEXT("[LEITOR] Nome: "));
		_fgetts(mensagem.jogador.nome, 256, stdin);
		mensagem.jogador.nome[_tcslen(mensagem.jogador.nome) - 1] = '\0';
		_tprintf(TEXT("[LEITOR] x: "));
		scanf_s(TEXT("%d"), &mensagem.jogador.x);
		_tprintf(TEXT("[LEITOR] y: "));
		scanf_s(TEXT("%d"), &mensagem.jogador.y);

		if (_tcscmp(mensagem.jogador.nome, TEXT("fim")))
			mensagem.termina = 1;

		if (!WriteFile(hPipe, &mensagem, sizeof(MENSAGEM), &nBytesEnviados, NULL)) {
			_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
			exit(-1);
		}

	} while (_tcscmp(mensagem.jogador.nome, TEXT("fim")));

	CloseHandle(hPipe);
	Sleep(200);
	return 0;
}