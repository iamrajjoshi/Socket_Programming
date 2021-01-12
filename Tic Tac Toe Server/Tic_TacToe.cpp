#include "Tic_Tac_Toe.h"

int TicTacToe::status() {
	if (board[0] == board[1] && board[1] == board[2])
		return 1;

	else if (board[3] == board[4] && board[4] == board[5])
		return 1;

	else if (board[6] == board[7] && board[7] == board[8])
		return 1;

	else if (board[0] == board[3] && board[3] == board[6])
		return 1;

	else if (board[1] == board[4] && board[4] == board[7])
		return 1;

	else if (board[2] == board[5] && board[5] == board[8])
		return 1;

	else if (board[0] == board[4] && board[4] == board[8])
		return 1;

	else if (board[2] == board[4] && board[4] == board[6])
		return 1;

	else if (board[0] != '0' && board[1] != '1' && board[2] != '2' && board[3] != '3' && board[4] != '4' && board[5] != '5' && board[6] != '6' && board[7] != '7' && board[8] != '8')
		return 0;

	else
		return -1;
}

int TicTacToe::sendBoard(char* buf) {
	char* p = &buf[0];
	*p = '\n';
	p++;
	for (int i = 0; i < 9; ++i) {
		if ((i + 1) % 3 == 0) {
			*p = board[i];
			p++;
			*p = '\n';
			p++;

		}
		else {
			*p = board[i];
			p++;

		}
		
	}
	*p = '\n';
	return p - &buf[0]+1;
}

void TicTacToe::makeMove(int i) {
	board[i] = player[turn];
	turn = !turn;
	return;
}